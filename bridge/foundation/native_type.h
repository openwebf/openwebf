/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_FOUNDATION_NATIVE_TYPE_H_
#define BRIDGE_FOUNDATION_NATIVE_TYPE_H_

#include <type_traits>
#include <vector>
#include "bindings/qjs/qjs_function.h"
#include "bindings/qjs/script_value.h"
#include "foundation/native_string.h"

namespace webf {

struct NativeTypeBase {
  using ImplType = void;
};

template <typename T>
struct NativeTypeBaseHelper {
  using ImplType = T;
};

// Null
struct NativeTypeNull final : public NativeTypeBaseHelper<ScriptValue> {};

// Bool
struct NativeTypeBool final : public NativeTypeBaseHelper<bool> {};

// String
struct NativeTypeString final : public NativeTypeBaseHelper<AtomicString> {};

// Int64
struct NativeTypeInt64 final : public NativeTypeBaseHelper<int64_t> {};

// Double
struct NativeTypeDouble final : public NativeTypeBaseHelper<double> {};

// JSON
struct NativeTypeJSON final : public NativeTypeBaseHelper<ScriptValue> {};

struct NativeTypeBytes final : public NativeTypeBaseHelper<uint8_t*> {};

// Array
template <typename T>
struct NativeTypeArray final : public NativeTypeBase {
  using ImplType = typename std::vector<T>;
};

// Pointer
template <typename T>
struct NativeTypePointer final : public NativeTypeBaseHelper<T*> {};

// Sync function
struct NativeTypeFunction final : public NativeTypeBaseHelper<std::shared_ptr<QJSFunction>> {};

// Async function
struct NativeTypeAsyncFunction final : public NativeTypeBaseHelper<std::shared_ptr<QJSFunction>> {};

}  // namespace webf

#endif  // BRIDGE_FOUNDATION_NATIVE_TYPE_H_
