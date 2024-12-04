/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_JS_CONTEXT_H
#define BRIDGE_JS_CONTEXT_H

#if WEBF_V8_JS_ENGINE
#include <v8/v8.h>
#elif WEBF_QUICKJS_JS_ENGINE
#include <quickjs/list.h>
#include <quickjs/quickjs.h>
#endif

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstring>
#include <locale>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#if WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/binding_initializer.h"
#include "bindings/qjs/rejected_promises.h"
#include "bindings/qjs/script_value.h"
#elif WEBF_V8_JS_ENGINE
#endif
#include "foundation/macros.h"
#include "foundation/ui_command_buffer.h"
#include "native/native_loader.h"
#include "plugin_api/executing_context.h"

#include "dart_isolate_context.h"
#include "dart_methods.h"
#include "executing_context_data.h"
#include "frame/dom_timer_coordinator.h"
//#include "frame/module_context_coordinator.h"
//#include "frame/module_listener_container.h"
#include "script_state.h"

#include "shared_ui_command.h"

namespace webf {

struct NativeByteCode {
  uint8_t* bytes;
  int32_t length;
};

class ExecutingContext;
class Document;
class Window;
class Performance;
class MemberMutationScope;
class ErrorEvent;
class DartContext;
class MutationObserver;
class BindingObject;
struct NativeBindingObject;
class ScriptWrappable;

using JSExceptionHandler = std::function<void(ExecutingContext* context, const char* message)>;
using MicrotaskCallback = void (*)(void* data);

bool isContextValid(double contextId);

// An environment in which script can execute. This class exposes the common
// properties of script execution environments on the webf.
// Window : Document : ExecutionContext = 1 : 1 : 1 at any point in time.
class ExecutingContext {
 public:
  ExecutingContext() = delete;
  ExecutingContext(DartIsolateContext* dart_isolate_context,
                   bool is_dedicated,
                   size_t sync_buffer_size,
                   double context_id,
                   JSExceptionHandler handler,
                   void* owner);
  ~ExecutingContext();

  bool EvaluateJavaScript(const char* code,
                          size_t codeLength,
                          uint8_t** parsed_bytecodes,
                          uint64_t* bytecode_len,
                          const char* sourceURL,
                          int startLine);
  bool EvaluateJavaScript(const char16_t* code, size_t length, const char* sourceURL, int startLine);
  bool EvaluateJavaScript(const char* code, size_t codeLength, const char* sourceURL, int startLine);
  bool EvaluateByteCode(uint8_t* bytes, size_t byteLength);
  bool IsContextValid() const;
  void SetContextInValid();
  bool IsCtxValid() const;
  FORCE_INLINE double contextId() const { return context_id_; };
  FORCE_INLINE int32_t uniqueId() const { return unique_id_; }
  void* owner();
  bool HandleException(ScriptValue* exc);
  bool HandleException(ExceptionState& exception_state);
  bool HandleException(ExceptionState& exception_state, char** rust_error_msg, uint32_t* rust_errmsg_len);
  void ReportError(JSValueConst error);
  void ReportError(JSValueConst error, char** rust_errmsg, uint32_t* rust_errmsg_length);
  void DrainMicrotasks();
  void EnqueueMicrotask(MicrotaskCallback callback, void* data = nullptr);
  ExecutionContextData* contextData();
  uint8_t* DumpByteCode(const char* code, uint32_t codeLength, const char* sourceURL, uint64_t* bytecodeLength);

#if WEBF_QUICKJS_JS_ENGINE
  static ExecutingContext* From(JSContext* ctx);
  JSValue Global();
  JSContext* ctx();
  bool HandleException(JSValue* exc);
  void ReportError(JSValueConst error);
  void DefineGlobalProperty(const char* prop, JSValueConst value);
  static void DispatchGlobalUnhandledRejectionEvent(ExecutingContext* context,
                                                    JSValueConst promise,
                                                    JSValueConst error);
  static void DispatchGlobalRejectionHandledEvent(ExecutingContext* context, JSValueConst promise, JSValueConst error);
  static void DispatchGlobalErrorEvent(ExecutingContext* context, JSValueConst error);
  static void promiseRejectTracker(JSContext* ctx,
                                   JSValueConst promise,
                                   JSValueConst reason,
                                   JS_BOOL is_handled,
                                   void* opaque);
#elif WEBF_V8_JS_ENGINE
  static ExecutingContext* From(v8::Isolate* isolate);
  v8::Local<v8::Value> Global();
  v8::Isolate* ctx();
  bool HandleException(v8::Local<v8::Value> exc);
  void ReportError(v8::Local<v8::Value> error);
  void DefineGlobalProperty(const char* prop, v8::Local<v8::Value> value);
#endif

  // Make global object inherit from WindowProperties.
  void InstallGlobal();

  // Register active script wrappers.
  void RegisterActiveScriptWrappers(ScriptWrappable* script_wrappable);
  void InActiveScriptWrappers(ScriptWrappable* script_wrappable);

  // Gets the DOMTimerCoordinator which maintains the "active timer
  // list" of tasks created by setTimeout and setInterval. The
  // DOMTimerCoordinator is owned by the ExecutionContext and should
  // not be used after the ExecutionContext is destroyed.
  DOMTimerCoordinator* Timers();

  // Gets the ModuleListeners which registered by `webf.addModuleListener API`.
//  ModuleListenerContainer* ModuleListeners();

