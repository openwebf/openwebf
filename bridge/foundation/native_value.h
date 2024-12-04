/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_NATIVE_VALUE_H
#define BRIDGE_NATIVE_VALUE_H

#if WEBF_QUICKJS_JS_ENGINE
#include <quickjs/list.h>
#include <quickjs/quickjs.h>
#include "bindings/qjs/native_string_utils.h"
#elif WEBF_V8_JS_ENGINE

#endif
#include <cinttypes>
#include <string>
#include "foundation/dart_readable.h"

namespace webf {

enum NativeTag {
  TAG_STRING = 0,
  TAG_INT = 1,
  TAG_BOOL = 2,
  TAG_NULL = 3,
  TAG_FLOAT64 = 4,
  TAG_JSON = 5,
  TAG_LIST = 6,
  TAG_POINTER = 7,
  TAG_FUNCTION = 8,
  TAG_ASYNC_FUNCTION = 9,
  TAG_UINT8_BYTES = 10,
};

enum class JSPointerType { NativeBindingObject = 0, Others = 1 };

class ExecutingContext;
class ExceptionState;
class ScriptValue;

// Exchange data struct between dart and C++
struct NativeValue : public DartReadable {
  union {
    int64_t int64;
    double float64;
    void* ptr;
  } u;
  uint32_t uint32;
  int32_t tag;
};

struct NativeFunctionContext;

using CallNativeFunction = void (*)(NativeFunctionContext* functionContext,
                                    int32_t argc,
                                    NativeValue* argv,
                                    NativeValue* returnValue);

static void call_native_function(NativeFunctionContext* functionContext,
                                 int32_t argc,
                                 NativeValue* argv,
                                 NativeValue* returnValue);

#if WEBF_QUICKJS_JS_ENGINE
struct NativeFunctionContext {
  CallNativeFunction call;
  NativeFunctionContext(ExecutingContext* context, JSValue callback);
  ~NativeFunctionContext();
  JSValue m_callback{JS_NULL};
  ExecutingContext* m_context{nullptr};
  JSContext* m_ctx{nullptr};
  list_head link;
};
#endif

NativeValue Native_NewNull();
NativeValue Native_NewString(SharedNativeString* string);
NativeValue Native_NewCString(const std::string& string);
NativeValue Native_NewFloat64(double value);
NativeValue Native_NewBool(bool value);
NativeValue Native_NewInt64(int64_t value);
NativeValue Native_NewList(uint32_t argc, NativeValue* argv);
NativeValue Native_NewPtr(JSPointerType pointerType, void* ptr);
#if WEBF_QUICKJS_JS_ENGINE
NativeValue Native_NewJSON(JSContext* ctx, const ScriptValue& value, ExceptionState& exception_state);
#endif

JSPointerType GetPointerTypeOfNativePointer(NativeValue native_value);

}  // namespace webf

#endif  // BRIDGE_NATIVE_VALUE_H
