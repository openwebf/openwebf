/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "webf_test_context_v8.h"
//#include "bindings/qjs/member_installer.h"
//#include "bindings/qjs/qjs_interface_bridge.h"
//#include "core/dom/document.h"
//#include "core/fileapi/blob.h"
//#include "core/frame/window.h"
//#include "core/html/html_body_element.h"
//#include "core/html/html_html_element.h"
//#include "core/html/parser/html_parser.h"
//#include "qjs_blob.h"
#include "testframework.h"

namespace webf {

struct ExecuteCallbackContext {
  ExecuteCallbackContext() = delete;

  explicit ExecuteCallbackContext(ExecutingContext* context,
                                  ExecuteResultCallback executeCallback,
                                  WebFTestContext* webf_context,
                                  Dart_PersistentHandle persistent_handle)
      : executeCallback(executeCallback),
        context(context),
        webf_context(webf_context),
        persistent_handle(persistent_handle){};
  ExecuteResultCallback executeCallback;
  ExecutingContext* context;
  WebFTestContext* webf_context;
  Dart_PersistentHandle persistent_handle;
};

void WebFTestContext::invokeExecuteTest(Dart_PersistentHandle persistent_handle,
                                        ExecuteResultCallback executeCallback) {
//  if (execute_test_callback_ == nullptr) {
//    return;
//  }
//
//  auto done = [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic,
//                 JSValue* func_data) -> JSValue {
//    JSValue& statusValue = argv[0];
//    JSValue proxyObject = func_data[0];
//    auto* callbackContext = static_cast<ExecuteCallbackContext*>(JS_GetOpaque(proxyObject, 1));
//
//    if (!JS_IsString(statusValue)) {
//      return JS_ThrowTypeError(ctx, "failed to execute 'done': parameter 1 (status) is not a string");
//    }
//
//    WEBF_LOG(VERBOSE) << "Done..";
//
//    std::unique_ptr<SharedNativeString> status = webf::jsValueToNativeString(ctx, statusValue);
//
//    callbackContext->context->dartIsolateContext()->dispatcher()->PostToDart(
//        callbackContext->context->isDedicated(),
//        [](ExecuteCallbackContext* callback_context, SharedNativeString* status) {
//          Dart_Handle handle = Dart_HandleFromPersistent_DL(callback_context->persistent_handle);
//          callback_context->executeCallback(handle, status);
//          Dart_DeletePersistentHandle_DL(callback_context->persistent_handle);
//          callback_context->webf_context->execute_test_proxy_object_ = JS_NULL;
//        },
//        callbackContext, status.release());
//    JS_FreeValue(ctx, proxyObject);
//    return JS_NULL;
//  };
//
//  auto* callbackContext = new ExecuteCallbackContext(context_, executeCallback, this, persistent_handle);
//  execute_test_proxy_object_ = JS_NewObject(context_->ctx());
//  JS_SetOpaque(execute_test_proxy_object_, callbackContext);
//  JSValue callbackData[]{execute_test_proxy_object_};
//  JSValue callback = JS_NewCFunctionData(context_->ctx(), done, 0, 0, 1, callbackData);
//
//  ScriptValue arguments[] = {ScriptValue(context_->ctx(), callback)};
//  ScriptValue result =
//      execute_test_callback_->Invoke(context_->ctx(), ScriptValue::Empty(context_->ctx()), 1, arguments);
//  context_->HandleException(&result);
//  context_->DrainMicrotasks();
//  JS_FreeValue(context_->ctx(), callback);
//  execute_test_callback_ = nullptr;
}

WebFTestContext::WebFTestContext(ExecutingContext* context)
    : context_(context), page_(static_cast<WebFPage*>(context->owner())) {
//  context->dartIsolateContext()->profiler()->StartTrackInitialize();

//  page_->owner = this;
//  page_->disposeCallback = [](WebFPage* bridge) { delete static_cast<WebFTestContext*>(bridge->owner); };

//  std::initializer_list<MemberInstaller::FunctionConfig> functionConfig{
//      {"__webf_execute_test__", executeTest, 1},
//      {"__webf_match_image_snapshot__", matchImageSnapshot, 3},
//      {"__webf_environment__", environment, 0},
//      {"__webf_simulate_pointer__", simulatePointer, 3},
//      {"__webf_simulate_inputtext__", simulateInputText, 1},
//      {"__webf_trigger_global_error__", triggerGlobalError, 0},
//      {"__webf_parse_html__", parseHTML, 1},
//  };

//  MemberInstaller::InstallFunctions(context, context->Global(), functionConfig);
//  initWebFTestFramework(context);
//
//  context->dartIsolateContext()->profiler()->FinishTrackInitialize();
}

WebFTestContext::~WebFTestContext() {
//  JS_FreeValue(context_->ctx(), execute_test_proxy_object_);
}

bool WebFTestContext::parseTestHTML(const uint16_t* code, size_t codeLength) {
  if (!context_->IsContextValid())
    return false;
  std::string utf8Code = toUTF8(std::u16string(reinterpret_cast<const char16_t*>(code), codeLength));
  return page_->parseHTML(utf8Code.c_str(), utf8Code.length());
}

void WebFTestContext::registerTestEnvDartMethods(uint64_t* methodBytes, int32_t length) {
  size_t i = 0;

  auto dartMethodPtr = context_->dartMethodPtr();

//  dartMethodPtr->SetOnJSError(reinterpret_cast<OnJSError>(methodBytes[i++]));
//  dartMethodPtr->SetMatchImageSnapshot(reinterpret_cast<MatchImageSnapshot>(methodBytes[i++]));
//  dartMethodPtr->SetMatchImageSnapshotBytes(reinterpret_cast<MatchImageSnapshotBytes>(methodBytes[i++]));
//  dartMethodPtr->SetEnvironment(reinterpret_cast<Environment>(methodBytes[i++]));
//  dartMethodPtr->SetSimulatePointer(reinterpret_cast<SimulatePointer>(methodBytes[i++]));
//  dartMethodPtr->SetSimulateInputText(reinterpret_cast<SimulateInputText>(methodBytes[i++]));

  assert_m(i == length, "Dart native methods count is not equal with C++ side method registrations.");
}

}  // namespace webf
