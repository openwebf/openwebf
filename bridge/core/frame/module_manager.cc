/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include "module_manager.h"
#include "core/executing_context.h"
#include "foundation/logging.h"
#include "foundation/native_value.h"
#include "include/dart_api.h"
#include "module_callback.h"
#include "core/html/forms/form_data.h"
#include "qjs_module_manager_options.h"

namespace webf {

NativeValue* handleInvokeModuleTransientCallback(void* ptr,
                                                 double contextId,
                                                 const char* errmsg,
                                                 NativeValue* extra_data) {
  auto* moduleContext = static_cast<ModuleContext*>(ptr);
  ExecutingContext* context = moduleContext->context;

  if (!context->IsCtxValid() || !context->IsContextValid())
    return nullptr;

  if (moduleContext->callback == nullptr) {
    JSValue exception = JS_ThrowTypeError(moduleContext->context->ctx(),
                                          "Failed to execute '__webf_invoke_module__': callback is null.");
    context->HandleException(&exception);
    return nullptr;
  }

  JSContext* ctx = moduleContext->context->ctx();

  if (ctx == nullptr)
    return nullptr;

  context->dartIsolateContext()->profiler()->StartTrackAsyncEvaluation();
  context->dartIsolateContext()->profiler()->StartTrackSteps("handleInvokeModuleTransientCallback");

  ExceptionState exception_state;

  NativeValue* return_value = nullptr;
  if (errmsg != nullptr) {
    ScriptValue error_object = ScriptValue::CreateErrorObject(ctx, errmsg);
    ScriptValue arguments[] = {error_object};
    ScriptValue result = moduleContext->callback->value()->Invoke(ctx, ScriptValue::Empty(ctx), 1, arguments);
    if (result.IsException()) {
      context->HandleException(&result);
    }
    NativeValue native_result = result.ToNative(ctx, exception_state);
    return_value = static_cast<NativeValue*>(malloc(sizeof(NativeValue)));
    memcpy(return_value, &native_result, sizeof(NativeValue));
  } else {
    ScriptValue arguments[] = {ScriptValue::Empty(ctx), ScriptValue(ctx, *extra_data)};
    ScriptValue result = moduleContext->callback->value()->Invoke(ctx, ScriptValue::Empty(ctx), 2, arguments);
    if (result.IsException()) {
      context->HandleException(&result);
    }
    NativeValue native_result = result.ToNative(ctx, exception_state);
    return_value = static_cast<NativeValue*>(malloc(sizeof(NativeValue)));
    memcpy(return_value, &native_result, sizeof(NativeValue));
  }

  context->dartIsolateContext()->profiler()->FinishTrackSteps();
  context->dartIsolateContext()->profiler()->FinishTrackAsyncEvaluation();

  if (exception_state.HasException()) {
    context->HandleException(exception_state);
    return nullptr;
  }

  return return_value;
}

static void ReturnResultToDart(Dart_PersistentHandle persistent_handle,
                               NativeValue* result,
                               InvokeModuleResultCallback result_callback) {
  Dart_Handle handle = Dart_HandleFromPersistent_DL(persistent_handle);
  result_callback(handle, result);
  Dart_DeletePersistentHandle_DL(persistent_handle);
}

static NativeValue* handleInvokeModuleTransientCallbackWrapper(void* ptr,
                                                               double context_id,
                                                               const char* errmsg,
                                                               NativeValue* extra_data,
                                                               Dart_Handle dart_handle,
                                                               InvokeModuleResultCallback result_callback) {
  auto* moduleContext = static_cast<ModuleContext*>(ptr);

#if FLUTTER_BACKEND
  Dart_PersistentHandle persistent_handle = Dart_NewPersistentHandle_DL(dart_handle);
  moduleContext->context->dartIsolateContext()->dispatcher()->PostToJs(
      moduleContext->context->isDedicated(), moduleContext->context->contextId(),
      [](ModuleContext* module_context, double context_id, const char* errmsg, NativeValue* extra_data,
         Dart_PersistentHandle persistent_handle, InvokeModuleResultCallback result_callback) {
        NativeValue* result = handleInvokeModuleTransientCallback(module_context, context_id, errmsg, extra_data);
        module_context->context->dartIsolateContext()->dispatcher()->PostToDart(
            module_context->context->isDedicated(), ReturnResultToDart, persistent_handle, result, result_callback);
      },
      moduleContext, context_id, errmsg, extra_data, persistent_handle, result_callback);
  return nullptr;
#else
  return handleInvokeModuleTransientCallback(moduleContext, context_id, errmsg, extra_data);
#endif
}

NativeValue* handleInvokeModuleUnexpectedCallback(void* callbackContext,
                                                  double contextId,
                                                  const char* errmsg,
                                                  NativeValue* extra_data,
                                                  Dart_Handle dart_handle,
                                                  InvokeModuleResultCallback result_callback) {
  static_assert("Unexpected module callback, please check your invokeModule implementation on the dart side.");
  return nullptr;
}

ScriptValue ModuleManager::__webf_invoke_module__(ExecutingContext* context,
                                                  const AtomicString& module_name,
                                                  const AtomicString& method,
                                                  ExceptionState& exception) {
  ScriptValue empty = ScriptValue::Empty(context->ctx());
  return __webf_invoke_module__(context, module_name, method, empty, nullptr, exception);
}

ScriptValue ModuleManager::__webf_invoke_module__(ExecutingContext* context,
                                                  const AtomicString& module_name,
                                                  const AtomicString& method,
                                                  ScriptValue& params_value,
                                                  ExceptionState& exception) {
  return __webf_invoke_module__(context, module_name, method, params_value, nullptr, exception);
}

ScriptValue ModuleManager::__webf_invoke_module__(ExecutingContext* context,
                                                  const AtomicString& module_name,
                                                  const AtomicString& method,
                                                  ScriptValue& params_value,
                                                  const std::shared_ptr<QJSFunction>& callback,
                                                  ExceptionState& exception) {
  return __webf_invoke_module_with_options__(context, module_name, method, params_value, nullptr, callback,
                                             exception);
}

ScriptValue ModuleManager::__webf_invoke_module_with_options__(ExecutingContext* context,
                                                               const AtomicString& module_name,
                                                               const AtomicString& method,
                                                               ScriptValue& params_value,
                                                               const std::shared_ptr<ModuleManagerOptions>& options,
                                                               const std::shared_ptr<QJSFunction>& callback,
                                                               ExceptionState& exception) {
  NativeValue params = params_value.ToNative(context->ctx(), exception);
  if (options != nullptr && options->hasFormData()) {
    FormData* form_data = options->formData();
    WEBF_LOG(VERBOSE) << "FORM DATA: " << form_data;
  }

  if (exception.HasException()) {
    return ScriptValue::Empty(context->ctx());
  }

  NativeValue* result;
  auto module_name_string = module_name.ToNativeString(context->ctx());
  auto method_name_string = method.ToNativeString(context->ctx());

  context->dartIsolateContext()->profiler()->StartTrackLinkSteps("Call To Dart");

  if (callback != nullptr) {
    auto module_callback = ModuleCallback::Create(callback);
    auto module_context = std::make_shared<ModuleContext>(context, module_callback);
    context->ModuleContexts()->AddModuleContext(module_context);
    result = context->dartMethodPtr()->invokeModule(context->isDedicated(), module_context.get(), context->contextId(),
                                                    context->dartIsolateContext()->profiler()->link_id(),
                                                    module_name_string.get(), method_name_string.get(), &params,
                                                    1,
                                                    handleInvokeModuleTransientCallbackWrapper);
  } else {
    result = context->dartMethodPtr()->invokeModule(
        context->isDedicated(), nullptr, context->contextId(), context->dartIsolateContext()->profiler()->link_id(),
        module_name_string.get(), method_name_string.get(), &params, 1, handleInvokeModuleUnexpectedCallback);
  }

  context->dartIsolateContext()->profiler()->FinishTrackLinkSteps();

  if (result == nullptr) {
    return ScriptValue::Empty(context->ctx());
  }

  ScriptValue return_value = ScriptValue(context->ctx(), *result);
  dart_free(result);
  return return_value;
}

void ModuleManager::__webf_add_module_listener__(ExecutingContext* context,
                                                 const AtomicString& module_name,
                                                 const std::shared_ptr<QJSFunction>& handler,
                                                 ExceptionState& exception) {
  auto listener = ModuleListener::Create(handler);
  context->ModuleListeners()->AddModuleListener(module_name, listener);
}

void ModuleManager::__webf_remove_module_listener__(ExecutingContext* context,
                                                    const AtomicString& module_name,
                                                    ExceptionState& exception_state) {
  context->ModuleListeners()->RemoveModuleListener(module_name);
}

void ModuleManager::__webf_clear_module_listener__(ExecutingContext* context, ExceptionState& exception_state) {
  context->ModuleListeners()->Clear();
}

}  // namespace webf
