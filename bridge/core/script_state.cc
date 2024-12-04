/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include "script_state.h"
//#include "event_factory.h"
#include "html_element_factory.h"
//#include "names_installer.h"

namespace webf {

thread_local std::atomic<int32_t> runningContexts{0};

#if WEBF_QUICKJS_JS_ENGINE
ScriptState::ScriptState(DartIsolateContext* dart_context) : dart_isolate_context_(dart_context) {
  runningContexts++;
  // Avoid stack overflow when running in multiple threads.
  ctx_ = JS_NewContext(dart_isolate_context_->runtime());
  InitializeBuiltInStrings(ctx_);
}

JSRuntime* ScriptState::runtime() {
  return dart_isolate_context_->runtime();
}
#elif WEBF_V8_JS_ENGINE
ScriptState::ScriptState(DartIsolateContext* dart_context,
                         v8::Local<v8::Context> context)
    : dart_isolate_context_(dart_context),
      context_(dart_context->isolate(), context) {
  runningContexts++;
  // Avoid stack overflow when running in multiple threads.

//  context_.SetWeak(this, &OnV8ContextCollectedCallback);
//  context->SetAlignedPointerInEmbedderData(kV8ContextPerContextDataIndex, this);
}

v8::Isolate* ScriptState::isolate() {
  return dart_isolate_context_->isolate();
}

//void ScriptState::OnV8ContextCollectedCallback(
//    const v8::WeakCallbackInfo<ScriptState>& data) {
////  data.GetParameter()->reference_from_v8_context_.Clear();
//  data.GetParameter()->context_.Clear();
//}

#endif

#if WEBF_QUICKJS_JS_ENGINE
ScriptState::~ScriptState() {
  ctx_invalid_ = true;
  JSRuntime* rt = JS_GetRuntime(ctx_);
  JS_TurnOnGC(rt);
  JS_FreeContext(ctx_);

  // Run GC to clean up remaining objects about m_ctx;
  JS_RunGC(rt);

  ctx_ = nullptr;
}

#elif WEBF_V8_JS_ENGINE
ScriptState::~ScriptState() {
  ctx_invalid_ = true;
}
#endif

}  // namespace webf
