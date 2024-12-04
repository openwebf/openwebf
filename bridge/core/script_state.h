/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_CORE_SCRIPT_STATE_H_
#define BRIDGE_CORE_SCRIPT_STATE_H_

#if WEBF_V8_JS_ENGINE
#include <v8/v8.h>
#include "bindings/v8/platform/scoped_persistent.h"
#elif WEBF_QUICKJS_JS_ENGINE
#include <quickjs/quickjs.h>
#endif

#include "dart_isolate_context.h"
#include <cassert>

namespace webf {

class DartIsolateContext;

// ScriptState is an abstraction class that holds all information about script
// execution (e.g., JSContext etc). If you need any info about the script execution, you're expected to
// pass around ScriptState in the code base. ScriptState is in a 1:1
// relationship with JSContext.
class ScriptState {
 public:
  ScriptState() = delete;
  ~ScriptState();

  inline bool Invalid() const { return !ctx_invalid_; }
#if WEBF_QUICKJS_JS_ENGINE
  ScriptState(DartIsolateContext* dart_context);
  inline JSContext* ctx() {
    assert(!ctx_invalid_ && "executingContext has been released");
    return ctx_;
  }
#elif WEBF_V8_JS_ENGINE
  ScriptState(DartIsolateContext* dart_context, v8::Local<v8::Context> context);
  inline v8::Local<v8::Context> ctx() {
    assert(!ctx_invalid_ && "executingContext has been released");
    v8::Isolate* isolate = dart_isolate_context_->isolate();
    return context_.NewLocal(isolate);
  }
  v8::Isolate* isolate();
#endif

 private:
#if WEBF_QUICKJS_JS_ENGINE
  JSContext* ctx_{nullptr};
#elif WEBF_V8_JS_ENGINE
  ScopedPersistent<v8::Context> context_;
#endif

  bool ctx_invalid_{false};
  DartIsolateContext* dart_isolate_context_{nullptr};
};

}  // namespace webf

#endif  // BRIDGE_CORE_SCRIPT_STATE_H_
