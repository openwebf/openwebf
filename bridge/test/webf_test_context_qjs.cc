/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "webf_test_context_qjs.h"
#include "bindings/qjs/member_installer.h"
#include "bindings/qjs/qjs_interface_bridge.h"
#include "core/dom/document.h"
#include "core/fileapi/blob.h"
#include "core/frame/window.h"
#include "core/html/html_body_element.h"
#include "core/html/html_html_element.h"
#include "core/html/parser/html_parser.h"
#include "qjs_blob.h"
#include "testframework.h"

namespace webf {

static JSValue executeTest(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSValue& callback = argv[0];
  auto context = static_cast<ExecutingContext*>(JS_GetContextOpaque(ctx));
  if (!JS_IsObject(callback)) {
    return JS_ThrowTypeError(ctx, "Failed to execute 'executeTest': parameter 1 (callback) is not an function.");
  }

  if (!JS_IsFunction(ctx, callback)) {
    return JS_ThrowTypeError(ctx, "Failed to execute 'executeTest': parameter 1 (callback) is not an function.");
  }
  auto bridge = static_cast<WebFPage*>(context->owner());
  auto bridgeTest = static_cast<WebFTestContext*>(bridge->owner);
  bridgeTest->execute_test_callback_ = QJSFunction::Create(ctx, callback);
  return JS_NULL;
}

static JSValue matchImageSnapshot(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSValue& blobValue = argv[0];
  JSValue& screenShotValue = argv[1];
  JSValue& callbackValue = argv[2];
  auto* context = static_cast<ExecutingContext*>(JS_GetContextOpaque(ctx));
  auto* blob = toScriptWrappable<Blob>(blobValue);

  if (blob == nullptr) {
    return JS_ThrowTypeError(
        ctx, "Failed to execute '__webf_match_image_snapshot__': parameter 1 (blob) must be an Blob object.");
  }

  if (!JS_IsString(screenShotValue) && !QJSBlob::HasInstance(context, screenShotValue)) {
    return JS_ThrowTypeError(
        ctx, "Failed to execute '__webf_match_image_snapshot__': parameter 2 (match) must be an string or blob.");
  }

  if (!JS_IsObject(callbackValue)) {
    return JS_ThrowTypeError(
        ctx, "Failed to execute '__webf_match_image_snapshot__': parameter 3 (callback) is not an function.");
  }

  if (!JS_IsFunction(ctx, callbackValue)) {
    return JS_ThrowTypeError(
        ctx, "Failed to execute '__webf_match_image_snapshot__': parameter 3 (callback) is not an function.");
  }

  auto* callbackContext = new ImageSnapShotContext{JS_DupValue(ctx, callbackValue), context};

  context->FlushUICommand(context->window(), FlushUICommandReason::kDependentsAll);

  auto fn = [](void* ptr, double contextId, int8_t result, char* errmsg) {
    auto* callback_context = static_cast<ImageSnapShotContext*>(ptr);
    auto* context = callback_context->context;

    callback_context->context->dartIsolateContext()->dispatcher()->PostToJs(
        context->isDedicated(), context->contextId(),
        [](ImageSnapShotContext* callback_context, int8_t result, char* errmsg) {
          JSContext* ctx = callback_context->context->ctx();

          callback_context->context->dartIsolateContext()->profiler()->StartTrackAsyncEvaluation();

          if (errmsg == nullptr) {
            JSValue arguments[] = {JS_NewBool(ctx, result != 0), JS_NULL};
            JSValue returnValue =
                JS_Call(ctx, callback_context->callback, callback_context->context->Global(), 1, arguments);
            callback_context->context->HandleException(&returnValue);
          } else {
            JSValue errmsgValue = JS_NewString(ctx, errmsg);
            JSValue arguments[] = {JS_NewBool(ctx, false), errmsgValue};
            JSValue returnValue =
                JS_Call(ctx, callback_context->callback, callback_context->context->Global(), 2, arguments);
            callback_context->context->HandleException(&returnValue);
            JS_FreeValue(ctx, errmsgValue);
            dart_free(errmsg);
          }

          callback_context->context->DrainMicrotasks();
          JS_FreeValue(callback_context->context->ctx(), callback_context->callback);

          callback_context->context->dartIsolateContext()->profiler()->FinishTrackAsyncEvaluation();

          delete callback_context;
        },
        callback_context, result, errmsg);
  };

  if (QJSBlob::HasInstance(context, screenShotValue)) {
    auto* expectedBlob = toScriptWrappable<Blob>(screenShotValue);
    context->dartMethodPtr()->matchImageSnapshotBytes(context->isDedicated(), callbackContext, context->contextId(),
                                                      blob->bytes(), blob->size(), expectedBlob->bytes(),
                                                      expectedBlob->size(), fn);
  } else {
    std::unique_ptr<SharedNativeString> screenShotNativeString = webf::jsValueToNativeString(ctx, screenShotValue);

    context->dartMethodPtr()->matchImageSnapshot(context->isDedicated(), callbackContext, context->contextId(),
                                                 blob->bytes(), blob->size(), screenShotNativeString.release(), fn);
  }

  return JS_NULL;
}

static JSValue environment(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  auto* context = ExecutingContext::From(ctx);
#if FLUTTER_BACKEND
  const char* env = context->dartMethodPtr()->environment(context->isDedicated(), context->contextId());
  return JS_ParseJSON(ctx, env, strlen(env), "");
#else
  return JS_NewObject(ctx);
#endif
}

struct SimulatePointerCallbackContext {
  ExecutingContext* context{nullptr};
  JSValue callbackValue{JS_NULL};
};

static void handleSimulatePointerCallback(void* p, double contextId, char* errmsg) {
  auto* simulate_context = static_cast<SimulatePointerCallbackContext*>(p);
  auto* context = simulate_context->context;
  context->dartIsolateContext()->dispatcher()->PostToJs(
      context->isDedicated(), context->contextId(),
      [](SimulatePointerCallbackContext* simulate_context, double contextId, char* errmsg) {
        simulate_context->context->dartIsolateContext()->profiler()->StartTrackAsyncEvaluation();

        JSValue return_value =
            JS_Call(simulate_context->context->ctx(), simulate_context->callbackValue, JS_NULL, 0, nullptr);
        JS_FreeValue(simulate_context->context->ctx(), return_value);
        JS_FreeValue(simulate_context->context->ctx(), simulate_context->callbackValue);
        simulate_context->context->DrainMicrotasks();
        simulate_context->context->dartIsolateContext()->profiler()->FinishTrackAsyncEvaluation();
        delete simulate_context;
      },
      simulate_context, contextId, errmsg);
}

static JSValue simulatePointer(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  auto* context = static_cast<ExecutingContext*>(JS_GetContextOpaque(ctx));
  JSValue inputArrayValue = argv[0];
  if (!JS_IsObject(inputArrayValue)) {
    return JS_ThrowTypeError(ctx, "Failed to execute '__webf_simulate_pointer__': first arguments should be an array.");
  }

  JSValue pointerValue = argv[1];
  if (!JS_IsNumber(pointerValue)) {
    return JS_ThrowTypeError(ctx,
                             "Failed to execute '__webf_simulate_pointer__': second arguments should be an number.");
  }
  JSValue callbackValue = argv[2];

  uint32_t length;
  JSValue lengthValue = JS_GetPropertyStr(ctx, inputArrayValue, "length");
  JS_ToUint32(ctx, &length, lengthValue);
  JS_FreeValue(ctx, lengthValue);

  auto* mousePointerList = new MousePointer[length];

  for (int i = 0; i < length; i++) {
    MousePointer* mouse = &mousePointerList[i];
    JSValue params = JS_GetPropertyUint32(ctx, inputArrayValue, i);
    JSValue paramsLengthValue = JS_GetPropertyStr(ctx, params, "length");
    uint32_t params_length;
    JS_ToUint32(ctx, &params_length, paramsLengthValue);
    mouse->context_id = context->contextId();
    JSValue xValue = JS_GetPropertyUint32(ctx, params, 0);
    JSValue yValue = JS_GetPropertyUint32(ctx, params, 1);
    JSValue changeValue = JS_GetPropertyUint32(ctx, params, 2);

    double x;
    double y;
    double change;
    double delta_x = 0.0;
    double delta_y = 0.0;
    int32_t signal_kind = 0;

    if (params_length > 2) {
      JSValue signalKindValue = JS_GetPropertyUint32(ctx, params, 3);
      JSValue delayXValue = JS_GetPropertyUint32(ctx, params, 4);
      JSValue delayYValue = JS_GetPropertyUint32(ctx, params, 5);

      JS_ToInt32(ctx, &signal_kind, signalKindValue);
      JS_ToFloat64(ctx, &delta_x, delayXValue);
      JS_ToFloat64(ctx, &delta_y, delayYValue);

      mouse->signal_kind = signal_kind;
      mouse->delta_x = delta_x;
      mouse->delta_y = delta_y;

      JS_FreeValue(ctx, signalKindValue);
      JS_FreeValue(ctx, delayXValue);
      JS_FreeValue(ctx, delayYValue);
    }

    JS_ToFloat64(ctx, &x, xValue);
    JS_ToFloat64(ctx, &y, yValue);
    JS_ToFloat64(ctx, &change, changeValue);

    mouse->x = x;
    mouse->y = y;
    mouse->change = change;

    JS_FreeValue(ctx, params);
    JS_FreeValue(ctx, xValue);
    JS_FreeValue(ctx, yValue);
    JS_FreeValue(ctx, changeValue);
    JS_FreeValue(ctx, paramsLengthValue);
  }

  uint32_t pointer;
  JS_ToUint32(ctx, &pointer, pointerValue);

  auto* simulate_context = new SimulatePointerCallbackContext();
  simulate_context->context = context;
  simulate_context->callbackValue = JS_DupValue(ctx, callbackValue);
  context->FlushUICommand(context->window(), FlushUICommandReason::kDependentsAll);

  context->dartMethodPtr()->simulatePointer(context->isDedicated(), simulate_context, mousePointerList, length, pointer,
                                            handleSimulatePointerCallback);

  return JS_NULL;
}

static JSValue simulateInputText(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  auto* context = static_cast<ExecutingContext*>(JS_GetContextOpaque(ctx));

  JSValue& charStringValue = argv[0];

  if (!JS_IsString(charStringValue)) {
    return JS_ThrowTypeError(ctx, "Failed to execute '__webf_simulate_keypress__': first arguments should be a string");
  }

  std::unique_ptr<SharedNativeString> nativeString = webf::jsValueToNativeString(ctx, charStringValue);
  void* p = static_cast<void*>(nativeString.get());

  context->FlushUICommand(context->window(), FlushUICommandReason::kDependentsAll);

  context->dartMethodPtr()->simulateInputText(context->isDedicated(), static_cast<SharedNativeString*>(p));
  return JS_NULL;
};

static JSValue parseHTML(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  auto* context = static_cast<ExecutingContext*>(JS_GetContextOpaque(ctx));
  MemberMutationScope scope(context);

  if (argc == 1) {
    std::string strHTML = AtomicString(ctx, argv[0]).ToStdString(ctx);
    HTMLParser::parseHTML(strHTML, context->document()->documentElement());
  }

  return JS_NULL;
}

static JSValue syncThreadBuffer(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  auto* context = static_cast<ExecutingContext*>(JS_GetContextOpaque(ctx));
  context->uiCommandBuffer()->SyncToActive();
}

static JSValue triggerGlobalError(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  auto* context = static_cast<ExecutingContext*>(JS_GetContextOpaque(ctx));

  JSValue globalErrorFunc = JS_GetPropertyStr(ctx, context->Global(), "triggerGlobalError");

  if (JS_IsFunction(ctx, globalErrorFunc)) {
    JSValue exception = JS_Call(ctx, globalErrorFunc, context->Global(), 0, nullptr);
    context->HandleException(&exception);
    JS_FreeValue(ctx, globalErrorFunc);
  }

  return JS_NULL;
}

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
  if (execute_test_callback_ == nullptr) {
    return;
  }

