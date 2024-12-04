/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include <atomic>
#include <unordered_map>

#if WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/atomic_string.h"
#include "bindings/qjs/binding_initializer.h"
#elif WEBF_V8_JS_ENGINE
#endif

#include "core/dart_methods.h"
//#include "core/dom/document.h"
//#include "core/frame/window.h"
//#include "core/html/html_html_element.h"
//#include "core/html/parser/html_parser.h"
//#include "event_factory.h"
#include "foundation/logging.h"
#include "foundation/native_value_converter.h"
#include "page.h"
#include "polyfill.h"

namespace webf {

ConsoleMessageHandler WebFPage::consoleMessageHandler{nullptr};

WebFPage::WebFPage(DartIsolateContext* dart_isolate_context,
                   bool is_dedicated,
                   size_t sync_buffer_size,
                   double context_id,
                   const JSExceptionHandler& handler)
    : ownerThreadId(std::this_thread::get_id()), dart_isolate_context_(dart_isolate_context) {
  context_ = new ExecutingContext(
      dart_isolate_context, is_dedicated, sync_buffer_size, context_id,
      [](ExecutingContext* context, const char* message) {
        WEBF_LOG(ERROR) << message << std::endl;
        if (context->IsContextValid()) {
          context->dartMethodPtr()->onJSError(context->isDedicated(), context->contextId(), message);
        }
      },
      this);
}

bool WebFPage::parseHTML(const char* code, size_t length) {
  if (!context_->IsContextValid())
    return false;

  {
//    MemberMutationScope scope{context_};

//    auto document_element = context_->document()->documentElement();
//    if (!document_element) {
//      return false;
//    }

//    context_->dartIsolateContext()->profiler()->StartTrackSteps("HTMLParser::parseHTML");
//    HTMLParser::parseHTML(code, length, context_->document()->documentElement());
//    context_->dartIsolateContext()->profiler()->FinishTrackSteps();
  }

  context_->uiCommandBuffer()->AddCommand(UICommand::kFinishRecordingCommand, nullptr, nullptr, nullptr);

  return true;
}

NativeValue* WebFPage::invokeModuleEvent(SharedNativeString* native_module_name,
                                         const char* eventType,
                                         void* ptr,
                                         NativeValue* extra) {
  return nullptr;

//  if (!context_->IsContextValid())
//    return nullptr;
//
//  MemberMutationScope scope{context_};
//
//  JSContext* ctx = context_->ctx();
//  Event* event = nullptr;
//  if (ptr != nullptr) {
//    std::string type = std::string(eventType);
//    auto* raw_event = static_cast<RawEvent*>(ptr);
//    event = EventFactory::Create(context_, AtomicString(ctx, type), raw_event);
//    delete raw_event;
//  }
//
//  ScriptValue extraObject = ScriptValue(ctx, const_cast<const NativeValue&>(*extra));
//  AtomicString module_name = AtomicString(
//      ctx, std::unique_ptr<AutoFreeNativeString>(reinterpret_cast<AutoFreeNativeString*>(native_module_name)));
//  auto listener = context_->ModuleListeners()->listener(module_name);
//
//  if (listener == nullptr) {
//    return nullptr;
//  }
//
//  ScriptValue arguments[] = {event != nullptr ? event->ToValue() : ScriptValue::Empty(ctx), extraObject};
//  ScriptValue result = listener->value()->Invoke(ctx, ScriptValue::Empty(ctx), 2, arguments);
//  if (result.IsException()) {
//    context_->HandleException(&result);
//    return nullptr;
//  }
//
//  ExceptionState exception_state;
//  auto* return_value = static_cast<NativeValue*>(malloc(sizeof(NativeValue)));
//  NativeValue tmp = result.ToNative(ctx, exception_state);
//  if (exception_state.HasException()) {
//    context_->HandleException(exception_state);
//    return nullptr;
//  }
//
//  memcpy(return_value, &tmp, sizeof(NativeValue));
//  return return_value;
}

bool WebFPage::evaluateScript(const char* script,
                              uint64_t script_len,
                              uint8_t** parsed_bytecodes,
                              uint64_t* bytecode_len,
                              const char* url,
                              int startLine) {
  if (!context_->IsContextValid())
    return false;

  return false;
//  return context_->EvaluateJavaScript(script, script_len, parsed_bytecodes, bytecode_len, url, startLine);
}

void WebFPage::evaluateScript(const char* script, size_t length, const char* url, int startLine) {
  if (!context_->IsContextValid())
    return;

  return;
//  context_->EvaluateJavaScript(script, length, url, startLine);
}

uint8_t* WebFPage::dumpByteCode(const char* script, size_t length, const char* url, uint64_t* byteLength) {
  if (!context_->IsContextValid())
    return nullptr;
  return context_->DumpByteCode(script, static_cast<uint32_t>(length), url, byteLength);
}

bool WebFPage::evaluateByteCode(uint8_t* bytes, size_t byteLength) {
  if (!context_->IsContextValid())
    return false;

  return false;
//  return context_->EvaluateByteCode(bytes, byteLength);
}

std::thread::id WebFPage::currentThread() const {
  return ownerThreadId;
}

WebFPage::~WebFPage() {
#if IS_TEST
  if (disposeCallback != nullptr) {
    disposeCallback(this);
  }
#endif
  delete context_;
}

void WebFPage::reportError(const char* errmsg) {
  handler_(context_, errmsg);
}

static void ReturnEvaluateScriptsInternal(Dart_PersistentHandle persistent_handle,
                                          EvaluateQuickjsByteCodeCallback result_callback,
                                          bool is_success) {
  Dart_Handle handle = Dart_HandleFromPersistent_DL(persistent_handle);
  result_callback(handle, is_success ? 1 : 0);
  Dart_DeletePersistentHandle_DL(persistent_handle);
}

void WebFPage::EvaluateScriptsInternal(void* page_,
                                       const char* code,
                                       uint64_t code_len,
                                       uint8_t** parsed_bytecodes,
                                       uint64_t* bytecode_len,
                                       const char* bundleFilename,
                                       int32_t startLine,
                                       int64_t profile_id,
                                       Dart_Handle persistent_handle,
                                       EvaluateScriptsCallback result_callback) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());

  page->dartIsolateContext()->profiler()->StartTrackEvaluation(profile_id);

  bool is_success = page->evaluateScript(code, code_len, parsed_bytecodes, bytecode_len, bundleFilename, startLine);

  page->dartIsolateContext()->profiler()->FinishTrackEvaluation(profile_id);

  page->dartIsolateContext()->dispatcher()->PostToDart(page->isDedicated(), ReturnEvaluateScriptsInternal,
                                                       persistent_handle, result_callback, is_success);
}

