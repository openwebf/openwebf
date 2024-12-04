/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_BINDINGS_WRAPPER_TYPE_INFO_H_
#define WEBF_BINDINGS_WRAPPER_TYPE_INFO_H_

#include <v8/v8-object.h>
#include <v8/v8.h>
#include "gin/public/wrapper_info.h"
#include "platform/platform_export.h"
#include "v8_interface_bridge_base.h"
#include "foundation/macros.h"

namespace webf {

//class DOMWrapperWorld;
class ScriptWrappable;

static const int kV8DOMWrapperTypeIndex =
    static_cast<int>(gin::kWrapperInfoIndex);
static const int kV8DOMWrapperObjectIndex =
    static_cast<int>(gin::kEncodedValueIndex);
static const int kV8DefaultWrapperInternalFieldCount =
    static_cast<int>(gin::kNumberOfInternalFields);
// The value of the following field isn't used (only its presence), hence no
// corresponding Index constant exists for it.
static const int kV8PrototypeInternalFieldcount = 1;

// This struct provides a way to store a bunch of information that is helpful
// when unwrapping v8 objects. Each v8 bindings class has exactly one static
// WrapperTypeInfo member, so comparing pointers is a safe way to determine if
// types match.
struct PLATFORM_EXPORT WrapperTypeInfo final {
  WEBF_DISALLOW_NEW();

  enum WrapperTypePrototype {
    kWrapperTypeObjectPrototype,
    kWrapperTypeNoPrototype,  // For legacy callback interface
  };

  enum WrapperClassId {
    // kNoInternalFieldClassId is used for the pseudo wrapper objects which do
    // not have any internal field pointing to a Blink object.
    kNoInternalFieldClassId = 0,
    // NodeClassId must be smaller than ObjectClassId, also must be non-zero.
    kNodeClassId = 1,
    kObjectClassId,
    kCustomWrappableId,
  };

  enum ActiveScriptWrappableInheritance {
    kNotInheritFromActiveScriptWrappable,
    kInheritFromActiveScriptWrappable,
  };

  enum IdlDefinitionKind {
    kIdlInterface,
    kIdlNamespace,
    kIdlCallbackInterface,
    kIdlBufferSourceType,
    kIdlObservableArray,
    kIdlAsyncOrSyncIterator,
    kCustomWrappableKind,
  };

  static const WrapperTypeInfo* Unwrap(v8::Local<v8::Value> type_info_wrapper) {
    return reinterpret_cast<const WrapperTypeInfo*>(
        v8::External::Cast(*type_info_wrapper)->Value());
  }

  bool Equals(const WrapperTypeInfo* that) const { return this == that; }

  bool IsSubclass(const WrapperTypeInfo* that) const {
    for (const WrapperTypeInfo* current = this; current;
         current = current->parent_class) {
      if (current == that)
        return true;
    }

    return false;
  }

  bool SupportsDroppingWrapper() const {
    return wrapper_class_id != kNoInternalFieldClassId;
  }

  // Returns a v8::Template of interface object, namespace object, or the
  // counterpart of the IDL definition.
  //
  // - kIdlInterface: v8::FunctionTemplate of interface object
  // - kIdlNamespace: v8::ObjectTemplate of namespace object
  // - kIdlCallbackInterface: v8::FunctionTemplate of legacy callback
  //       interface object
  // - kIdlAsyncOrSyncIterator: v8::FunctionTemplate of default (asynchronous
  //       or synchronous) iterator object
  // - kCustomWrappableKind: v8::FunctionTemplate
//  v8::Local<v8::Template> GetV8ClassTemplate(
//      v8::Isolate* isolate,
//      const DOMWrapperWorld& world) const;

//  void InstallConditionalFeatures(
//      v8::Local<v8::Context> context,
//      const DOMWrapperWorld& world,
//      v8::Local<v8::Object> instance_object,
//      v8::Local<v8::Object> prototype_object,
//      v8::Local<v8::Object> interface_object,
//      v8::Local<v8::Template> interface_template) const {
//    /*TODO fix FeatureSelector
//
//    if (!install_context_dependent_props_func)
//      return;
//
//    install_context_dependent_props_func(
//        context, world, instance_object, prototype_object, interface_object,
//        interface_template, bindings::V8InterfaceBridgeBase::FeatureSelector());
//        */
//  }

