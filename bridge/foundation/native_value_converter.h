/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_FOUNDATION_NATIVE_VALUE_CONVERTER_H_
#define BRIDGE_FOUNDATION_NATIVE_VALUE_CONVERTER_H_


#if WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/script_wrappable.h"
#elif WEBF_V8_JS_ENGINE
#endif

//#include "core/binding_object.h"
#include "native_type.h"
#include "native_value.h"

namespace webf {

// NativeValueConverter converts types back and forth from C++ types to NativeValue. The template
// parameter |T| determines what kind of type conversion to perform.
template <typename T, typename SFINAEHelper = void>
struct NativeValueConverter {
  using ImplType = T;
};

template <typename T>
struct NativeValueConverterBase {
  using ImplType = typename T::ImplType;
};
//
//template <>
//struct NativeValueConverter<NativeTypeNull> : public NativeValueConverterBase<NativeTypeNull> {
//  static NativeValue ToNativeValue() { return Native_NewNull(); }
//
//  static ImplType FromNativeValue(JSContext* ctx) { return ScriptValue::Empty(ctx); }
//};
//
//template <>
//struct NativeValueConverter<NativeTypeString> : public NativeValueConverterBase<NativeTypeString> {
//  static NativeValue ToNativeValue(JSContext* ctx, const ImplType& value) {
//    return Native_NewString(value.ToNativeString(ctx).release());
//  }
//  static NativeValue ToNativeValue(const std::string& value) { return Native_NewCString(value); }
//
//  static ImplType FromNativeValue(JSContext* ctx, NativeValue&& value) {
//    if (value.tag == NativeTag::TAG_NULL) {
//      return AtomicString::Empty();
//    }
//    assert(value.tag == NativeTag::TAG_STRING);
//    return {ctx, std::unique_ptr<AutoFreeNativeString>(reinterpret_cast<AutoFreeNativeString*>(value.u.ptr))};
//  }
//
//  static ImplType FromNativeValue(JSContext* ctx, NativeValue& value) {
//    if (value.tag == NativeTag::TAG_NULL) {
//      return AtomicString::Empty();
//    }
//    assert(value.tag == NativeTag::TAG_STRING);
//    return {ctx, std::unique_ptr<AutoFreeNativeString>(reinterpret_cast<AutoFreeNativeString*>(value.u.ptr))};
//  }
//};

template <>
struct NativeValueConverter<NativeTypeBool> : public NativeValueConverterBase<NativeTypeBool> {
  static NativeValue ToNativeValue(ImplType value) { return Native_NewBool(value); }

  static ImplType FromNativeValue(NativeValue value) {
    if (value.tag == NativeTag::TAG_NULL) {
      return false;
    }

    assert(value.tag == NativeTag::TAG_BOOL);
    return value.u.int64 == 1;
  }
};

template <>
struct NativeValueConverter<NativeTypeInt64> : public NativeValueConverterBase<NativeTypeInt64> {
  static NativeValue ToNativeValue(ImplType value) { return Native_NewInt64(value); }

  static ImplType FromNativeValue(NativeValue value) {
    if (value.tag == NativeTag::TAG_NULL) {
      return 0;
    }

    assert(value.tag == NativeTag::TAG_INT);
    return value.u.int64;
  }
};

template <>
struct NativeValueConverter<NativeTypeDouble> : public NativeValueConverterBase<NativeTypeDouble> {
  static NativeValue ToNativeValue(ImplType value) { return Native_NewFloat64(value); }