  auto done = [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic,
                 JSValue* func_data) -> JSValue {
    JSValue& statusValue = argv[0];
    JSValue proxyObject = func_data[0];
    auto* callbackContext = static_cast<ExecuteCallbackContext*>(JS_GetOpaque(proxyObject, 1));

    if (!JS_IsString(statusValue)) {
      return JS_ThrowTypeError(ctx, "failed to execute 'done': parameter 1 (status) is not a string");
    }

    WEBF_LOG(VERBOSE) << "Done..";

    std::unique_ptr<SharedNativeString> status = webf::jsValueToNativeString(ctx, statusValue);

    callbackContext->context->dartIsolateContext()->dispatcher()->PostToDart(
        callbackContext->context->isDedicated(),
        [](ExecuteCallbackContext* callback_context, SharedNativeString* status) {
          Dart_Handle handle = Dart_HandleFromPersistent_DL(callback_context->persistent_handle);
          callback_context->executeCallback(handle, status);
          Dart_DeletePersistentHandle_DL(callback_context->persistent_handle);
          callback_context->webf_context->execute_test_proxy_object_ = JS_NULL;
        },
        callbackContext, status.release());
    JS_FreeValue(ctx, proxyObject);
    return JS_NULL;
  };

  auto* callbackContext = new ExecuteCallbackContext(context_, executeCallback, this, persistent_handle);
  execute_test_proxy_object_ = JS_NewObject(context_->ctx());
  JS_SetOpaque(execute_test_proxy_object_, callbackContext);
  JSValue callbackData[]{execute_test_proxy_object_};
  JSValue callback = JS_NewCFunctionData(context_->ctx(), done, 0, 0, 1, callbackData);

