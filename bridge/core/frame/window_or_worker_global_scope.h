/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_WINDOW_OR_WORKER_GLOBAL_SCROPE_H
#define BRIDGE_WINDOW_OR_WORKER_GLOBAL_SCROPE_H

#if WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/exception_state.h"
#include "bindings/qjs/qjs_function.h"
#elif WEBF_V8_JS_ENGINE
#include "bindings/v8/exception_state.h"
#endif

#include "core/executing_context.h"
#include "foundation/function.h"

namespace webf {

class WindowOrWorkerGlobalScope {
 public:
#if WEBF_QUICKJS_JS_ENGINE
  static int setTimeout(ExecutingContext* context,
                        const std::shared_ptr<Function>& handler,
                        int32_t timeout,
                        ExceptionState& exception);
  static int setTimeout(ExecutingContext* context, const std::shared_ptr<Function>& handler, ExceptionState& exception);
  static int setInterval(ExecutingContext* context,
                         const std::shared_ptr<Function>& handler,
                         int32_t timeout,
                         ExceptionState& exception);
  static int setInterval(ExecutingContext* context, std::shared_ptr<Function> handler, ExceptionState& exception);
  static void clearTimeout(ExecutingContext* context, int32_t timerId, ExceptionState& exception);
  static void clearInterval(ExecutingContext* context, int32_t timerId, ExceptionState& exception);
  static void __gc__(ExecutingContext* context, ExceptionState& exception);
  static ScriptValue __memory_usage__(ExecutingContext* context, ExceptionState& exception_state);
#elif WEBF_V8_JS_ENGINE
  static int setTimeout(ExecutingContext* context,
                        v8::MaybeLocal<v8::Function> maybeCallback,
                        int32_t timeout,
                        ExceptionState& exception);
  static int setTimeout(ExecutingContext* context,
                        v8::MaybeLocal<v8::Function> maybeCallback,
                        ExceptionState& exception);
//  static int setInterval(ExecutingContext* context,
//                         const v8::Local<v8::Function> callback,
//                         int32_t timeout,
//                         ExceptionState& exception);
//  static int setInterval(ExecutingContext* context, std::shared_ptr<QJSFunction> handler, ExceptionState& exception);
//  static void clearTimeout(ExecutingContext* context, int32_t timerId, ExceptionState& exception);
//  static void clearInterval(ExecutingContext* context, int32_t timerId, ExceptionState& exception);
//  static void __gc__(ExecutingContext* context, ExceptionState& exception);
//  static ScriptValue __memory_usage__(ExecutingContext* context, ExceptionState& exception_state);
#endif
};

}  // namespace webf

#endif  // BRIDGE_WINDOW_OR_WORKER_GLOBAL_SCROPE_H
