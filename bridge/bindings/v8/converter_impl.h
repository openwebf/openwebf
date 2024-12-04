/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef BRIDGE_BINDINGS_V8_CONVERTER_IMPL_H_
#define BRIDGE_BINDINGS_V8_CONVERTER_IMPL_H_

#include "bindings/v8/idl_type.h"
#include "bindings/v8/exception_state.h"

namespace webf {

// Int64
template <>
struct Converter<IDLInt64> : public ConverterBase<IDLInt64> {
  static ImplType FromValue(v8::Isolate* isolate, v8::Local<v8::Value> arg, ExceptionState& exception_state) {
    if (!arg->IsNumber()) {
      return 0;
    }

    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    return static_cast<int64_t>(arg->IntegerValue(context).ToChecked());
  }

  static v8::Local<v8::Value> ToValue(v8::Isolate* isolate, uint32_t v) {
    return v8::BigInt::New(isolate, v);
  }
};

template <>
struct Converter<IDLDouble> : public ConverterBase<IDLDouble> {
  static double FromValue(v8::Isolate* isolate, v8::Local<v8::Value> arg, ExceptionState& exception_state) {
    if (!arg->IsNumber()) {
      return 0;
    }

    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    return arg->NumberValue(context).ToChecked();
  }

  static v8::Local<v8::Value> ToValue(v8::Isolate* isolate, double v) {
    return v8::Number::New(isolate, v);
  }
};

// Optional value for arithmetic value
template <typename T>
struct Converter<IDLOptional<T>, std::enable_if_t<std::is_arithmetic<typename Converter<T>::ImplType>::value>>
    : public ConverterBase<IDLOptional<T>> {
  using ImplType = typename Converter<T>::ImplType;

  static ImplType FromValue(v8::Isolate* isolate, v8::Local<v8::Value> value, ExceptionState& exception) {
    if (value.IsEmpty() || value->IsUndefined()) {
      return 0;
    }

    return Converter<T>::FromValue(isolate, value, exception);
  }

  static v8::Local<v8::Value> ToValue(v8::Isolate* isolate, typename Converter<T>::ImplType value) {
    return Converter<T>::ToValue(isolate, value);
  }
};

template <>
struct Converter<IDLCallback> : public ConverterBase<IDLCallback> {
  static ImplType FromValue(v8::Isolate* isolate, v8::Local<v8::Value> arg, ExceptionState& exception_state) {
    if (!arg->IsFunction()) {
      return {};
    }

    return v8::Local<v8::Function>::Cast(arg);
  }
};

};  // namespace webf

#endif  // BRIDGE_BINDINGS_V8_CONVERTER_IMPL_H_