  // Gets the ModuleCallbacks which from the 4th parameter of `webf.invokeModule` function.
//  ModuleContextCoordinator* ModuleContexts();

  // Get current script state.
  ScriptState* GetScriptState() { return &script_state_; }

  void SetMutationScope(MemberMutationScope& mutation_scope);
  bool HasMutationScope() const { return active_mutation_scope != nullptr; }
  MemberMutationScope* mutationScope() const { return active_mutation_scope; }
  void ClearMutationScope();

  FORCE_INLINE Document* document() const { return document_; };
  FORCE_INLINE Window* window() const { return window_; }
  FORCE_INLINE DartIsolateContext* dartIsolateContext() const { return dart_isolate_context_; };
  FORCE_INLINE Performance* performance() const { return performance_; }
  FORCE_INLINE SharedUICommand* uiCommandBuffer() { return &ui_command_buffer_; };
  FORCE_INLINE DartMethodPointer* dartMethodPtr() const {
    assert(dart_isolate_context_->valid());
    return dart_isolate_context_->dartMethodPtr();
  }
  FORCE_INLINE WebFValueStatus* status() const { return executing_context_status_; }
  FORCE_INLINE ExecutingContextWebFMethods* publicMethodPtr() const { return public_method_ptr_.get(); }
  FORCE_INLINE bool isDedicated() { return is_dedicated_; }
  FORCE_INLINE std::chrono::time_point<std::chrono::system_clock> timeOrigin() const { return time_origin_; }

  // Force dart side to execute the pending ui commands.
  void FlushUICommand(const BindingObject* self, uint32_t reason);
  void FlushUICommand(const BindingObject* self, uint32_t reason, std::vector<NativeBindingObject*>& deps);

  // Sync pending ui commands and make it accessible to Dart
  bool SyncUICommandBuffer(const BindingObject* self, uint32_t reason, std::vector<NativeBindingObject*>& deps);

  void TurnOnJavaScriptGC();
  void TurnOffJavaScriptGC();

  void DispatchErrorEvent(ErrorEvent* error_event);
  void DispatchErrorEventInterval(ErrorEvent* error_event);
  void ReportErrorEvent(ErrorEvent* error_event);

  // Bytecodes which registered by webf plugins.
  static std::unordered_map<std::string, NativeByteCode> plugin_byte_code;
  // Raw string codes which registered by webf plugins.
  static std::unordered_map<std::string, std::string> plugin_string_code;

 private:
  std::chrono::time_point<std::chrono::system_clock> time_origin_;
  int32_t unique_id_;

  void InstallDocument();
  void InstallPerformance();
  void InstallNativeLoader();

  void DrainPendingPromiseJobs();
  static void promiseRejectTracker(JSContext* ctx,
                                   JSValueConst promise,
                                   JSValueConst reason,
                                   JS_BOOL is_handled,
                                   void* opaque);
  // Warning: Don't change the orders of members in ExecutingContext if you really know what are you doing.
  // From C++ standard, https://isocpp.org/wiki/faq/dtors#order-dtors-for-members
  // Members first initialized and destructed at the last.
  // Keep uiCommandBuffer below dartMethod ptr to make sure we can flush all disposeEventTarget when UICommandBuffer
  // release.
  SharedUICommand ui_command_buffer_{this};
  DartIsolateContext* dart_isolate_context_{nullptr};
  // Keep uiCommandBuffer above ScriptState to make sure we can collect all disposedEventTarget command when free
  // JSContext. When call JSFreeContext(ctx) inside ScriptState, all eventTargets will be finalized and UICommandBuffer
  // will be fill up to UICommand::disposeEventTarget commands.
  // ----------------------------------------------------------------------
  // All members above ScriptState will be freed after ScriptState freed
  // ----------------------------------------------------------------------
#if WEBF_QUICKJS_JS_ENGINE
  ScriptState script_state_{dart_isolate_context_};
#elif WEBF_V8_JS_ENGINE
  ScriptState script_state_{dart_isolate_context_, v8::Context::New(dart_isolate_context_->isolate())};
#endif
  // ----------------------------------------------------------------------
  // All members below will be free before ScriptState freed.
  // ----------------------------------------------------------------------
  std::atomic<bool> is_context_valid_{false};
  double context_id_;
  JSExceptionHandler dart_error_report_handler_;
  void* owner_;
#if WEBF_QUICKJS_JS_ENGINE
  JSValue global_object_{JS_NULL};
#elif WEBF_V8_JS_ENGINE
  v8::Local<v8::Value> global_object_;
#endif
  Document* document_{nullptr};
  Window* window_{nullptr};
  NativeLoader* native_loader_{nullptr};
  Performance* performance_{nullptr};
  DOMTimerCoordinator timers_;
//  ModuleListenerContainer module_listener_container_;
//  ModuleContextCoordinator module_contexts_;
  ExecutionContextData context_data_{this};
  bool in_dispatch_error_event_{false};
#if WEBF_QUICKJS_JS_ENGINE
  RejectedPromises rejected_promises_;
#endif
  MemberMutationScope* active_mutation_scope{nullptr};
  std::unordered_set<ScriptWrappable*> active_wrappers_;
  WebFValueStatus* executing_context_status_{new WebFValueStatus()};
  bool is_dedicated_;

  // Rust methods ptr should keep alive when ExecutingContext is disposing.
  const std::unique_ptr<ExecutingContextWebFMethods> public_method_ptr_ = nullptr;
};

}  // namespace webf

#endif  // BRIDGE_JS_CONTEXT_H
