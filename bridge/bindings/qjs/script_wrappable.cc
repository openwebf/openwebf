/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "script_wrappable.h"
#include <quickjs/quickjs.h>
#include "built_in_string.h"
#include "core/executing_context.h"
#include "cppgc/gc_visitor.h"
#include "foundation/logging.h"

namespace webf {

ScriptWrappable::ScriptWrappable(JSContext* ctx)
    : ctx_(ctx),
      runtime_(JS_GetRuntime(ctx)),
      context_(ExecutingContext::From(ctx)),
      context_id_(context_->contextId()) {}

ScriptWrappable::~ScriptWrappable() {
  if (status_block_ != nullptr) {
    status_block_->disposed = true;
  }
}

JSValue ScriptWrappable::ToQuickJS() const {
  return JS_DupValue(ctx_, jsObject_);
}

JSValue ScriptWrappable::ToQuickJSUnsafe() const {
  return jsObject_;
}

ScriptValue ScriptWrappable::ToValue() {
  return ScriptValue(ctx_, jsObject_);
}

multi_threading::Dispatcher* ScriptWrappable::GetDispatcher() const {
  return context_->dartIsolateContext()->dispatcher().get();
}

/// This callback will be called when QuickJS GC is running at marking stage.
/// Users of this class should override `void TraceMember(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func)` to
/// tell GC which member of their class should be collected by GC.
static void HandleJSObjectGCMark(JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
  auto* object = static_cast<ScriptWrappable*>(JS_GetOpaque(val, JSValueGetClassId(val)));
  GCVisitor visitor{rt, mark_func};
  object->Trace(&visitor);
}

/// This callback will be called when QuickJS GC will release the `jsObject` object memory of this class.
/// The deconstruct method of this class will be called and all memory about this class will be freed when finalize
/// completed.
static void HandleJSObjectFinalized(JSRuntime* rt, JSValue val) {
  auto* object = static_cast<ScriptWrappable*>(JS_GetOpaque(val, JSValueGetClassId(val)));
  // When a JSObject got finalized by QuickJS GC, we can not guarantee the ExecutingContext are still alive and
  // accessible.
  if (isContextValid(object->contextId())) {
    ExecutingContext* context = object->GetExecutingContext();
    MemberMutationScope scope{object->GetExecutingContext()};
    delete object;
  } else {
    delete object;
  }
}

/// This callback will be called when JS code access this object using [] or `.` operator.
/// When exec `obj[1]`, it will call indexed_property_getter_handler_ defined in WrapperTypeInfo.
/// When exec `obj['hello']`, it will call string_property_getter_handler_ defined in WrapperTypeInfo.
static JSValue HandleJSPropertyGetterCallback(JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst receiver) {
  ExecutingContext* context = ExecutingContext::From(ctx);
  auto* object = static_cast<ScriptWrappable*>(JS_GetOpaque(obj, JSValueGetClassId(obj)));
  auto* wrapper_type_info = object->GetWrapperTypeInfo();

  JSValue getterValue = JS_UNDEFINED;
  if (wrapper_type_info->indexed_property_getter_handler_ != nullptr && JS_AtomIsTaggedInt(atom)) {
    getterValue = wrapper_type_info->indexed_property_getter_handler_(ctx, obj, JS_AtomToUInt32(atom));
  } else if (wrapper_type_info->string_property_getter_handler_ != nullptr) {
    getterValue = wrapper_type_info->string_property_getter_handler_(ctx, obj, atom);
  }

  if (!JS_IsUndefined(getterValue)) {
    return getterValue;
  }

  JSValue prototypeObject = context->contextData()->prototypeForType(wrapper_type_info);
  return JS_GetPropertyInternal(ctx, prototypeObject, atom, obj, NULL, 0);
}

/// This callback will be called when JS code set property on this object using [] or `.` operator.
/// When exec `obj[1] = 1`, it will call
static int HandleJSPropertySetterCallback(JSContext* ctx,
                                          JSValueConst obj,
                                          JSAtom atom,
                                          JSValueConst value,
                                          JSValueConst receiver,
                                          int flags) {
  auto* object = static_cast<ScriptWrappable*>(JS_GetOpaque(obj, JSValueGetClassId(obj)));
  auto* wrapper_type_info = object->GetWrapperTypeInfo();

  bool is_success = false;

  if (wrapper_type_info->indexed_property_setter_handler_ != nullptr && JS_AtomIsTaggedInt(atom)) {
    is_success = wrapper_type_info->indexed_property_setter_handler_(ctx, obj, JS_AtomToUInt32(atom), value);
  } else if (wrapper_type_info->string_property_setter_handler_ != nullptr) {
    is_success = wrapper_type_info->string_property_setter_handler_(ctx, obj, atom, value);
  }

  if (is_success) {
    return is_success;
  }

  ExecutingContext* context = ExecutingContext::From(ctx);
  JSValue prototypeObject = context->contextData()->prototypeForType(wrapper_type_info);
  if (JS_HasProperty(ctx, prototypeObject, atom)) {
    JSValue target = JS_DupValue(ctx, prototypeObject);
    JSValue setterFunc = JS_UNDEFINED;
    while (JS_IsUndefined(setterFunc) && JS_IsObject(target)) {
      JSPropertyDescriptor descriptor;
      descriptor.setter = JS_UNDEFINED;
      descriptor.getter = JS_UNDEFINED;
      descriptor.value = JS_UNDEFINED;
      JS_GetOwnProperty(ctx, &descriptor, target, atom);
      setterFunc = descriptor.setter;
      if (JS_IsFunction(ctx, setterFunc)) {
        JS_FreeValue(ctx, descriptor.getter);
        JS_FreeValue(ctx, descriptor.value);
        break;
      }

      JSValue new_target = JS_GetPrototype(ctx, target);
      JS_FreeValue(ctx, target);
      target = new_target;
      JS_FreeValue(ctx, descriptor.getter);
      JS_FreeValue(ctx, descriptor.setter);
      JS_FreeValue(ctx, descriptor.value);
    }

    if (!JS_IsFunction(ctx, setterFunc)) {
      return false;
    }

    assert_m(JS_IsFunction(ctx, setterFunc), "Setter on prototype should be an function.");
    JSValue ret = JS_Call(ctx, setterFunc, obj, 1, &value);
    if (JS_IsException(ret))
      return false;

    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, setterFunc);
    JS_FreeValue(ctx, target);
    return true;
  }

