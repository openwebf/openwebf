/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_BINDINGS_QJS_CONVERTER_IMPL_H_
#define BRIDGE_BINDINGS_QJS_CONVERTER_IMPL_H_

#include <memory>
#include <type_traits>
#include "atomic_string.h"
#include "bindings/qjs/union_base.h"
#include "converter.h"
#include "core/dom/events/event.h"
#include "core/dom/events/event_target.h"
#include "core/dom/node_list.h"
#include "core/fileapi/blob_part.h"
#include "core/fileapi/blob_property_bag.h"
#include "core/frame/window.h"
#include "core/html/html_body_element.h"
#include "core/html/html_div_element.h"
#include "core/html/html_element.h"
#include "core/html/html_head_element.h"
#include "core/html/html_html_element.h"
#include "exception_message.h"
#include "idl_type.h"
#include "js_event_handler.h"
#include "js_event_listener.h"
#include "native_string_utils.h"
#include "script_promise.h"

namespace webf {

template <typename T>
struct is_shared_ptr : std::false_type {};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

// Optional value for pointer value.
template <typename T>
struct Converter<IDLOptional<T>, std::enable_if_t<std::is_pointer<typename Converter<T>::ImplType>::value>>
    : public ConverterBase<IDLOptional<T>> {
  using ImplType = typename Converter<T>::ImplType;

  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception) {
    if (JS_IsUndefined(value)) {
      return nullptr;
    }
    return Converter<T>::FromValue(ctx, value, exception);
  }

  static ImplType ArgumentsValue(ExecutingContext* context,
                                 JSValue value,
                                 uint32_t argv_index,
                                 ExceptionState& exception_state) {
    if (JS_IsUndefined(value)) {
      return nullptr;
    }
    return Converter<T>::ArgumentsValue(context, value, argv_index, exception_state);
  }

  static JSValue ToValue(JSContext* ctx, typename Converter<T>::ImplType value) {
    if (value == nullptr) {
      return JS_UNDEFINED;
    }

    return Converter<T>::ToValue(ctx, value);
  }
};

template <typename T>
struct Converter<IDLOptional<T>, std::enable_if_t<is_shared_ptr<typename Converter<T>::ImplType>::value>>
    : public ConverterBase<IDLOptional<T>> {
  using ImplType = typename Converter<T>::ImplType;

  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception) {
    if (JS_IsUndefined(value)) {
      return nullptr;
    }
    return Converter<T>::FromValue(ctx, value, exception);
  }

  static JSValue ToValue(JSContext* ctx, typename Converter<T>::ImplType value) {
    if (value == nullptr) {
      return JS_UNDEFINED;
    }

    return Converter<T>::ToValue(ctx, value);
  }
};

// Optional value for arithmetic value
template <typename T>
struct Converter<IDLOptional<T>, std::enable_if_t<std::is_arithmetic<typename Converter<T>::ImplType>::value>>
    : public ConverterBase<IDLOptional<T>> {
  using ImplType = typename Converter<T>::ImplType;

  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception) {
    if (JS_IsUndefined(value)) {
      return 0;
    }
    return Converter<T>::FromValue(ctx, value, exception);
  }

  static JSValue ToValue(JSContext* ctx, typename Converter<T>::ImplType value) {
    return Converter<T>::ToValue(ctx, value);
  }
};

// Any
template <>
struct Converter<IDLAny> : public ConverterBase<IDLAny> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    return ScriptValue(ctx, value);
  }

  static JSValue ToValue(JSContext* ctx, const ScriptValue& value) { return JS_DupValue(ctx, value.QJSValue()); }
};

template <>
struct Converter<IDLOptional<IDLAny>> : public ConverterBase<IDLOptional<IDLAny>> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsUndefined(value)) {
      return ScriptValue::Empty(ctx);
    }

    assert(!JS_IsException(value));
    return ScriptValue(ctx, value);
  }

  static JSValue ToValue(JSContext* ctx, typename Converter<IDLAny>::ImplType value) {
    return Converter<IDLAny>::ToValue(ctx, std::move(value));
  }
};