static void ReturnEvaluateQuickjsByteCodeResultToDart(Dart_PersistentHandle persistent_handle,
                                                      EvaluateQuickjsByteCodeCallback result_callback,
                                                      bool is_success) {
  Dart_Handle handle = Dart_HandleFromPersistent_DL(persistent_handle);
  result_callback(handle, is_success ? 1 : 0);
  Dart_DeletePersistentHandle_DL(persistent_handle);
}

void WebFPage::EvaluateQuickjsByteCodeInternal(void* page_,
                                               uint8_t* bytes,
                                               int32_t byteLen,
                                               int64_t profile_id,
                                               Dart_PersistentHandle persistent_handle,
                                               EvaluateQuickjsByteCodeCallback result_callback) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());

  page->dartIsolateContext()->profiler()->StartTrackEvaluation(profile_id);

  bool is_success = page->evaluateByteCode(bytes, byteLen);

  page->dartIsolateContext()->profiler()->FinishTrackEvaluation(profile_id);

  page->dartIsolateContext()->dispatcher()->PostToDart(page->isDedicated(), ReturnEvaluateQuickjsByteCodeResultToDart,
                                                       persistent_handle, result_callback, is_success);
}

static void ReturnParseHTMLToDart(Dart_PersistentHandle persistent_handle, ParseHTMLCallback result_callback) {
  Dart_Handle handle = Dart_HandleFromPersistent_DL(persistent_handle);
  result_callback(handle);
  Dart_DeletePersistentHandle_DL(persistent_handle);
}

