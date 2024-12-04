/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "v8_binding_initializer.h"
#include "v8_window_or_worker_global_scope.h"

namespace webf {

void InstallBindings(ExecutingContext* context) {
  // Must follow the inheritance order when install.
  // Exp: Node extends EventTarget, EventTarget must be install first.
  V8WindowOrWorkerGlobalScope::Install(context);
}

}  // namespace webf