template <>
struct Converter<IDLNullable<IDLAny>> : public ConverterBase<IDLNullable<IDLAny>> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsNull(value)) {
      return ScriptValue::Empty(ctx);
    }

    assert(!JS_IsException(value));
    return ScriptValue(ctx, value);
  }
};

// Boolean
template <>
struct Converter<IDLBoolean> : public ConverterBase<IDLBoolean> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    return JS_ToBool(ctx, value);
  };

  static JSValue ToValue(JSContext* ctx, bool value) { return JS_NewBool(ctx, value); };
};

// Uint32
template <>
struct Converter<IDLUint32> : public ConverterBase<IDLUint32> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    uint32_t v;
    JS_ToUint32(ctx, &v, value);
    return v;
  }

  static JSValue ToValue(JSContext* ctx, uint32_t v) { return JS_NewUint32(ctx, v); }
};

// Int32
template <>
struct Converter<IDLInt32> : public ConverterBase<IDLInt32> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    int32_t v;
    JS_ToInt32(ctx, &v, value);
    return v;
  }
  static JSValue ToValue(JSContext* ctx, uint32_t v) { return JS_NewInt32(ctx, v); }
};

// Int64
template <>
struct Converter<IDLInt64> : public ConverterBase<IDLInt64> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    int64_t v;
    JS_ToInt64(ctx, &v, value);
    return v;
  }
  static JSValue ToValue(JSContext* ctx, uint32_t v) { return JS_NewInt64(ctx, v); }
};

template <>
struct Converter<IDLDouble> : public ConverterBase<IDLDouble> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    double v;
    JS_ToFloat64(ctx, &v, value);
    return v;
  }

  static JSValue ToValue(JSContext* ctx, double v) { return JS_NewFloat64(ctx, v); }
};

template <>
struct Converter<IDLDOMString> : public ConverterBase<IDLDOMString> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    return AtomicString(ctx, value);
  }

  static JSValue ToValue(JSContext* ctx, const AtomicString& value) { return value.ToQuickJS(ctx); }
  static JSValue ToValue(JSContext* ctx, SharedNativeString* str) {
    return JS_NewUnicodeString(ctx, str->string(), str->length());
  }
  static JSValue ToValue(JSContext* ctx, std::unique_ptr<SharedNativeString> str) {
    return JS_NewUnicodeString(ctx, str->string(), str->length());
  }
  static JSValue ToValue(JSContext* ctx, uint16_t* bytes, size_t length) {
    return JS_NewUnicodeString(ctx, bytes, length);
  }
  static JSValue ToValue(JSContext* ctx, const std::string& str) { return JS_NewString(ctx, str.c_str()); }
};

template <>
struct Converter<IDLOptional<IDLDOMString>> : public ConverterBase<IDLDOMString> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsUndefined(value))
      return AtomicString::Empty();
    return Converter<IDLDOMString>::FromValue(ctx, value, exception_state);
  }

  static JSValue ToValue(JSContext* ctx, uint16_t* bytes, size_t length) {
    return Converter<IDLDOMString>::ToValue(ctx, bytes, length);
  }
  static JSValue ToValue(JSContext* ctx, const std::string& str) { return Converter<IDLDOMString>::ToValue(ctx, str); }
  static JSValue ToValue(JSContext* ctx, typename Converter<IDLDOMString>::ImplType value) {
    if (value == AtomicString::Null()) {
      return JS_UNDEFINED;
    }

    return Converter<IDLDOMString>::ToValue(ctx, std::move(value));
  }
};

template <>
struct Converter<IDLNullable<IDLDOMString>> : public ConverterBase<IDLDOMString> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsNull(value) || JS_IsUndefined(value))
      return AtomicString::Null();
    return Converter<IDLDOMString>::FromValue(ctx, value, exception_state);
  }

  static JSValue ToValue(JSContext* ctx, const std::string& value) { return AtomicString(ctx, value).ToQuickJS(ctx); }
  static JSValue ToValue(JSContext* ctx, const AtomicString& value) {
    if (value == AtomicString::Null()) {
      return JS_NULL;
    }
    return value.ToQuickJS(ctx);
  }
};