void WebFPage::ParseHTMLInternal(void* page_,
                                 char* code,
                                 int32_t length,
                                 int64_t profile_id,
                                 Dart_PersistentHandle dart_handle,
                                 ParseHTMLCallback result_callback) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());

  page->dartIsolateContext()->profiler()->StartTrackEvaluation(profile_id);

  page->parseHTML(code, length);
  dart_free(code);

  page->dartIsolateContext()->profiler()->FinishTrackEvaluation(profile_id);

  page->dartIsolateContext()->dispatcher()->PostToDart(page->isDedicated(), ReturnParseHTMLToDart, dart_handle,
                                                       result_callback);
}

static void ReturnInvokeEventResultToDart(Dart_Handle persistent_handle,
                                          InvokeModuleEventCallback result_callback,
                                          webf::NativeValue* result) {
  Dart_Handle handle = Dart_HandleFromPersistent_DL(persistent_handle);
  result_callback(handle, result);
  Dart_DeletePersistentHandle_DL(persistent_handle);
}

void WebFPage::InvokeModuleEventInternal(void* page_,
                                         void* module_name,
                                         const char* eventType,
                                         void* event,
                                         void* extra,
                                         Dart_Handle persistent_handle,
                                         InvokeModuleEventCallback result_callback) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  auto dart_isolate_context = page->executingContext()->dartIsolateContext();
  assert(std::this_thread::get_id() == page->currentThread());

  page->dartIsolateContext()->profiler()->StartTrackAsyncEvaluation();

  auto* result = page->invokeModuleEvent(reinterpret_cast<webf::SharedNativeString*>(module_name), eventType, event,
                                         reinterpret_cast<webf::NativeValue*>(extra));

  page->dartIsolateContext()->profiler()->FinishTrackAsyncEvaluation();

  dart_isolate_context->dispatcher()->PostToDart(page->isDedicated(), ReturnInvokeEventResultToDart, persistent_handle,
                                                 result_callback, result);
}

static void ReturnDumpByteCodeResultToDart(Dart_Handle persistent_handle, DumpQuickjsByteCodeCallback result_callback) {
  Dart_Handle handle = Dart_HandleFromPersistent_DL(persistent_handle);
  result_callback(handle);
  Dart_DeletePersistentHandle_DL(persistent_handle);
}

void WebFPage::DumpQuickJsByteCodeInternal(void* page_,
                                           int64_t profile_id,
                                           const char* code,
                                           int32_t code_len,
                                           uint8_t** parsed_bytecodes,
                                           uint64_t* bytecode_len,
                                           const char* url,
                                           Dart_PersistentHandle persistent_handle,
                                           DumpQuickjsByteCodeCallback result_callback) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  auto dart_isolate_context = page->executingContext()->dartIsolateContext();

  dart_isolate_context->profiler()->StartTrackEvaluation(profile_id);

  assert(std::this_thread::get_id() == page->currentThread());
  uint8_t* bytes = page->dumpByteCode(code, code_len, url, bytecode_len);
  *parsed_bytecodes = bytes;

  dart_isolate_context->profiler()->FinishTrackEvaluation(profile_id);

  dart_isolate_context->dispatcher()->PostToDart(page->isDedicated(), ReturnDumpByteCodeResultToDart, persistent_handle,
                                                 result_callback);
}

}  // namespace webf
