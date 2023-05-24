/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include "script_state.h"
#include "defined_properties_initializer.h"
#include "event_factory.h"
#include "html_element_factory.h"
#include "names_installer.h"

namespace webf {

thread_local std::atomic<int32_t> runningContexts{0};

ScriptState::ScriptState(DartIsolateContext* dart_context) : dart_isolate_context_(dart_context) {
  runningContexts++;
  // Avoid stack overflow when running in multiple threads.
  ctx_ = JS_NewContext(dart_isolate_context_->dartContext()->runtime());
  names_installer::Init(ctx_);
}

JSRuntime* ScriptState::runtime() {
  return dart_isolate_context_->dartContext()->runtime();
}

ScriptState::~ScriptState() {
  ctx_invalid_ = true;
  JS_FreeContext(ctx_);

  // Run GC to clean up remaining objects about m_ctx;
  JS_RunGC(dart_isolate_context_->dartContext()->runtime());

  ctx_ = nullptr;
}
}  // namespace webf