template <>
struct Converter<IDLLegacyDOMString> : public ConverterBase<IDLLegacyDOMString> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsNull(value)) {
      return AtomicString::Empty();
    }
    return AtomicString(ctx, value);
  }

  static JSValue ToValue(JSContext* ctx, const std::string& value) { return AtomicString(ctx, value).ToQuickJS(ctx); }
  static JSValue ToValue(JSContext* ctx, const AtomicString& value) { return value.ToQuickJS(ctx); }
};

template <typename T>
struct Converter<IDLSequence<T>> : public ConverterBase<IDLSequence<T>> {
  using ImplType = typename IDLSequence<typename Converter<T>::ImplType>::ImplType;

  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));

    if (!JS_IsArray(ctx, value)) {
      exception_state.ThrowException(ctx, ErrorType::TypeError, "The expected type of value is not array.");
      return {};
    }

    ImplType v;
    uint32_t length = Converter<IDLUint32>::FromValue(ctx, JS_GetPropertyStr(ctx, value, "length"), exception_state);

    v.reserve(length);

    for (uint32_t i = 0; i < length; i++) {
      JSValue iv = JS_GetPropertyUint32(ctx, value, i);
      auto&& item = Converter<T>::FromValue(ctx, iv, exception_state);
      JS_FreeValue(ctx, iv);

      if (exception_state.HasException()) {
        return {};
      }

      v.emplace_back(item);
    }

    return v;
  }

  static ImplType FromValue(JSContext* ctx, JSValue* array, size_t length, ExceptionState& exception_state) {
    ImplType v;
    v.reserve(length);
    for (uint32_t i = 0; i < length; i++) {
      JSValue iv = array[i];
      auto&& item = Converter<T>::FromValue(ctx, iv, exception_state);
      if (exception_state.HasException()) {
        return {};
      }
      v.emplace_back(item);
    }

    return v;
  };

  static ImplType ArgumentsValue(ExecutingContext* context,
                                 JSValue value,
                                 uint32_t argv_index,
                                 ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    if (JS_IsArray(context->ctx(), value)) {
      return FromValue(context->ctx(), value, exception_state);
    }
    ImplType v;
    exception_state.ThrowException(context->ctx(), ErrorType::TypeError,
                                   ExceptionMessage::ArgumentNotOfType(argv_index, "Array"));
    return v;
  }

  static JSValue ToValue(JSContext* ctx, ImplType value) {
    JSValue array = JS_NewArray(ctx);
    JS_SetPropertyStr(ctx, array, "length", Converter<IDLInt64>::ToValue(ctx, value.size()));
    for (int i = 0; i < value.size(); i++) {
      JS_SetPropertyUint32(ctx, array, i, Converter<T>::ToValue(ctx, value[i]));
    }
    return array;
  }
};

template <typename T>
struct Converter<IDLOptional<IDLSequence<T>>> : public ConverterBase<IDLSequence<T>> {
  using ImplType = typename IDLSequence<typename Converter<T>::ImplType>::ImplType;
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsUndefined(value)) {
      return {};
    }

    return Converter<IDLSequence<T>>::FromValue(ctx, value, exception_state);
  }

  static JSValue ToValue(JSContext* ctx, ImplType value) { return Converter<IDLSequence<T>>::ToValue(ctx, value); }
};

template <typename T>
struct Converter<IDLNullable<IDLSequence<T>>> : public ConverterBase<IDLSequence<T>> {
  using ImplType = typename IDLSequence<typename Converter<T>::ImplType>::ImplType;
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsNull(value)) {
      return {};
    }

    return Converter<IDLSequence<T>>::FromValue(ctx, value, exception_state);
  }
};

template <>
struct Converter<IDLCallback> : public ConverterBase<IDLCallback> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    if (!JS_IsFunction(ctx, value)) {
      return nullptr;
    }

    return QJSFunction::Create(ctx, value);
  }
};