  ScriptValue arguments[] = {ScriptValue(context_->ctx(), callback)};
  ScriptValue result =
      execute_test_callback_->Invoke(context_->ctx(), ScriptValue::Empty(context_->ctx()), 1, arguments);
  context_->HandleException(&result);
  context_->DrainMicrotasks();
  JS_FreeValue(context_->ctx(), callback);
  execute_test_callback_ = nullptr;
}

WebFTestContext::WebFTestContext(ExecutingContext* context)
    : context_(context), page_(static_cast<WebFPage*>(context->owner())) {
  context->dartIsolateContext()->profiler()->StartTrackInitialize();

  page_->owner = this;
  page_->disposeCallback = [](WebFPage* bridge) { delete static_cast<WebFTestContext*>(bridge->owner); };

  std::initializer_list<MemberInstaller::FunctionConfig> functionConfig{
      {"__webf_execute_test__", executeTest, 1},
      {"__webf_match_image_snapshot__", matchImageSnapshot, 3},
      {"__webf_environment__", environment, 0},
      {"__webf_simulate_pointer__", simulatePointer, 3},
      {"__webf_simulate_inputtext__", simulateInputText, 1},
      {"__webf_sync_buffer__", syncThreadBuffer, 0},
      {"__webf_trigger_global_error__", triggerGlobalError, 0},
      {"__webf_parse_html__", parseHTML, 1},
  };

  MemberInstaller::InstallFunctions(context, context->Global(), functionConfig);
  initWebFTestFramework(context);

  context->dartIsolateContext()->profiler()->FinishTrackInitialize();
}

WebFTestContext::~WebFTestContext() {
  JS_FreeValue(context_->ctx(), execute_test_proxy_object_);
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

  dartMethodPtr->SetOnJSError(reinterpret_cast<OnJSError>(methodBytes[i++]));
  dartMethodPtr->SetMatchImageSnapshot(reinterpret_cast<MatchImageSnapshot>(methodBytes[i++]));
  dartMethodPtr->SetMatchImageSnapshotBytes(reinterpret_cast<MatchImageSnapshotBytes>(methodBytes[i++]));
  dartMethodPtr->SetEnvironment(reinterpret_cast<Environment>(methodBytes[i++]));
  dartMethodPtr->SetSimulatePointer(reinterpret_cast<SimulatePointer>(methodBytes[i++]));
  dartMethodPtr->SetSimulateInputText(reinterpret_cast<SimulateInputText>(methodBytes[i++]));

  assert_m(i == length, "Dart native methods count is not equal with C++ side method registrations.");
}

}  // namespace webf