  return false;
}

/// This callback will be called when JS code check property exit on this object using `in` operator.
/// Wehn exec `'prop' in obj`, it will call.
static int HandleJSPropertyCheckerCallback(JSContext* ctx, JSValueConst obj, JSAtom atom) {
  auto* object = static_cast<ScriptWrappable*>(JS_GetOpaque(obj, JSValueGetClassId(obj)));
  auto* wrapper_type_info = object->GetWrapperTypeInfo();

  return wrapper_type_info->property_checker_handler_(ctx, obj, atom);
}

/// This callback will be called when JS code enumerate all own properties on this object.
/// Exp: Object.keys(obj);
static int HandleJSPropertyEnumerateCallback(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj) {
  auto* object = static_cast<ScriptWrappable*>(JS_GetOpaque(obj, JSValueGetClassId(obj)));
  auto* wrapper_type_info = object->GetWrapperTypeInfo();

  return wrapper_type_info->property_enumerate_handler_(ctx, ptab, plen, obj);
}

/// This callback will be called when JS code delete properties on this object.
/// Exp: delete obj['name']
static int HandleJSPropertyDelete(JSContext* ctx, JSValueConst obj, JSAtom prop) {
  auto* object = static_cast<ScriptWrappable*>(JS_GetOpaque(obj, JSValueGetClassId(obj)));
  auto* wrapper_type_info = object->GetWrapperTypeInfo();

  return wrapper_type_info->property_delete_handler_(ctx, obj, prop);
}

static int HandleJSGetOwnPropertyNames(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj) {
  // All props and methods are finded in prototype object of scriptwrappable.
  JSValue proto = JS_GetPrototype(ctx, obj);
  bool result = JS_GetOwnPropertyNames(ctx, ptab, plen, proto, JS_GPN_ENUM_ONLY | JS_GPN_STRING_MASK);
  JS_FreeValue(ctx, proto);
  return result;
};

static int HandleJSGetOwnProperty(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop) {
  // Call JSGetOwnPropertyNames will also call HandleJSGetOwnProperty for secondary verify.
  JSValue proto = JS_GetPrototype(ctx, obj);
  bool result = JS_GetOwnProperty(ctx, desc, proto, prop);
  JS_FreeValue(ctx, proto);
  return result;
}

