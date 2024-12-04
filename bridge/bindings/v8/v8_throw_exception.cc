/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "v8_throw_exception.h"
#include <v8/v8-exception.h>

namespace webf {

#define DEFINE_CREATE_AND_THROW_ERROR_FUNC(webfErrorType, v8ErrorType,  \
                                           defaultMessage)               \
  v8::Local<v8::Value> V8ThrowException::Create##webfErrorType(         \
      v8::Isolate* isolate, const std::string& message) {                \
    std::string defaultMsgStr = defaultMessage;                          \
    return v8::Exception::v8ErrorType(                                   \
        v8::String::NewFromUtf8(isolate,                                \
                               message.empty() ? defaultMsgStr.c_str() : message.c_str(), \
                               v8::NewStringType::kNormal).ToLocalChecked()); \
  }                                                                      \
                                                                         \
  void V8ThrowException::Throw##webfErrorType(v8::Isolate* isolate,     \
                                               const std::string& message) {  \
    ThrowException(isolate, Create##webfErrorType(isolate, message));   \
  }

DEFINE_CREATE_AND_THROW_ERROR_FUNC(Error, Error, "Error")
DEFINE_CREATE_AND_THROW_ERROR_FUNC(RangeError, RangeError, "Range error")
DEFINE_CREATE_AND_THROW_ERROR_FUNC(ReferenceError,
                                   ReferenceError,
                                   "Reference error")
DEFINE_CREATE_AND_THROW_ERROR_FUNC(SyntaxError, SyntaxError, "Syntax error")
DEFINE_CREATE_AND_THROW_ERROR_FUNC(TypeError, TypeError, "Type error")

#undef DEFINE_CREATE_AND_THROW_ERROR_FUNC

}  // namespace webf