  bool IsActiveScriptWrappable() const {
    return active_script_wrappable_inheritance ==
           kInheritFromActiveScriptWrappable;
  }

  // This field must be the first member of the struct WrapperTypeInfo.
  // See also static_assert() in .cpp file.
//  const gin::GinEmbedder gin_embedder;

  bindings::V8InterfaceBridgeBase::InstallInterfaceTemplateFuncType
      install_interface_template_func;
  /*TODO fix InstallContextDependentPropertiesFuncType
  bindings::V8InterfaceBridgeBase::InstallContextDependentPropertiesFuncType
      install_context_dependent_props_func;
      */
  const char* interface_name;
  const WrapperTypeInfo* parent_class;
  unsigned wrapper_type_prototype : 2;  // WrapperTypePrototype
  unsigned wrapper_class_id : 2;        // WrapperClassId
  unsigned                              // ActiveScriptWrappableInheritance
      active_script_wrappable_inheritance : 1;
  unsigned idl_definition_kind : 3;  // IdlDefinitionKind

  // This is a special case only used by V8WindowProperties::WrapperTypeInfo().
  // WindowProperties is part of Window's prototype object's prototype chain,
  // but not part of Window's interface object prototype chain. When this bit is
  // set, V8PerContextData::ConstructorForTypeSlowCase() skips over this type
  // when constructing the interface object's prototype chain.
  bool is_skipped_in_interface_object_prototype_chain : 1;
};

template <typename T, int offset>
inline T* GetInternalField(const v8::TracedReference<v8::Object>& wrapper) {
//  DCHECK_LT(offset, v8::Object::InternalFieldCount(wrapper));
  return reinterpret_cast<T*>(
      v8::Object::GetAlignedPointerFromInternalField(wrapper, offset));
}

template <typename T, int offset>
inline T* GetInternalField(v8::Local<v8::Object> wrapper) {
//  DCHECK_LT(offset, wrapper->InternalFieldCount());
  return reinterpret_cast<T*>(
      wrapper->GetAlignedPointerFromInternalField(offset));
}

template <typename T, int offset>
inline T* GetInternalField(v8::Isolate* isolate,
                           v8::Local<v8::Object> wrapper) {
//  DCHECK_LT(offset, wrapper->InternalFieldCount());
/*TODO fix
 * void* Object::GetAlignedPointerFromInternalField(v8::Isolate* isolate,
int index) {
  return reinterpret_cast<T*>(
      wrapper->GetAlignedPointerFromInternalField(isolate, offset));
      */

// just for compiler , todo fix
return reinterpret_cast<T*>(
    wrapper->GetAlignedPointerFromInternalField(offset));
}

// The return value can be null if |wrapper| is a global proxy, which points to
// nothing while a navigation.
inline ScriptWrappable* ToScriptWrappable(
    v8::Isolate* isolate,
    const v8::TracedReference<v8::Object>& wrapper) {
  return GetInternalField<ScriptWrappable, kV8DOMWrapperObjectIndex>(wrapper);
}

inline ScriptWrappable* ToScriptWrappable(v8::Isolate* isolate,
                                          v8::Local<v8::Object> wrapper) {
  return GetInternalField<ScriptWrappable, kV8DOMWrapperObjectIndex>(isolate,
                                                                     wrapper);
}

PLATFORM_EXPORT const WrapperTypeInfo* ToWrapperTypeInfo(
    v8::Local<v8::Object> wrapper);

}  // namespace webf

#endif  // WEBF_BINDINGS_WRAPPER_TYPE_INFO_H_