void ScriptWrappable::InitializeQuickJSObject() {
  auto* wrapper_type_info = GetWrapperTypeInfo();
  JSRuntime* runtime = runtime_;

  /// ClassId should be a static QJSValue to make sure JSClassDef when this class are created at the first class.
  if (!JS_HasClassId(runtime, wrapper_type_info->classId)) {
    /// Basic template to describe the behavior about this class.
    JSClassDef def{};

    // Define object's className
    def.class_name = wrapper_type_info->className;

    // Register the hooks when GC marking at this object.
    def.gc_mark = HandleJSObjectGCMark;

    // Define the custom behavior of object.
    auto* exotic_methods = new JSClassExoticMethods{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

    // Define the callback when access object property.
    if (UNLIKELY(wrapper_type_info->indexed_property_getter_handler_ != nullptr ||
                 wrapper_type_info->string_property_getter_handler_ != nullptr)) {
      exotic_methods->get_property = HandleJSPropertyGetterCallback;
    }

    // Define the callback when set object property.
    if (UNLIKELY(wrapper_type_info->indexed_property_getter_handler_ != nullptr ||
                 wrapper_type_info->string_property_setter_handler_ != nullptr)) {
      exotic_methods->set_property = HandleJSPropertySetterCallback;
    }

    // Define the callback when check object property exist.
    if (UNLIKELY(wrapper_type_info->property_checker_handler_ != nullptr)) {
      exotic_methods->has_property = HandleJSPropertyCheckerCallback;
    }

    if (UNLIKELY(wrapper_type_info->property_enumerate_handler_ != nullptr)) {
      exotic_methods->get_own_property_names = HandleJSPropertyEnumerateCallback;
      exotic_methods->get_own_property = [](JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj,
                                            JSAtom prop) -> int {
        auto* object = static_cast<ScriptWrappable*>(JS_GetOpaque(obj, JSValueGetClassId(obj)));
        auto* wrapper_type_info = object->GetWrapperTypeInfo();

        if (wrapper_type_info->string_property_getter_handler_ != nullptr) {
          JSValue return_value = wrapper_type_info->string_property_getter_handler_(ctx, obj, prop);
          if (!JS_IsNull(return_value)) {
            if (desc != nullptr) {
              desc->flags = JS_PROP_ENUMERABLE;
              desc->value = return_value;
              desc->getter = JS_NULL;
              desc->setter = JS_NULL;
            } else {
              JS_FreeValue(ctx, return_value);
            }
            return true;
          }
        }

        if (wrapper_type_info->indexed_property_getter_handler_ != nullptr) {
          uint32_t index = JS_AtomToUInt32(prop);
          JSValue return_value = wrapper_type_info->indexed_property_getter_handler_(ctx, obj, index);
          if (!JS_IsNull(return_value)) {
            if (desc != nullptr) {
              desc->flags = JS_PROP_ENUMERABLE;
              desc->value = return_value;
              desc->getter = JS_NULL;
              desc->setter = JS_NULL;
            } else {
              JS_FreeValue(ctx, return_value);
            }
            return true;
          }
        }

        return false;
      };
    } else {
      // Support iterate script wrappable defined properties.
      exotic_methods->get_own_property_names = HandleJSGetOwnPropertyNames;
      exotic_methods->get_own_property = HandleJSGetOwnProperty;
    }

    if (UNLIKELY(wrapper_type_info->property_delete_handler_ != nullptr)) {
      exotic_methods->delete_property = HandleJSPropertyDelete;
    }

    def.exotic = exotic_methods;
    def.finalizer = HandleJSObjectFinalized;

    JS_NewClass(runtime, wrapper_type_info->classId, &def);
  }

  /// The JavaScript object underline this class. This `jsObject` is the JavaScript object which can be directly access
  /// within JavaScript code. When the reference count of `jsObject` decrease to 0, QuickJS will trigger `finalizer`
  /// callback and free `jsObject` memory. When QuickJS GC found `jsObject` at marking stage, `gc_mark` callback will be
  /// triggered.
  jsObject_ = JS_NewObjectClass(ctx_, wrapper_type_info->classId);
  JS_SetOpaque(jsObject_, this);

  // Let our instance into inherit prototype methods.
  JSValue prototype = GetExecutingContext()->contextData()->prototypeForType(wrapper_type_info);
  JS_SetPrototype(ctx_, jsObject_, prototype);
}

WebFValueStatus* ScriptWrappable::KeepAlive() {
  if (alive_count == 0) {
    context_->RegisterActiveScriptWrappers(this);
    JS_DupValue(ctx_, jsObject_);
    status_block_ = new WebFValueStatus();
  }
  alive_count++;
  return status_block_;
}

void ScriptWrappable::ReleaseAlive() {
  alive_count--;
  if (alive_count == 0) {
    delete status_block_;
    status_block_ = nullptr;
    if (context_->IsContextValid()) {
      context_->InActiveScriptWrappers(this);
    }
    if (context_->IsCtxValid()) {
      JS_FreeValue(ctx_, jsObject_);
    }
  }
}

}  // namespace webf
