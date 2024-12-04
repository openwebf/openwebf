/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef GIN_PUBLIC_V8_IDLE_TASK_RUNNER_H_
#define GIN_PUBLIC_V8_IDLE_TASK_RUNNER_H_

#include <memory>
#include "bindings/v8/gin/gin_export.h"
#include <v8/v8-platform.h>

namespace gin {

// A V8IdleTaskRunner is a task runner for running idle tasks.
// Idle tasks have an unbound argument which is bound to a deadline in
// (v8::Platform::MonotonicallyIncreasingTime) when they are run.
// The idle task is expected to complete by this deadline.
class GIN_EXPORT V8IdleTaskRunner {
 public:
  virtual void PostIdleTask(std::unique_ptr<v8::IdleTask> task) = 0;

  virtual ~V8IdleTaskRunner() {}
};

}  // namespace gin

#endif  // GIN_PUBLIC_V8_IDLE_TASK_RUNNER_H_
