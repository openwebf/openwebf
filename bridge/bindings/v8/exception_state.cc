/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "exception_state.h"
#include "bindings/v8/v8_throw_exception.h"

namespace webf {

void ExceptionState::ThrowException(v8::Isolate* isolate, ErrorType type, const std::string& message) {
  switch (type) {
    case ErrorType::TypeError:
      V8ThrowException::ThrowError(isolate, message);
      break;
    case InternalError:
      // TODO match InternalError
      V8ThrowException::ThrowError(isolate, message);
      break;
    case RangeError:
      V8ThrowException::ThrowRangeError(isolate, message);
      break;
    case ReferenceError:
      V8ThrowException::ThrowReferenceError(isolate, message);
      break;
    case SyntaxError:
      V8ThrowException::ThrowSyntaxError(isolate, message);
      break;
  }
  didThrowException_ = true;
}


bool ExceptionState::HasException() {
  return didThrowException_;
}

}  // namespace webf