template <>
struct Converter<BlobPart> : public ConverterBase<BlobPart> {
  using ImplType = BlobPart::ImplType;
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    return BlobPart::Create(ctx, value, exception_state);
  }

  static JSValue ToValue(JSContext* ctx, BlobPart* data) {
    if (data == nullptr)
      return JS_NULL;

    return data->ToQuickJS(ctx);
  }
    static JSValue ToValue(JSContext* ctx, std::shared_ptr<BlobPart> data) {
    if (data == nullptr)
      return JS_NULL;

    return data->ToQuickJS(ctx);
  }
};

template <>
struct Converter<BlobPropertyBag> : public ConverterBase<BlobPropertyBag> {
  using ImplType = BlobPropertyBag::ImplType;
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    return BlobPropertyBag::Create(ctx, value, exception_state);
  }
};

template <>
struct Converter<JSEventListener> : public ConverterBase<JSEventListener> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    if (JS_IsObject(value) && !JS_IsFunction(ctx, value)) {
      JSValue handleEventMethod = JS_GetPropertyStr(ctx, value, "handleEvent");

      if (JS_IsFunction(ctx, handleEventMethod)) {
        auto result = JSEventListener::CreateOrNull(QJSFunction::Create(ctx, handleEventMethod, value));
        JS_FreeValue(ctx, handleEventMethod);
        return result;
      }

      return JSEventListener::CreateOrNull(nullptr);
    }
    return JSEventListener::CreateOrNull(QJSFunction::Create(ctx, value));
  }
};

template <>
struct Converter<IDLPromise> : public ConverterBase<IDLPromise> {
  static JSValue ToValue(JSContext* ctx, ImplType value) { return value.ToQuickJS(); }
};

template <>
struct Converter<IDLEventHandler> : public ConverterBase<IDLEventHandler> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    return JSEventHandler::CreateOrNull(ctx, value, JSEventHandler::HandlerType::kEventHandler);
  }

  static JSValue ToValue(JSContext* ctx, ImplType value) {
    if (DynamicTo<JSBasedEventListener>(*value)) {
      return To<JSBasedEventListener>(*value).GetListenerObject();
    }
    return JS_NULL;
  }
};

template <>
struct Converter<IDLNullable<IDLEventHandler>> : public ConverterBase<IDLEventHandler> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsNull(value)) {
      return nullptr;
    }
    assert(!JS_IsException(value));
    return Converter<IDLEventHandler>::FromValue(ctx, value, exception_state);
  }

  static JSValue ToValue(JSContext* ctx, ImplType value) {
    if (value == nullptr) {
      return JS_NULL;
    }

    return Converter<IDLEventHandler>::ToValue(ctx, value);
  }
};

template <>
struct Converter<IDLNullable<JSEventListener>> : public ConverterBase<JSEventListener> {
  static ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsNull(value)) {
      return nullptr;
    }

    if (!JS_IsFunction(ctx, value) && !JS_IsObject(value)) {
      return nullptr;
    }

    assert(!JS_IsException(value));
    return Converter<JSEventListener>::FromValue(ctx, value, exception_state);
  }
};

// DictionaryBase and Derived class.
template <typename T>
struct Converter<T, typename std::enable_if_t<std::is_base_of<DictionaryBase, T>::value>> : public ConverterBase<T> {
  static typename T::ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    return T::Create(ctx, value, exception_state);
  }

  static JSValue ToValue(JSContext* ctx, typename T::ImplType value) {
    if (value == nullptr)
      return JS_NULL;

    return value->toQuickJS(ctx);
  }
};

