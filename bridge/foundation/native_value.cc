/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include "native_value.h"
#if WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/qjs_engine_patch.h"
#include "bindings/qjs/script_value.h"
#endif
#include "core/executing_context.h"

namespace webf {

NativeValue Native_NewNull() {
#ifdef _MSC_VER
  NativeValue value{};
  value.u.int64 = 0;
  value.uint32 = 0;
  value.tag = NativeTag::TAG_NULL;
  return value;
#else
  return (NativeValue){.u = {.int64 = 0}, .uint32 = 0, .tag = NativeTag::TAG_NULL};
#endif
}

NativeValue Native_NewString(SharedNativeString* string) {
#ifdef _MSC_VER
  NativeValue value{};
  value.u.ptr = static_cast<void*>(string);
  value.uint32 = 0;
  value.tag = static_cast<int32_t>(NativeTag::TAG_STRING);
  return value;
#else
  return (NativeValue){
      .u = {.ptr = static_cast<void*>(string)},
      .uint32 = 0,
      .tag = NativeTag::TAG_STRING,
  };
#endif
}

NativeValue Native_NewCString(const std::string& string) {
  std::unique_ptr<SharedNativeString> nativeString = stringToNativeString(string);
  // NativeString owned by NativeValue will be freed by users.
  return Native_NewString(nativeString.release());
}

NativeValue Native_NewFloat64(double value) {
  int64_t result;
  memcpy(&result, reinterpret_cast<void*>(&value), sizeof(double));

#ifdef _MSC_VER
  NativeValue v{};
  v.u.int64 = result;
  v.uint32 = 0;
  v.tag = NativeTag::TAG_FLOAT64;
  return v;
#else
  return (NativeValue){
      .u = {.int64 = result},
      .uint32 = 0,
      .tag = NativeTag::TAG_FLOAT64,
  };
#endif
}

NativeValue Native_NewPtr(JSPointerType pointerType, void* ptr) {
#if _MSC_VER
  NativeValue v{};
  v.u.ptr = ptr;
  v.uint32 = static_cast<uint32_t>(pointerType);
  v.tag = NativeTag::TAG_POINTER;
  return v;
#else
  return (NativeValue){.u = {.ptr = ptr}, .uint32 = static_cast<uint32_t>(pointerType), .tag = NativeTag::TAG_POINTER};
#endif
}

NativeValue Native_NewBool(bool value) {
#if _MSC_VER
  NativeValue v{};
  v.u.int64 = value ? 1 : 0;
  v.uint32 = 0;
  v.tag = NativeTag::TAG_BOOL;
  return v;
#else
  return (NativeValue){
      .u = {.int64 = value ? 1 : 0},
      .uint32 = 0,
      .tag = NativeTag::TAG_BOOL,
  };
#endif
}

NativeValue Native_NewInt64(int64_t value) {
#if _MSC_VER
  NativeValue v{};
  v.u.int64 = value;
  v.uint32 = 0;
  v.tag = NativeTag::TAG_INT;
  return v;
#else
  return (NativeValue){
      .u = {.int64 = value},
      .uint32 = 0,
      .tag = NativeTag::TAG_INT,
  };
#endif
}

NativeValue Native_NewList(uint32_t argc, NativeValue* argv) {
#if _MSC_VER
  NativeValue v{};
  v.u.ptr = reinterpret_cast<void*>(argv);
  v.uint32 = argc;
  v.tag = NativeTag::TAG_LIST;
  return v;
#else
  return (NativeValue){.u = {.ptr = reinterpret_cast<void*>(argv)}, .uint32 = argc, .tag = NativeTag::TAG_LIST};
#endif
}

#if WEBF_QUICKJS_JS_ENGINE

NativeValue Native_NewJSON(JSContext* ctx, const ScriptValue& value, ExceptionState& exception_state) {
  ScriptValue json = value.ToJSONStringify(ctx, &exception_state);
  if (exception_state.HasException()) {
    return Native_NewNull();
  }

  auto native_string = json.ToNativeString(ctx);

#if _MSC_VER
  NativeValue v{};
  v.u.ptr = static_cast<void*>(native_string.release());
  v.uint32 = 0;
  v.tag = NativeTag::TAG_JSON;
  return v;
#else
  NativeValue result = (NativeValue){
      .u = {.ptr = static_cast<void*>(native_string.release())},
      .uint32 = 0,
      .tag = NativeTag::TAG_JSON,
  };
  return result;
#endif
}

#endif

JSPointerType GetPointerTypeOfNativePointer(NativeValue native_value) {
  assert(native_value.tag == NativeTag::TAG_POINTER);
  return static_cast<JSPointerType>(native_value.uint32);
}

}  // namespace webf
