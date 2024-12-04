/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_V8_THROW_EXCEPTION_H
#define WEBF_V8_THROW_EXCEPTION_H

#include <v8/v8-forward.h>
#include <v8/v8-isolate.h>
#include <v8/v8-local-handle.h>
#include "foundation/macros.h"

namespace webf {

// Provides utility functions to create and/or throw JS built-in errors.
class V8ThrowException {
  WEBF_STATIC_ONLY(V8ThrowException);

 public:
  static void ThrowException(v8::Isolate* isolate,
                             v8::Local<v8::Value> exception) {
    if (!isolate->IsExecutionTerminating())
      isolate->ThrowException(exception);
  }

  static v8::Local<v8::Value> CreateError(v8::Isolate*, const std::string& message);
  static v8::Local<v8::Value> CreateRangeError(v8::Isolate*,
                                               const std::string& message);
  static v8::Local<v8::Value> CreateReferenceError(v8::Isolate*,
                                                   const std::string& message);
  static v8::Local<v8::Value> CreateSyntaxError(v8::Isolate*,
                                                const std::string& message);
  static v8::Local<v8::Value> CreateTypeError(v8::Isolate*,
                                              const std::string& message);

  static void ThrowError(v8::Isolate*, const std::string& message);
  static void ThrowRangeError(v8::Isolate*, const std::string& message);
  static void ThrowReferenceError(v8::Isolate*, const std::string& message);
  static void ThrowSyntaxError(v8::Isolate*, const std::string& message);
  static void ThrowTypeError(v8::Isolate*, const std::string& message);
};

}  // namespace webf

#endif  // WEBF_V8_THROW_EXCEPTION_H
