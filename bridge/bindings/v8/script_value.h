/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/
#ifndef BRIDGE_SCRIPT_VALUE_H
#define BRIDGE_SCRIPT_VALUE_H

#include <v8/cppgc/visitor.h>
#include <v8/v8.h>
#include <concepts>
#include <memory>
#include "atomic_string.h"
#include "dictionary_base.h"
#include "foundation/macros.h"
#include "foundation/native_string.h"
//#include "platform/script_state.h"
#include "union_base.h"
//#include "bindings/v8/platform/wtf/vector_traits.h"
#include "foundation/macros.h"
#include "bindings/v8/trace_wrapper_v8_reference.h"

namespace webf {
namespace bindings {
class DictionaryBase;
class UnionBase;
}  // namespace bindings
class ScriptState;

// ScriptValue is used when an idl specifies the type as 'any'. ScriptValue
// stores the v8 value using WorldSafeV8Reference.
class ScriptValue final {
  WEBF_DISALLOW_NEW();

 public:
  // ScriptValue::From() is restricted to certain types that are unambiguous in
  // how they are exposed to V8. Objects that need to know what the expected IDL
  // type is in order to be correctly converted must explicitly use ToV8Traits<>
  // to get a v8::Value, then pass it directly to the constructor.
//  template <typename T>
//  requires std::derived_from<T, bindings::DictionaryBase> ||
//      std::derived_from<T, ScriptWrappable> ||
//      std::derived_from<T, bindings::UnionBase>
//      static ScriptValue From(ScriptState* script_state, T* value) {
//    return ScriptValue(script_state->GetIsolate(), value->ToV8(script_state));
//  }

  ScriptValue() = default;

  ScriptValue(v8::Isolate* isolate, v8::Local<v8::Value> value)
      : isolate_(isolate) {
    assert_m(isolate_, "isolate is nullptr");
    if (value.IsEmpty())
        return;
    v8_reference_.Reset(isolate, value);
  }

  ~ScriptValue() {
    // Reset() below eagerly cleans up Oilpan-internal book-keeping data
    // structures. Since most uses of ScriptValue are from stack or parameters
    // this significantly helps in keeping memory compact at the expense of a
    // few more finalizers in the on-heap use case. Keeping the internals
    // compact is important in AudioWorklet use cases that don't allocate and
    // thus never trigger GC.
    //
    // Note: If you see a CHECK() fail in non-production code (e.g. C++ unit
    // tests) then this means that the test runs manual GCs and/or invokes the
    // `RunLoop` to trigger GCs without stack while having a ScriptValue on the
    // stack which is not supported. To solve this pass the `v8::StackState`
    // explicitly on GCs. Alternatively, you can keep ScriptValue alive via
    // wrapper objects through Persistent instead of referring to it from the
    // stack.
    //
    // TODO(v8:v8:13372): Remove once v8::TracedReference is implemented as
    // direct pointer.
    v8_reference_.Reset();
  }

  ScriptValue(const ScriptValue& value) = default;

  // TODO(riakf): Use this GetIsolate() only when doing DCHECK inside
  // ScriptValue.
  v8::Isolate* GetIsolate() const {
    assert_m(isolate_, "isolate is nullptr");
    return isolate_;
  }

  ScriptValue& operator=(const ScriptValue& value) = default;

  bool operator==(const ScriptValue& value) const {
    if (IsEmpty())
      return value.IsEmpty();
    if (value.IsEmpty())
      return false;
    return v8_reference_ == value.v8_reference_;
  }

  bool operator!=(const ScriptValue& value) const { return !operator==(value); }

  // This creates a new local Handle; Don't use this in performance-sensitive
  // places.
  bool IsNull() const {
    assert_m(!IsEmpty(), "ScriptValue is empty");
    v8::Local<v8::Value> value = V8Value();
    return !value.IsEmpty() && value->IsNull();
  }

  // This creates a new local Handle; Don't use this in performance-sensitive
  // places.
  bool IsUndefined() const {
    assert_m(!IsEmpty(), "ScriptValue is empty");
    v8::Local<v8::Value> value = V8Value();
    return !value.IsEmpty() && value->IsUndefined();
  }

  // This creates a new local Handle; Don't use this in performance-sensitive
  // places.
  bool IsObject() const {
    assert_m(!IsEmpty(), "ScriptValue is empty");
    v8::Local<v8::Value> value = V8Value();
    return !value.IsEmpty() && value->IsObject();
  }

  bool IsEmpty() const { return v8_reference_.IsEmpty(); }

  void Clear() {
    isolate_ = nullptr;
    v8_reference_.Reset();
  }

  v8::Local<v8::Value> V8Value() const;
  // Returns v8Value() if a given ScriptState is the same as the
  // ScriptState which is associated with this ScriptValue. Otherwise
  // this "clones" the v8 value and returns it.
  v8::Local<v8::Value> V8ValueFor(ScriptState*) const;

  bool ToString(AtomicString&) const;

  static ScriptValue CreateNull(v8::Isolate*);

  void Trace(Visitor* visitor) const { visitor->Trace(v8_reference_); }

 private:
  v8::Isolate* isolate_ = nullptr;
  TraceWrapperV8Reference<v8::Value> v8_reference_;
};

}  // namespace webf

#endif  // BRIDGE_SCRIPT_VALUE_H
