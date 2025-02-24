/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include "executing_context.h"

#include <utility>
#include "bindings/qjs/converter_impl.h"
#include "bindings/qjs/script_promise_resolver.h"
#include "built_in_string.h"
#include "core/dom/document.h"
#include "core/dom/intersection_observer.h"
#include "core/dom/mutation_observer.h"
#include "core/events/error_event.h"
#include "core/events/promise_rejection_event.h"
#include "event_type_names.h"
#include "foundation/logging.h"
#include "polyfill.h"
#include "qjs_window.h"
#include "script_forbidden_scope.h"
#include "timing/performance.h"

namespace webf {

static std::atomic<int32_t> context_unique_id{0};

#define MAX_JS_CONTEXT 8192
thread_local std::unordered_map<double, bool> valid_contexts;
std::atomic<uint32_t> running_context_list{0};

ExecutingContext::ExecutingContext(DartIsolateContext* dart_isolate_context,
                                   bool is_dedicated,
                                   size_t sync_buffer_size,
                                   double context_id,
                                   JSExceptionHandler handler,
                                   void* owner)
    : dart_isolate_context_(dart_isolate_context),
      context_id_(context_id),
      dart_error_report_handler_(std::move(handler)),
      owner_(owner),
      public_method_ptr_(std::make_unique<ExecutingContextWebFMethods>()),
      is_dedicated_(is_dedicated),
      unique_id_(context_unique_id++),
      is_context_valid_(true) {
  if (is_dedicated) {
    // Set up the sync command size for dedicated thread mode.
    // Bigger size introduce more ui consistence and lower size led to more high performance by the reason of
    // concurrency.
    ui_command_buffer_.ConfigureSyncCommandBufferSize(sync_buffer_size);
  }

  // @FIXME: maybe contextId will larger than MAX_JS_CONTEXT
  assert_m(valid_contexts[context_id] != true, "Conflict context found!");
  valid_contexts[context_id] = true;
  if (context_id > running_context_list)
    running_context_list = context_id;

  time_origin_ = std::chrono::system_clock::now();

  JSContext* ctx = script_state_.ctx();
  global_object_ = JS_GetGlobalObject(script_state_.ctx());

  // Turn off quickjs GC to avoid performance issue at loading status.
  // When the `load` event fired in window, the GC will turn on.
  JS_TurnOffGC(script_state_.runtime());
  JS_SetContextOpaque(ctx, this);
  JS_SetHostPromiseRejectionTracker(script_state_.runtime(), promiseRejectTracker, nullptr);

  dart_isolate_context->profiler()->StartTrackSteps("ExecutingContext::InstallBindings");

  // Register all built-in native bindings.
  InstallBindings(this);

  dart_isolate_context->profiler()->FinishTrackSteps();
  dart_isolate_context->profiler()->StartTrackSteps("ExecutingContext::InstallDocument");

  // Install document.
  InstallDocument();

  dart_isolate_context->profiler()->FinishTrackSteps();
  dart_isolate_context->profiler()->StartTrackSteps("ExecutingContext::InstallGlobal");

  // Binding global object and window.
  InstallGlobal();

  dart_isolate_context->profiler()->FinishTrackSteps();
  dart_isolate_context->profiler()->StartTrackSteps("ExecutingContext::InstallPerformance");

  // Install performance
  InstallPerformance();
  InstallNativeLoader();

  dart_isolate_context->profiler()->FinishTrackSteps();
  dart_isolate_context->profiler()->StartTrackSteps("ExecutingContext::initWebFPolyFill");

  initWebFPolyFill(this);

  dart_isolate_context->profiler()->FinishTrackSteps();
  dart_isolate_context->profiler()->StartTrackSteps("ExecutingContext::InitializePlugin");

  for (auto& p : plugin_byte_code) {
    EvaluateByteCode(p.second.bytes, p.second.length);
  }

  for (auto& p : plugin_string_code) {
    EvaluateJavaScript(p.second.c_str(), p.second.size(), p.first.c_str(), 0);
  }

  dart_isolate_context->profiler()->FinishTrackSteps();

  ui_command_buffer_.AddCommand(UICommand::kFinishRecordingCommand, nullptr, nullptr, nullptr);
}

ExecutingContext::~ExecutingContext() {
  is_context_valid_ = false;
  valid_contexts[context_id_] = false;
  executing_context_status_->disposed = true;

  // Check if current context have unhandled exceptions.
  JSValue exception = JS_GetException(script_state_.ctx());
  if (JS_IsObject(exception) || JS_IsException(exception)) {
    // There must be bugs in native functions from call stack frame. Someone needs to fix it if throws.
    ReportError(exception);
    assert_m(false, "Unhandled exception found when Dispose JSContext.");
  }

  JS_FreeValue(script_state_.ctx(), global_object_);

  // Free active wrappers.
  for (auto& active_wrapper : active_wrappers_) {
    JS_FreeValue(ctx(), active_wrapper->ToQuickJSUnsafe());
  }
}

ExecutingContext* ExecutingContext::From(JSContext* ctx) {
  return static_cast<ExecutingContext*>(JS_GetContextOpaque(ctx));
}

bool ExecutingContext::EvaluateJavaScript(const char* code,
                                          size_t code_len,
                                          uint8_t** parsed_bytecodes,
                                          uint64_t* bytecode_len,
                                          const char* sourceURL,
                                          int startLine) {
  if (ScriptForbiddenScope::IsScriptForbidden()) {
    return false;
  }
  dart_isolate_context_->profiler()->StartTrackSteps("ExecutingContext::EvaluateJavaScript");

  JSValue result;
  if (parsed_bytecodes == nullptr) {
    dart_isolate_context_->profiler()->StartTrackSteps("JS_Eval");

    result = JS_Eval(script_state_.ctx(), code, code_len, sourceURL, JS_EVAL_TYPE_GLOBAL);

    dart_isolate_context_->profiler()->FinishTrackSteps();
  } else {
    dart_isolate_context_->profiler()->StartTrackSteps("JS_Eval");

    JSValue byte_object =
        JS_Eval(script_state_.ctx(), code, code_len, sourceURL, JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);

    dart_isolate_context_->profiler()->FinishTrackSteps();

    if (JS_IsException(byte_object)) {
      HandleException(&byte_object);
      dart_isolate_context_->profiler()->FinishTrackSteps();
      return false;
    }

    dart_isolate_context_->profiler()->StartTrackSteps("JS_Eval");
    size_t len;
    *parsed_bytecodes = JS_WriteObject(script_state_.ctx(), &len, byte_object, JS_WRITE_OBJ_BYTECODE);
    *bytecode_len = len;

    dart_isolate_context_->profiler()->FinishTrackSteps();
    dart_isolate_context_->profiler()->StartTrackSteps("JS_EvalFunction");

    result = JS_EvalFunction(script_state_.ctx(), byte_object);

    dart_isolate_context_->profiler()->FinishTrackSteps();
  }

  DrainMicrotasks();
  bool success = HandleException(&result);
  JS_FreeValue(script_state_.ctx(), result);

  dart_isolate_context_->profiler()->FinishTrackSteps();

  return success;
}

bool ExecutingContext::EvaluateJavaScript(const char16_t* code, size_t length, const char* sourceURL, int startLine) {
  std::string utf8Code = toUTF8(std::u16string(reinterpret_cast<const char16_t*>(code), length));
  JSValue result = JS_Eval(script_state_.ctx(), utf8Code.c_str(), utf8Code.size(), sourceURL, JS_EVAL_TYPE_GLOBAL);
  DrainMicrotasks();
  bool success = HandleException(&result);
  JS_FreeValue(script_state_.ctx(), result);
  return success;
}

bool ExecutingContext::EvaluateJavaScript(const char* code, size_t codeLength, const char* sourceURL, int startLine) {
  JSValue result = JS_Eval(script_state_.ctx(), code, codeLength, sourceURL, JS_EVAL_TYPE_GLOBAL);
  DrainMicrotasks();
  bool success = HandleException(&result);
  JS_FreeValue(script_state_.ctx(), result);
  return success;
}

bool ExecutingContext::EvaluateByteCode(uint8_t* bytes, size_t byteLength) {
  dart_isolate_context_->profiler()->StartTrackSteps("ExecutingContext::EvaluateByteCode");

  JSValue obj, val;

  dart_isolate_context_->profiler()->StartTrackSteps("JS_EvalFunction");

  obj = JS_ReadObject(script_state_.ctx(), bytes, byteLength, JS_READ_OBJ_BYTECODE);

  dart_isolate_context_->profiler()->FinishTrackSteps();

  if (!HandleException(&obj)) {
    dart_isolate_context_->profiler()->FinishTrackSteps();
    return false;
  }

  dart_isolate_context_->profiler()->StartTrackSteps("JS_EvalFunction");

  val = JS_EvalFunction(script_state_.ctx(), obj);

  dart_isolate_context_->profiler()->FinishTrackSteps();

  DrainMicrotasks();
  if (!HandleException(&val)) {
    dart_isolate_context_->profiler()->FinishTrackSteps();
    return false;
  }
  JS_FreeValue(script_state_.ctx(), val);
  dart_isolate_context_->profiler()->FinishTrackSteps();
  return true;
}

bool ExecutingContext::IsContextValid() const {
  return is_context_valid_;
}

void ExecutingContext::SetContextInValid() {
  is_context_valid_ = false;
}

bool ExecutingContext::IsCtxValid() const {
  return script_state_.Invalid();
}

void* ExecutingContext::owner() {
  return owner_;
}

bool ExecutingContext::HandleException(JSValue* exc) {
  if (JS_IsException(*exc)) {
    JSValue error = JS_GetException(script_state_.ctx());
    MemberMutationScope scope{this};
    DispatchGlobalErrorEvent(this, error);
    JS_FreeValue(script_state_.ctx(), error);
    return false;
  }

  return true;
}

bool ExecutingContext::HandleException(ScriptValue* exc) {
  JSValue value = exc->QJSValue();
  return HandleException(&value);
}

bool ExecutingContext::HandleException(ExceptionState& exception_state) {
  if (exception_state.HasException()) {
    JSValue error = JS_GetException(ctx());
    ReportError(error);
    JS_FreeValue(ctx(), error);
    return false;
  }
  return true;
}

bool ExecutingContext ::HandleException(webf::ExceptionState& exception_state,
                                        char** rust_error_msg,
                                        uint32_t* rust_errmsg_len) {
  if (exception_state.HasException()) {
    JSValue error = JS_GetException(ctx());
    ReportError(error, rust_error_msg, rust_errmsg_len);
    JS_FreeValue(ctx(), error);
    return false;
  }
  return true;
}

JSValue ExecutingContext::Global() {
  return global_object_;
}

JSContext* ExecutingContext::ctx() {
  assert(IsCtxValid());
  return script_state_.ctx();
}

void ExecutingContext::ReportError(JSValue error) {
  ReportError(error, nullptr, nullptr);
}

void ExecutingContext::ReportError(JSValueConst error, char** rust_errmsg, uint32_t* rust_errmsg_length) {
  JSContext* ctx = script_state_.ctx();
  if (!JS_IsError(ctx, error))
    return;

  JSValue message_value = JS_GetPropertyStr(ctx, error, "message");
  JSValue error_type_value = JS_GetPropertyStr(ctx, error, "name");
  const char* title = JS_ToCString(ctx, message_value);
  const char* type = JS_ToCString(ctx, error_type_value);
  const char* stack = nullptr;
  JSValue stack_value = JS_GetPropertyStr(ctx, error, "stack");
  if (!JS_IsUndefined(stack_value)) {
    stack = JS_ToCString(ctx, stack_value);
  }

  uint32_t message_length = strlen(type) + strlen(title);
  char* message;
  if (stack != nullptr) {
    message_length += 4 + strlen(stack);
    message = (char*)dart_malloc(message_length * sizeof(char));
    snprintf(message, message_length, "%s: %s\n%s", type, title, stack);
  } else {
    message_length += 3;
    message = (char*)dart_malloc(message_length * sizeof(char));
    snprintf(message, message_length, "%s: %s", type, title);
  }

  // Report errmsg to rust side
  if (rust_errmsg != nullptr && rust_errmsg_length != nullptr) {
    *rust_errmsg = (char*)malloc(sizeof(char) * message_length);
    *rust_errmsg_length = message_length;
    memcpy(*rust_errmsg, message, sizeof(char) * message_length);
  } else {
    // Report errmsg to dart side
    dart_error_report_handler_(this, message);
  }

  JS_FreeValue(ctx, error_type_value);
  JS_FreeValue(ctx, message_value);
  JS_FreeValue(ctx, stack_value);
  JS_FreeCString(ctx, title);
  JS_FreeCString(ctx, stack);
  JS_FreeCString(ctx, type);
}

void ExecutingContext::DrainMicrotasks() {
  dart_isolate_context_->profiler()->StartTrackSteps("ExecutingContext::DrainMicrotasks");

  DrainPendingPromiseJobs();

  dart_isolate_context_->profiler()->FinishTrackSteps();

  ui_command_buffer_.AddCommand(UICommand::kFinishRecordingCommand, nullptr, nullptr, nullptr);
}

namespace {

struct MicroTaskDeliver {
  MicrotaskCallback callback;
  void* data;
};

}  // namespace

void ExecutingContext::EnqueueMicrotask(MicrotaskCallback callback, void* data) {
  JSValue proxy_data = JS_NewObject(ctx());

  auto* deliver = new MicroTaskDeliver();
  deliver->data = data;
  deliver->callback = callback;

  JS_SetOpaque(proxy_data, deliver);

  JS_EnqueueJob(
      ctx(),
      [](JSContext* ctx, int argc, JSValueConst* argv) -> JSValue {
        auto* deliver = static_cast<MicroTaskDeliver*>(JS_GetOpaque(argv[0], JS_CLASS_OBJECT));
        deliver->callback(deliver->data);

        delete deliver;
        return JS_NULL;
      },
      1, &proxy_data);

  JS_FreeValue(ctx(), proxy_data);
}

int32_t ExecutingContext::AddRustFutureTask(const std::shared_ptr<WebFNativeFunction>& run_future_task,
                                            NativeLibraryMetaData* meta_data) {
  meta_data->unique_id_++;
  meta_data->callbacks.emplace_back(meta_data->unique_id_, run_future_task);
  return meta_data->unique_id_;
}

void ExecutingContext::RemoveRustFutureTask(int32_t callback_id, NativeLibraryMetaData* meta_data) {
  // Add the callback to the removed_callbacks list to avoid removing the callback during the iteration.
  meta_data->removed_callbacks.push_back(callback_id);
}

void ExecutingContext::RunRustFutureTasks() {
  for (auto& meta_data : native_library_meta_data_contaner_) {
    for (auto& callback : meta_data->callbacks) {
      dart_isolate_context_->profiler()->StartTrackAsyncEvaluation();
      callback.callback->Invoke(this, 0, nullptr);
      dart_isolate_context_->profiler()->FinishTrackAsyncEvaluation();
    }

    if (meta_data->removed_callbacks.size() == 0) {
      continue;
    }

    for (auto& removed_callback : meta_data->removed_callbacks) {
      meta_data->callbacks.erase(std::remove_if(meta_data->callbacks.begin(), meta_data->callbacks.end(),
                                                [&](const NativeLibraryMetaDataCallback& callback) {
                                                  return callback.callback_id == removed_callback;
                                                }),
                                 meta_data->callbacks.end());
    }

    meta_data->removed_callbacks.clear();

    if (meta_data->callbacks.empty() && meta_data->load_context != nullptr) {
      meta_data->load_context->promise_resolver->Resolve(JS_NULL);
      delete meta_data->load_context;
      meta_data->load_context = nullptr;
    } else {
      RunRustFutureTasks();
    }
  }
}

void ExecutingContext::RegisterNativeLibraryMetaData(NativeLibraryMetaData* meta_data) {
  native_library_meta_data_contaner_.push_back(meta_data);
}

void ExecutingContext::DrainPendingPromiseJobs() {
  // should executing pending promise jobs.
  JSContext* pctx;

  dart_isolate_context_->profiler()->StartTrackSteps("JS_ExecutePendingJob");

  int finished = JS_ExecutePendingJob(script_state_.runtime(), &pctx);

  dart_isolate_context_->profiler()->FinishTrackSteps();

  while (finished != 0) {
    dart_isolate_context_->profiler()->StartTrackSteps("JS_ExecutePendingJob");
    finished = JS_ExecutePendingJob(script_state_.runtime(), &pctx);
    dart_isolate_context_->profiler()->FinishTrackSteps();
    if (finished == -1) {
      break;
    }
  }

  // Throw error when promise are not handled.
  rejected_promises_.Process(this);
}

void ExecutingContext::DefineGlobalProperty(const char* prop, JSValue value) {
  JSAtom atom = JS_NewAtom(script_state_.ctx(), prop);
  JS_SetProperty(script_state_.ctx(), global_object_, atom, value);
  JS_FreeAtom(script_state_.ctx(), atom);
}

ExecutionContextData* ExecutingContext::contextData() {
  return &context_data_;
}

uint8_t* ExecutingContext::DumpByteCode(const char* code,
                                        uint32_t codeLength,
                                        const char* sourceURL,
                                        uint64_t* bytecodeLength) {
  dart_isolate_context_->profiler()->StartTrackSteps("JS_Eval");

  JSValue object =
      JS_Eval(script_state_.ctx(), code, codeLength, sourceURL, JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);

  dart_isolate_context_->profiler()->FinishTrackSteps();

  bool success = HandleException(&object);
  if (!success)
    return nullptr;

  dart_isolate_context_->profiler()->StartTrackSteps("JS_WriteObject");

  size_t len;
  uint8_t* bytes = JS_WriteObject(script_state_.ctx(), &len, object, JS_WRITE_OBJ_BYTECODE);
  *bytecodeLength = len;
  JS_FreeValue(script_state_.ctx(), object);

  dart_isolate_context_->profiler()->FinishTrackSteps();

  return bytes;
}

void ExecutingContext::DispatchGlobalErrorEvent(ExecutingContext* context, JSValueConst error) {
  ExceptionState exceptionState;

  auto error_init = ErrorEventInit::Create(context->ctx(), error, exceptionState);
  error_init->setError(Converter<IDLAny>::FromValue(context->ctx(), error, exceptionState));
  auto* error_event = ErrorEvent::Create(context, event_type_names::kerror, error_init, exceptionState);

  context->DispatchErrorEvent(error_event);
}

static void DispatchPromiseRejectionEvent(const AtomicString& event_type,
                                          ExecutingContext* context,
                                          JSValueConst promise,
                                          JSValueConst reason) {
  ExceptionState exception_state;

  auto event_init = PromiseRejectionEventInit::Create();
  event_init->setPromise(Converter<IDLAny>::FromValue(context->ctx(), promise, exception_state));
  event_init->setReason(Converter<IDLAny>::FromValue(context->ctx(), reason, exception_state));
  auto event = PromiseRejectionEvent::Create(context, event_type, event_init, exception_state);

  context->window()->dispatchEvent(event, exception_state);
  if (exception_state.HasException()) {
    context->ReportError(reason);
  }
}

void ExecutingContext::FlushUICommand(const BindingObject* self, uint32_t reason) {
  std::vector<NativeBindingObject*> deps;
  FlushUICommand(self, reason, deps);
}

void ExecutingContext::FlushUICommand(const webf::BindingObject* self,
                                      uint32_t reason,
                                      std::vector<NativeBindingObject*>& deps) {
  if (SyncUICommandBuffer(self, reason, deps)) {
    dartMethodPtr()->flushUICommand(is_dedicated_, context_id_, self->bindingObject());
  }
}

bool ExecutingContext::SyncUICommandBuffer(const BindingObject* self,
                                           uint32_t reason,
                                           std::vector<NativeBindingObject*>& deps) {
  if (!uiCommandBuffer()->empty()) {
    if (is_dedicated_) {
      bool should_swap_ui_commands = false;
      if (isUICommandReasonDependsOnElement(reason)) {
        bool element_mounted_on_dart = self->bindingObject()->invoke_bindings_methods_from_native != nullptr;
        bool is_deps_elements_mounted_on_dart = true;

        for (auto binding : deps) {
          if (binding->invoke_bindings_methods_from_native == nullptr) {
            is_deps_elements_mounted_on_dart = false;
          }
        }

        if (!element_mounted_on_dart || !is_deps_elements_mounted_on_dart) {
          should_swap_ui_commands = true;
        }
      }

      if (isUICommandReasonDependsOnLayout(reason) || isUICommandReasonDependsOnAll(reason)) {
        should_swap_ui_commands = true;
      }

      // Sync commands to dart when caller dependents on Element.
      if (should_swap_ui_commands) {
        ui_command_buffer_.SyncToActive();
      }
    }
    return true;
  }

  return false;
}

void ExecutingContext::TurnOnJavaScriptGC() {
  JS_TurnOnGC(script_state_.runtime());
}

void ExecutingContext::TurnOffJavaScriptGC() {
  JS_TurnOffGC(script_state_.runtime());
}

void ExecutingContext::DispatchErrorEvent(ErrorEvent* error_event) {
  if (in_dispatch_error_event_) {
    return;
  }

  DispatchErrorEventInterval(error_event);
  ReportErrorEvent(error_event);
}

void ExecutingContext::DispatchErrorEventInterval(ErrorEvent* error_event) {
  assert(!in_dispatch_error_event_);
  in_dispatch_error_event_ = true;
  ExceptionState exception_state;
  window_->dispatchEvent(error_event, exception_state);
  in_dispatch_error_event_ = false;

  if (exception_state.HasException()) {
    JSValue error = JS_GetException(ctx());
    ReportError(error);
    JS_FreeValue(ctx(), error);
  }
}

void ExecutingContext::ReportErrorEvent(ErrorEvent* error_event) {
  ReportError(error_event->error().QJSValue());
}

void ExecutingContext::DispatchGlobalUnhandledRejectionEvent(ExecutingContext* context,
                                                             JSValueConst promise,
                                                             JSValueConst reason) {
  // Trigger unhandledRejection event.
  DispatchPromiseRejectionEvent(event_type_names::kunhandledrejection, context, promise, reason);
}

void ExecutingContext::DispatchGlobalRejectionHandledEvent(ExecutingContext* context, JSValue promise, JSValue error) {
  // Trigger rejectionhandled event.
  DispatchPromiseRejectionEvent(event_type_names::krejectionhandled, context, promise, error);
}

std::unordered_map<std::string, NativeByteCode> ExecutingContext::plugin_byte_code{};
std::unordered_map<std::string, std::string> ExecutingContext::plugin_string_code{};

void ExecutingContext::promiseRejectTracker(JSContext* ctx,
                                            JSValue promise,
                                            JSValue reason,
                                            int is_handled,
                                            void* opaque) {
  auto* context = static_cast<ExecutingContext*>(JS_GetContextOpaque(ctx));
  // The unhandledrejection event is the promise-equivalent of the global error event, which is fired for uncaught
  // exceptions. Because a rejected promise could be handled after the fact, by attaching catch(onRejected) or
  // then(onFulfilled, onRejected) to it, the additional rejectionhandled event is needed to indicate that a promise
  // which was previously rejected should no longer be considered unhandled.
  if (is_handled) {
    context->rejected_promises_.TrackHandledPromiseRejection(context, promise, reason);
  } else {
    context->rejected_promises_.TrackUnhandledPromiseRejection(context, promise, reason);
  }
}

DOMTimerCoordinator* ExecutingContext::Timers() {
  return &timers_;
}

ModuleListenerContainer* ExecutingContext::ModuleListeners() {
  return &module_listener_container_;
}

ModuleContextCoordinator* ExecutingContext::ModuleContexts() {
  return &module_contexts_;
}

void ExecutingContext::SetMutationScope(MemberMutationScope& mutation_scope) {
  // MemberMutationScope may be called by other MemberMutationScope in the call stack.
  // Should save the tree corresponding to the call stack.
  if (active_mutation_scope != nullptr) {
    mutation_scope.SetParent(active_mutation_scope);
  }
  active_mutation_scope = &mutation_scope;
}

void ExecutingContext::ClearMutationScope() {
  active_mutation_scope = active_mutation_scope->Parent();
}

void ExecutingContext::InstallDocument() {
  MemberMutationScope scope{this};
  document_ = MakeGarbageCollected<Document>(this);
  DefineGlobalProperty("document", document_->ToQuickJS());
}

void ExecutingContext::InstallPerformance() {
  MemberMutationScope scope{this};
  performance_ = MakeGarbageCollected<Performance>(this);
  DefineGlobalProperty("performance", performance_->ToQuickJS());
}

void ExecutingContext::InstallNativeLoader() {
  MemberMutationScope scope{this};
  native_loader_ = MakeGarbageCollected<NativeLoader>(this);
  DefineGlobalProperty("nativeLoader", native_loader_->ToQuickJS());
}

void ExecutingContext::InstallGlobal() {
  MemberMutationScope mutation_scope{this};
  window_ = MakeGarbageCollected<Window>(this);
  JS_SetPrototype(ctx(), Global(), window_->ToQuickJSUnsafe());
  JS_SetOpaque(Global(), window_);
}

void ExecutingContext::RegisterActiveScriptWrappers(ScriptWrappable* script_wrappable) {
  active_wrappers_.emplace(script_wrappable);
}

void ExecutingContext::InActiveScriptWrappers(ScriptWrappable* script_wrappable) {
  active_wrappers_.erase(script_wrappable);
}

// A lock free context validator.
bool isContextValid(double contextId) {
  if (contextId > running_context_list)
    return false;
  if (valid_contexts.count(contextId) == 0)
    return false;
  return valid_contexts[contextId];
}

}  // namespace webf
