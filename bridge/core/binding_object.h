/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_CORE_DOM_BINDING_OBJECT_H_
#define BRIDGE_CORE_DOM_BINDING_OBJECT_H_

#include <include/dart_api_dl.h>
#include <cinttypes>
#include <unordered_set>
#if WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/atomic_string.h"
#include "bindings/qjs/script_wrappable.h"
#endif
#include "core/dart_methods.h"
#include "foundation/native_type.h"
#include "foundation/native_value.h"

namespace webf {

class BindingObject;
struct NativeBindingObject;
class ExceptionState;
class GCVisitor;
class ScriptPromiseResolver;
class DartIsolateContext;

using InvokeBindingsMethodsFromNative = void (*)(double contextId,
                                                 int64_t profile_id,
                                                 const NativeBindingObject* binding_object,
                                                 NativeValue* return_value,
                                                 NativeValue* method,
                                                 int32_t argc,
                                                 const NativeValue* argv);

using DartInvokeResultCallback = void (*)(Dart_Handle dart_object, NativeValue* result);

using InvokeBindingMethodsFromDart = void (*)(NativeBindingObject* binding_object,
                                              int64_t profile_id,
                                              NativeValue* method,
                                              int32_t argc,
                                              NativeValue* argv,
                                              Dart_Handle dart_object,
                                              DartInvokeResultCallback result_callback);

struct NativeBindingObject : public DartReadable {
  NativeBindingObject() = delete;
  explicit NativeBindingObject(BindingObject* target);

  static void HandleCallFromDartSide(const DartIsolateContext* dart_isolate_context,
                                     const NativeBindingObject* binding_object,
                                     int64_t profile_id,
                                     const NativeValue* method,
                                     int32_t argc,
                                     const NativeValue* argv,
                                     Dart_PersistentHandle dart_object,
                                     DartInvokeResultCallback result_callback);

  bool disposed_{false};
  BindingObject* binding_target_{nullptr};
  InvokeBindingMethodsFromDart invoke_binding_methods_from_dart{nullptr};
  InvokeBindingsMethodsFromNative invoke_bindings_methods_from_native{nullptr};
  void* extra{nullptr};
};

enum BindingMethodCallOperations {
  kGetProperty,
  kSetProperty,
  kGetAllPropertyNames,
  kAnonymousFunctionCall,
  kAsyncAnonymousFunction,
};

enum CreateBindingObjectType {
  kCreateDOMMatrix = 0,
  kCreatePath2D = 1,
  kCreateDOMPoint = 2,
};

struct BindingObjectPromiseContext : public DartReadable {
  ExecutingContext* context;
  BindingObject* binding_object;
  std::shared_ptr<ScriptPromiseResolver> promise_resolver;
};

class BindingObject : public ScriptWrappable {
 public:
  struct AnonymousFunctionData {
    std::string method_name;
  };

  // This function were called when the anonymous function returned to the JS code has been called by users.
  static ScriptValue AnonymousFunctionCallback(JSContext* ctx,
                                               const ScriptValue& this_val,
                                               uint32_t argc,
                                               const ScriptValue* argv,
                                               void* private_data);
  static ScriptValue AnonymousAsyncFunctionCallback(JSContext* ctx,
                                                    const ScriptValue& this_val,
                                                    uint32_t argc,
                                                    const ScriptValue* argv,
                                                    void* private_data);
  static void HandleAnonymousAsyncCalledFromDart(void* ptr,
                                                 NativeValue* native_value,
                                                 double contextId,
                                                 const char* errmsg);

  BindingObject() = delete;
  ~BindingObject();
  explicit BindingObject(JSContext* ctx);

  // Handle call from dart side.
  virtual NativeValue HandleCallFromDartSide(const AtomicString& method,
                                             int32_t argc,
                                             const NativeValue* argv,
                                             Dart_Handle dart_object);
  // Invoke methods which implemented at dart side.
  NativeValue InvokeBindingMethod(const AtomicString& method,
                                  int32_t argc,
                                  const NativeValue* args,
                                  uint32_t reason,
                                  ExceptionState& exception_state) const;
  NativeValue GetBindingProperty(const AtomicString& prop, uint32_t reason, ExceptionState& exception_state) const;
  NativeValue SetBindingProperty(const AtomicString& prop, NativeValue value, ExceptionState& exception_state) const;
  NativeValue GetAllBindingPropertyNames(ExceptionState& exception_state) const;

  void CollectElementDepsOnArgs(std::vector<NativeBindingObject*>& deps, size_t argc, const NativeValue* args) const;

  FORCE_INLINE NativeBindingObject* bindingObject() const { return binding_object_; }

  void Trace(GCVisitor* visitor) const;

  inline static BindingObject* From(NativeBindingObject* native_binding_object) {
    if (native_binding_object == nullptr)
      return nullptr;

    return native_binding_object->binding_target_;
  };

  virtual bool IsEventTarget() const;
  virtual bool IsTouchList() const;
  virtual bool IsComputedCssStyleDeclaration() const;
  virtual bool IsCanvasGradient() const;

 protected:
  void TrackPendingPromiseBindingContext(BindingObjectPromiseContext* binding_object_promise_context);
  void FullFillPendingPromise(BindingObjectPromiseContext* binding_object_promise_context);
  NativeValue InvokeBindingMethod(BindingMethodCallOperations binding_method_call_operation,
                                  size_t argc,
                                  const NativeValue* args,
                                  uint32_t reason,
                                  ExceptionState& exception_state) const;

  // NativeBindingObject may allocated at Dart side. Binding this with Dart allocated NativeBindingObject.
  explicit BindingObject(JSContext* ctx, NativeBindingObject* native_binding_object);

 private:
  NativeBindingObject* binding_object_ = nullptr;
  std::unordered_set<BindingObjectPromiseContext*> pending_promise_contexts_;
};

}  // namespace webf

#endif  // BRIDGE_CORE_DOM_BINDING_OBJECT_H_
