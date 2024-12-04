/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include "window_or_worker_global_scope.h"
#include "core/frame/dom_timer.h"

namespace webf {

static void handleTimerCallback(DOMTimer* timer, char* errmsg) {
  auto* context = timer->context();

  if (errmsg != nullptr) {
    // TODO hanle exception
    //    JSValue exception = JS_ThrowTypeError(context->ctx(), "%s", errmsg);
    //    context->HandleException(&exception);
    context->Timers()->forceStopTimeoutById(timer->timerId());
    dart_free(errmsg);
    return;
  }

  if (context->Timers()->getTimerById(timer->timerId()) == nullptr)
    return;

  //  context->dartIsolateContext()->profiler()->StartTrackAsyncEvaluation();
  //  context->dartIsolateContext()->profiler()->StartTrackSteps("handleTimerCallback");

  // Trigger timer callbacks.
  timer->Fire();

  //  context->dartIsolateContext()->profiler()->FinishTrackSteps();
  //  context->dartIsolateContext()->profiler()->FinishTrackAsyncEvaluation();
}

static void handleTransientCallback(void* ptr, double contextId, char* errmsg) {
  if (!isContextValid(contextId))
    return;

  auto* timer = static_cast<DOMTimer*>(ptr);
  auto* context = timer->context();

  if (!context->IsContextValid())
    return;

  if (timer->status() == DOMTimer::TimerStatus::kCanceled || timer->status() == DOMTimer::TimerStatus::kTerminated) {
    return;
  }

  timer->SetStatus(DOMTimer::TimerStatus::kExecuting);
  handleTimerCallback(timer, errmsg);
  timer->SetStatus(DOMTimer::TimerStatus::kFinished);

  context->Timers()->removeTimeoutById(timer->timerId());
}

static void handleTransientCallbackWrapper(void* ptr, double contextId, char* errmsg) {
  auto* timer = static_cast<DOMTimer*>(ptr);
  auto* context = timer->context();

  if (!context->IsContextValid())
    return;

  context->dartIsolateContext()->dispatcher()->PostToJs(context->isDedicated(), contextId,
                                                        webf::handleTransientCallback, ptr, contextId, errmsg);
}

#if WEBF_QUICKJS_JS_ENGINE

static void handlePersistentCallback(void* ptr, double contextId, char* errmsg) {
  if (!isContextValid(contextId))
    return;

  auto* timer = static_cast<DOMTimer*>(ptr);
  auto* context = timer->context();

  if (!context->IsContextValid())
    return;

  if (timer->status() == DOMTimer::TimerStatus::kTerminated) {
    return;
  }

  if (timer->status() == DOMTimer::TimerStatus::kCanceled) {
    context->Timers()->removeTimeoutById(timer->timerId());
    return;
  }

  timer->SetStatus(DOMTimer::TimerStatus::kExecuting);
  handleTimerCallback(timer, errmsg);

  if (timer->status() == DOMTimer::TimerStatus::kCanceled) {
    context->Timers()->removeTimeoutById(timer->timerId());
    return;
  }

  timer->SetStatus(DOMTimer::TimerStatus::kFinished);
}

static void handlePersistentCallbackWrapper(void* ptr, double contextId, char* errmsg) {
  auto* timer = static_cast<DOMTimer*>(ptr);
  auto* context = timer->context();

  if (!context->IsContextValid())
    return;

  context->dartIsolateContext()->dispatcher()->PostToJs(context->isDedicated(), contextId,
                                                        webf::handlePersistentCallback, ptr, contextId, errmsg);
}

int WindowOrWorkerGlobalScope::setTimeout(ExecutingContext* context,
                                          const std::shared_ptr<Function>& handler,
                                          ExceptionState& exception) {
  return setTimeout(context, handler, 0.0, exception);
}

int WindowOrWorkerGlobalScope::setTimeout(ExecutingContext* context,
                                          const std::shared_ptr<Function>& handler,
                                          int32_t timeout,
                                          ExceptionState& exception) {
  if (handler == nullptr) {
    exception.ThrowException(context->ctx(), ErrorType::InternalError, "Timeout callback is null");
    return -1;
  }

  // Create a timer object to keep track timer callback.
  auto timer = DOMTimer::create(context, handler, DOMTimer::TimerKind::kOnce);
  auto timer_id = context->dartMethodPtr()->setTimeout(context->isDedicated(), timer.get(), context->contextId(),
                                                       handleTransientCallbackWrapper, timeout);

  // Register timerId.
  timer->setTimerId(timer_id);

  context->Timers()->installNewTimer(context, timer_id, timer);

  return timer_id;
}

int WindowOrWorkerGlobalScope::setInterval(ExecutingContext* context,
                                           std::shared_ptr<Function> handler,
                                           ExceptionState& exception) {
  return setInterval(context, handler, 0.0, exception);
}

int WindowOrWorkerGlobalScope::setInterval(ExecutingContext* context,
                                           const std::shared_ptr<Function>& handler,
                                           int32_t timeout,
                                           ExceptionState& exception) {
  if (handler == nullptr) {
    exception.ThrowException(context->ctx(), ErrorType::InternalError, "Timeout callback is null");
    return -1;
  }

  // Create a timer object to keep track timer callback.
  auto timer = DOMTimer::create(context, handler, DOMTimer::TimerKind::kMultiple);

  int32_t timerId = context->dartMethodPtr()->setInterval(context->isDedicated(), timer.get(), context->contextId(),
                                                          handlePersistentCallbackWrapper, timeout);

  // Register timerId.
  timer->setTimerId(timerId);
  context->Timers()->installNewTimer(context, timerId, timer);

  return timerId;
}

void WindowOrWorkerGlobalScope::clearTimeout(ExecutingContext* context, int32_t timerId, ExceptionState& exception) {
  context->dartMethodPtr()->clearTimeout(context->isDedicated(), context->contextId(), timerId);
  context->Timers()->forceStopTimeoutById(timerId);
}

void WindowOrWorkerGlobalScope::clearInterval(ExecutingContext* context, int32_t timerId, ExceptionState& exception) {
  context->dartMethodPtr()->clearTimeout(context->isDedicated(), context->contextId(), timerId);
  context->Timers()->forceStopTimeoutById(timerId);
}

void WindowOrWorkerGlobalScope::__gc__(ExecutingContext* context, ExceptionState& exception) {
  JS_RunGC(context->GetScriptState()->runtime());
}

ScriptValue WindowOrWorkerGlobalScope::__memory_usage__(ExecutingContext* context, ExceptionState& exception) {
  JSRuntime* runtime = context->GetScriptState()->runtime();
  JSMemoryUsage memory_usage;
  JS_ComputeMemoryUsage(runtime, &memory_usage);

  char buff[2048];
  snprintf(buff, 2048,
           R"({"malloc_size": %lld, "malloc_limit": %lld, "memory_used_size": %lld, "memory_used_count": %lld})",
           memory_usage.malloc_size, memory_usage.malloc_limit, memory_usage.memory_used_size,
           memory_usage.memory_used_count);

  return ScriptValue::CreateJsonObject(context->ctx(), buff, strlen(buff));
}
#elif WEBF_V8_JS_ENGINE

int WindowOrWorkerGlobalScope::setTimeout(ExecutingContext* context,
                                          const v8::MaybeLocal<v8::Function> maybeCallback,
                                          int32_t timeout,
                                          ExceptionState& exception) {
  v8::Local<v8::Function> callback;
  if (!maybeCallback.ToLocal(&callback)) {
    exception.ThrowException(context->ctx(), ErrorType::InternalError, "Timeout callback is null");
    return -1;
  }

    // Create a timer object to keep track timer callback.
    auto timer = DOMTimer::create(context, callback, DOMTimer::TimerKind::kOnce);
    auto timer_id = context->dartMethodPtr()->setTimeout(context->isDedicated(), timer.get(), context->contextId(),
                                                         handleTransientCallbackWrapper, timeout);

    // Register timerId.
    timer->setTimerId(timer_id);

    context->Timers()->installNewTimer(context, timer_id, timer);

    return timer_id;
}

int WindowOrWorkerGlobalScope::setTimeout(ExecutingContext* context,
                                          const v8::MaybeLocal<v8::Function> maybeCallback,
                                          ExceptionState& exception) {
  return setTimeout(context, maybeCallback, 0, exception);
}

#endif

}  // namespace webf
