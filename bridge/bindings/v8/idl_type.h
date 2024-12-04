/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef BRIDGE_BINDINGS_V8_CONVERTER_TS_TYPE_H_
#define BRIDGE_BINDINGS_V8_CONVERTER_TS_TYPE_H_

#include <vector>
#include "bindings/qjs/converter.h"
#include <v8/v8.h>

namespace webf {

struct IDLTypeBase {
  using ImplType = void;
};

template <typename T>
struct IDLTypeBaseHelper {
  using ImplType = T;
};

template <typename T>
struct IDLOptional final : public IDLTypeBase {
  using ImplType = typename Converter<T>::ImplType;
};

struct IDLInt64 final : public IDLTypeBaseHelper<int64_t> {};
struct IDLDouble final : public IDLTypeBaseHelper<double> {};

// Function callback
struct IDLCallback : public IDLTypeBaseHelper<v8::MaybeLocal<v8::Function>> {
  using ImplType = typename Converter<v8::MaybeLocal<v8::Function>>::ImplType;
};

}  // namespace webf

#endif  // BRIDGE_BINDINGS_V8_CONVERTER_TS_TYPE_H_
