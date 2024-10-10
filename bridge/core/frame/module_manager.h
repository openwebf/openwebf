/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_MODULE_MANAGER_H
#define BRIDGE_MODULE_MANAGER_H

#include "bindings/qjs/atomic_string.h"
#include "bindings/qjs/exception_state.h"
#include "bindings/qjs/qjs_function.h"
#include "module_callback.h"
#include "qjs_module_manager_options.h"

namespace webf {

struct ModuleContext {
  ModuleContext(ExecutingContext* context, const std::shared_ptr<ModuleCallback>& callback)
      : context(context), callback(callback) {}
  ExecutingContext* context;
  std::shared_ptr<ModuleCallback> callback;
};

class ModuleManager {
 public:
  static ScriptValue __webf_invoke_module__(ExecutingContext* context,
                                            const AtomicString& module_name,
                                            const AtomicString& method,
                                            ExceptionState& exception);
  static ScriptValue __webf_invoke_module__(ExecutingContext* context,
                                            const AtomicString& module_name,
                                            const AtomicString& method,
                                            ScriptValue& params_value,
                                            ExceptionState& exception);
  static ScriptValue __webf_invoke_module__(ExecutingContext* context,
                                            const AtomicString& module_name,
                                            const AtomicString& method,
                                            ScriptValue& params_value,
                                            const std::shared_ptr<QJSFunction>& callback,
                                            ExceptionState& exception);
  static ScriptValue __webf_invoke_module_with_options__(ExecutingContext* context,
                                                         const AtomicString& module_name,
                                                         const AtomicString& method,
                                                         ScriptValue& params_value,
                                                         const std::shared_ptr<ModuleManagerOptions>& options,
                                                         const std::shared_ptr<QJSFunction>& callback,
                                                         ExceptionState& exception);
  static void __webf_add_module_listener__(ExecutingContext* context,
                                           const AtomicString& module_name,
                                           const std::shared_ptr<QJSFunction>& handler,
                                           ExceptionState& exception);
  static void __webf_remove_module_listener__(ExecutingContext* context,
                                              const AtomicString& module_name,
                                              ExceptionState& exception_state);
  static void __webf_clear_module_listener__(ExecutingContext* context, ExceptionState& exception_state);
};

}  // namespace webf

#endif  // BRIDGE_MODULE_MANAGER_H
