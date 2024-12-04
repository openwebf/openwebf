/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "script_value.h"
#include "atomic_string.h"

namespace webf {

v8::Local<v8::Value> ScriptValue::V8Value() const {
  return v8::Local<v8::Value>();

//  if (IsEmpty())
//    return v8::Local<v8::Value>();
//
//  assert_m(GetIsolate()->InContext(), "The v8::Isolate is not in a valid context.");
//  assert_m(!v8_reference_.IsEmpty(), "The v8::TracedReference is empty");
//  auto scriptState = ScriptState::From(isolate_->GetCurrentContext());
//  return v8_reference_.Get(scriptState->GetIsolate());
}

v8::Local<v8::Value> ScriptValue::V8ValueFor(
    ScriptState* target_script_state) const {
  return v8::Local<v8::Value>();

//  if (IsEmpty())
//    return v8::Local<v8::Value>();
//
//  assert_m(!v8_reference_.IsEmpty(), "The v8::TracedReference is empty");
//  v8::Isolate* isolate = target_script_state->GetIsolate();
//  return v8_reference_.Get(isolate);
}

bool ScriptValue::ToString(AtomicString& result) const {
  if (IsEmpty())
    return false;

  v8::Local<v8::Value> string = V8Value();
  if (string.IsEmpty() || !string->IsString())
    return false;

  result = AtomicString(GetIsolate()->GetCurrentContext(), v8::Local<v8::String>::Cast(string));
  return true;
}

ScriptValue ScriptValue::CreateNull(v8::Isolate* isolate) {
  return ScriptValue(isolate, v8::Null(isolate));
}

}  // namespace webf