template <typename T>
struct Converter<T, typename std::enable_if_t<std::is_base_of<UnionBase, T>::value>> : public ConverterBase<T> {
  static typename T::ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    return T::Create(ctx, value, exception_state);
  }
  static typename T::ImplType ArgumentsValue(ExecutingContext* context,
                                             JSValue value,
                                             uint32_t argv_index,
                                             ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    const WrapperTypeInfo* wrapper_type_info = Node::GetStaticWrapperTypeInfo();
    if (JS_IsInstanceOf(context->ctx(), value, context->contextData()->constructorForType(wrapper_type_info))) {
      return FromValue(context->ctx(), value, exception_state);
    }
    exception_state.ThrowException(context->ctx(), ErrorType::TypeError,
                                   ExceptionMessage::ArgumentNotOfType(argv_index, wrapper_type_info->className));
    return nullptr;
  }
  static JSValue ToValue(JSContext* ctx, typename T::ImplType value) {
    if (value == nullptr)
      return JS_NULL;
    return value->ToQuickJSValue(ctx, ASSERT_NO_EXCEPTION());
  }
};

template <typename T>
struct Converter<IDLNullable<T, typename std::enable_if_t<std::is_base_of<UnionBase, T>::value>>>
    : public ConverterBase<IDLNullable<T>> {
  static typename T::ImplType FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsNull(value))
      return nullptr;
    assert(!JS_IsException(value));
    return T::Create(ctx, value, exception_state);
  }
  static typename T::ImplType ArgumentsValue(ExecutingContext* context,
                                             JSValue value,
                                             uint32_t argv_index,
                                             ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    if (JS_IsNull(value))
      return nullptr;
    return FromValue(context->ctx(), value, exception_state);
  }
  static JSValue ToValue(JSContext* ctx, typename T::ImplType value) {
    if (value == nullptr)
      return JS_NULL;
    return value->ToQuickJSValue(ctx, ASSERT_NO_EXCEPTION());
  }
};

// ScriptWrappable and Derived class.
template <typename T>
struct Converter<T, typename std::enable_if_t<std::is_base_of<ScriptWrappable, T>::value>> : public ConverterBase<T> {
  static T* FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    return toScriptWrappable<T>(value);
  }
  static T* ArgumentsValue(ExecutingContext* context,
                           JSValue value,
                           uint32_t argv_index,
                           ExceptionState& exception_state) {
    assert(!JS_IsException(value));
    const WrapperTypeInfo* wrapper_type_info = T::GetStaticWrapperTypeInfo();
    if (JS_IsInstanceOf(context->ctx(), value, context->contextData()->constructorForType(wrapper_type_info))) {
      return FromValue(context->ctx(), value, exception_state);
    }
    exception_state.ThrowException(context->ctx(), ErrorType::TypeError,
                                   ExceptionMessage::ArgumentNotOfType(argv_index, wrapper_type_info->className));
    return nullptr;
  }
  static JSValue ToValue(JSContext* ctx, T* value) {
    if (value == nullptr)
      return JS_NULL;
    return value->ToQuickJS();
  }
  static JSValue ToValue(JSContext* ctx, const T* value) {
    if (value == nullptr)
      return JS_NULL;
    return value->ToQuickJS();
  }
};

template <typename T>
struct Converter<IDLNullable<T, typename std::enable_if_t<std::is_base_of<ScriptWrappable, T>::value>>>
    : ConverterBase<T> {
  static T* FromValue(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
    if (JS_IsNull(value)) {
      return nullptr;
    }
    return Converter<T>::FromValue(ctx, value, exception_state);
  }

  static T* ArgumentsValue(ExecutingContext* context,
                           JSValue value,
                           uint32_t argv_index,
                           ExceptionState& exception_state) {
    if (JS_IsNull(value)) {
      return nullptr;
    }
    return Converter<T>::ArgumentsValue(context, value, argv_index, exception_state);
  }

  static JSValue ToValue(JSContext* ctx, T* value) {
    if (value == nullptr)
      return JS_NULL;
    return Converter<T>::ToValue(ctx, value);
  }

  static JSValue ToValue(JSContext* ctx, const T* value) {
    if (value == nullptr)
      return JS_NULL;
    return Converter<T>::ToValue(ctx, value);
  }
};

};  // namespace webf

#endif  // BRIDGE_BINDINGS_QJS_CONVERTER_IMPL_H_
