/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_DOM_TIMER_H
#define BRIDGE_DOM_TIMER_H

#if WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/qjs_function.h"
#include "bindings/qjs/script_wrappable.h"
#elif WEBF_V8_JS_ENGINE
#include "dom_timer_coordinator.h"
#include "v8-function.h"
#endif

namespace webf {

class DOMTimer {
 public:
  enum TimerKind { kOnce, kMultiple };
  enum TimerStatus { kPending, kExecuting, kFinished, kCanceled, kTerminated };

#if WEBF_QUICKJS_JS_ENGINE
  static std::shared_ptr<DOMTimer> create(ExecutingContext* context,
                                          const std::shared_ptr<Function>& callback,
                                          TimerKind timer_kind);
  DOMTimer(ExecutingContext* context, std::shared_ptr<Function> callback, TimerKind timer_kind);
#elif WEBF_V8_JS_ENGINE
  static std::shared_ptr<DOMTimer> create(ExecutingContext* context,
                                          v8::Local<v8::Function> callback,
                                          TimerKind timer_kind);
  DOMTimer(ExecutingContext* context, v8::Local<v8::Function> callback, TimerKind timer_kind);
  ~DOMTimer() {
    callback_.Reset();
  }
#endif

  // Trigger timer callback.
  void Fire();

  // Mark this timer is terminated and free the underly callbacks.
  void Terminate();

  TimerKind kind() const { return kind_; }

  [[nodiscard]] int32_t timerId() const { return timer_id_; };
  void setTimerId(int32_t timerId);

  void SetStatus(TimerStatus status) { status_ = status; }
  [[nodiscard]] TimerStatus status() const { return status_; }

  ExecutingContext* context() { return context_; }

 private:
  TimerKind kind_;
  ExecutingContext* context_{nullptr};
  int32_t timer_id_{-1};
  TimerStatus status_;
#if WEBF_QUICKJS_JS_ENGINE
  std::shared_ptr<Function> callback_;
#elif WEBF_V8_JS_ENGINE
  v8::Persistent<v8::Function> callback_;
#endif
};

}  // namespace webf

#endif  // BRIDGE_DOM_TIMER_H