  static ImplType FromNativeValue(NativeValue value) {
    if (value.tag == NativeTag::TAG_NULL) {
      return 0.0;
    }

    assert(value.tag == NativeTag::TAG_FLOAT64);
    double result;
    memcpy(&result, reinterpret_cast<void*>(&value.u.int64), sizeof(double));
    return result;
  }
};
//
//template <>
//struct NativeValueConverter<NativeTypeJSON> : public NativeValueConverterBase<NativeTypeJSON> {
//  static NativeValue ToNativeValue(JSContext* ctx, ImplType value, ExceptionState& exception_state) {
//    return Native_NewJSON(ctx, value, exception_state);
//  }
//  static ImplType FromNativeValue(JSContext* ctx, NativeValue value) {
//    if (value.tag == NativeTag::TAG_NULL) {
//      return ScriptValue::Empty(ctx);
//    }
//
//    assert(value.tag == NativeTag::TAG_JSON);
//    auto* str = static_cast<const char*>(value.u.ptr);
//    return ScriptValue::CreateJsonObject(ctx, str, strlen(str));
//  }
//};

class BindingObject;
struct DartReadable;

template <typename T>
struct NativeValueConverter<NativeTypePointer<T>, std::enable_if_t<std::is_void_v<T>>>
    : public NativeValueConverterBase<NativeTypePointer<T>> {
  static NativeValue ToNativeValue(T* value) { return Native_NewPtr(JSPointerType::Others, value); }
  static T* FromNativeValue(NativeValue value) {
    if (value.tag == NativeTag::TAG_NULL) {
      return nullptr;
    }

    assert(value.tag == NativeTag::TAG_POINTER);
    return static_cast<T*>(value.u.ptr);
  }
  static T* FromNativeValue(v8::Isolate* isolate, NativeValue value) {
    if (value.tag == NativeTag::TAG_NULL) {
      return nullptr;
    }

    assert(value.tag == NativeTag::TAG_POINTER);
    return static_cast<T*>(value.u.ptr);
  }
};

template <typename T>
struct NativeValueConverter<NativeTypePointer<T>, std::enable_if_t<std::is_base_of_v<DartReadable, T>>>
    : public NativeValueConverterBase<NativeTypePointer<T>> {
  static NativeValue ToNativeValue(T* value) { return Native_NewPtr(JSPointerType::Others, value); }
  static T* FromNativeValue(NativeValue value) {
    if (value.tag == NativeTag::TAG_NULL) {
      return nullptr;
    }

    assert(value.tag == NativeTag::TAG_POINTER);
    return static_cast<T*>(value.u.ptr);
  }
  static T* FromNativeValue(v8::Isolate* isolate, NativeValue value) {
    if (value.tag == NativeTag::TAG_NULL) {
      return nullptr;
    }

    assert(value.tag == NativeTag::TAG_POINTER);
    return static_cast<T*>(value.u.ptr);
  }
};
//
//template <typename T>
//struct NativeValueConverter<NativeTypePointer<T>, std::enable_if_t<std::is_base_of_v<ScriptWrappable, T>>>
//    : public NativeValueConverterBase<T> {
//  static NativeValue ToNativeValue(T* value) {
//    return Native_NewPtr(JSPointerType::NativeBindingObject, value->bindingObject());
//  }
//  static T* FromNativeValue(JSContext* ctx, NativeValue value) {
//    if (value.tag == NativeTag::TAG_NULL) {
//      return nullptr;
//    }
//    assert(value.tag == NativeTag::TAG_POINTER);
//    assert(value.uint32 == static_cast<int32_t>(JSPointerType::NativeBindingObject));
//    return DynamicTo<T>(BindingObject::From(static_cast<NativeBindingObject*>(value.u.ptr)));
//  }
//};
//
//template <>
//struct NativeValueConverter<NativeTypeFunction> : public NativeValueConverterBase<NativeTypeFunction> {
//  static NativeValue ToNativeValue(ImplType value) {
//    // Not supported.
//    assert(false);
//    return Native_NewNull();
//  }
//
//  static ImplType FromNativeValue(JSContext* ctx, NativeValue value) {
//    assert(value.tag == NativeTag::TAG_FUNCTION);
//    return QJSFunction::Create(ctx, BindingObject::AnonymousFunctionCallback, 4, value.u.ptr);
//  };
//};
//
//template <>
//struct NativeValueConverter<NativeTypeAsyncFunction> : public NativeValueConverterBase<NativeTypeAsyncFunction> {
//  static NativeValue ToNativeValue(ImplType value) {
//    // Not supported.
//    assert(false);
//    return Native_NewNull();
//  }
//
//  static ImplType FromNativeValue(JSContext* ctx, NativeValue value) {
//    assert(value.tag == NativeTag::TAG_ASYNC_FUNCTION);
//    return QJSFunction::Create(ctx, BindingObject::AnonymousAsyncFunctionCallback, 4, value.u.ptr);
//  }
//};

template <typename T>
struct NativeValueConverter<NativeTypeArray<T>> : public NativeValueConverterBase<NativeTypeArray<T>> {
  using ImplType = typename NativeTypeArray<typename NativeValueConverter<T>::ImplType>::ImplType;
  static NativeValue ToNativeValue(ImplType value) {
    auto* ptr = new NativeValue[value.size()];
    for (int i = 0; i < value.size(); i++) {
      ptr[i] = NativeValueConverter<T>::ToNativeValue(value[i]);
    }
    return Native_NewList(value.size(), ptr);
  }

  static ImplType FromNativeValue(v8::Isolate* isolate, NativeValue native_value) {
    if (native_value.tag == NativeTag::TAG_NULL) {
      return std::vector<typename T::ImplType>();
    }

    assert(native_value.tag == NativeTag::TAG_LIST);
    size_t length = native_value.uint32;
    auto* arr = static_cast<NativeValue*>(native_value.u.ptr);
    std::vector<typename T::ImplType> vec;
    vec.reserve(length);
    for (int i = 0; i < length; i++) {
      NativeValue v = arr[i];
      vec.emplace_back(NativeValueConverter<T>::FromNativeValue(isolate, std::move(v)));
    }
    return vec;
  }
};

}  // namespace webf

#endif  // BRIDGE_FOUNDATION_NATIVE_VALUE_CONVERTER_H_
