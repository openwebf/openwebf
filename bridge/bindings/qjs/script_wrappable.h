/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_SCRIPT_WRAPPABLE_H
#define BRIDGE_SCRIPT_WRAPPABLE_H

#include <quickjs/quickjs.h>
#include "bindings/qjs/cppgc/garbage_collected.h"
#include "core/executing_context.h"
#include "foundation/macros.h"
#include "wrapper_type_info.h"

namespace webf {

class ScriptValue;
class GCVisitor;

// Defines |GetWrapperTypeInfo| virtual method which returns the WrapperTypeInfo
// of the instance. Also declares a static member of type WrapperTypeInfo, of
// which the definition is given by the IDL code generator.
//
// All the derived classes of ScriptWrappable, regardless of directly or
// indirectly, must write this macro in the class definition as long as the
// class has a corresponding .idl file.
#define DEFINE_WRAPPERTYPEINFO()                                                             \
 public:                                                                                     \
  const WrapperTypeInfo* GetWrapperTypeInfo() const override { return &wrapper_type_info_; } \
  static const WrapperTypeInfo* GetStaticWrapperTypeInfo() { return &wrapper_type_info_; }   \
                                                                                             \
 private:                                                                                    \
  static const WrapperTypeInfo& wrapper_type_info_

// ScriptWrappable provides a way to map from/to C++ DOM implementation to/from
// JavaScript object (platform object).  ToQuickJS() converts a ScriptWrappable to
// a QuickJS object and toScriptWrappable() converts a QuickJS object back to
// a ScriptWrappable.
class ScriptWrappable : public GarbageCollected<ScriptWrappable> {
 public:
  ScriptWrappable() = delete;

  explicit ScriptWrappable(JSContext* ctx);
  virtual ~ScriptWrappable() = default;

  // Returns the WrapperTypeInfo of the instance.
  virtual const WrapperTypeInfo* GetWrapperTypeInfo() const = 0;

  void Trace(GCVisitor* visitor) const override{};

  virtual JSValue ToQuickJS() const;
  JSValue ToQuickJSUnsafe() const;

  ScriptValue ToValue();
  FORCE_INLINE ExecutingContext* GetExecutingContext() const { return context_; };
  FORCE_INLINE JSContext* ctx() const { return ctx_; }
  FORCE_INLINE JSRuntime* runtime() const { return runtime_; }
  FORCE_INLINE int64_t contextId() const { return context_id_; }

  void InitializeQuickJSObject() override;

  /**
   * Classes kept alive as long as they have a pending activity.
   * Release them via `ReleaseAlive` method.
   */
  void KeepAlive();
  void ReleaseAlive();

 private:
  bool is_alive = false;
  JSValue jsObject_{JS_NULL};
  JSContext* ctx_{nullptr};
  ExecutingContext* context_{nullptr};
  int64_t context_id_;
  JSRuntime* runtime_{nullptr};
  friend class GCVisitor;
};

// Converts a QuickJS object back to a ScriptWrappable.
template <typename ScriptWrappable>
inline ScriptWrappable* toScriptWrappable(JSValue object) {
  // If object is proxy object, should get the rel target of this proxy.
  if (JS_IsProxy(object)) {
    object = JS_GetProxyTarget(object);
  }
  return static_cast<ScriptWrappable*>(JS_GetOpaque(object, JSValueGetClassId(object)));
}

template <typename T>
Local<T>::~Local<T>() {
  if (raw_ == nullptr)
    return;
  auto* wrappable = To<ScriptWrappable>(raw_);
  // Record the free operation to avoid JSObject had been freed immediately.
  if (LIKELY(wrappable->GetExecutingContext()->HasMutationScope())) {
    wrappable->GetExecutingContext()->mutationScope()->RecordFree(wrappable);
  } else {
    assert_m(false, "LocalHandle must be used after MemberMutationScope allcated.");
  }
}

}  // namespace webf

#endif  // BRIDGE_SCRIPT_WRAPPABLE_H
