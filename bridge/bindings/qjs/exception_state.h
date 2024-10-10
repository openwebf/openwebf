/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_EXCEPTION_STATE_H
#define BRIDGE_EXCEPTION_STATE_H

#include <quickjs/quickjs.h>
#include <string>
#include "foundation/macros.h"

#define ASSERT_NO_EXCEPTION() ExceptionState().ReturnThis()

namespace webf {

enum ErrorType { TypeError, InternalError, RangeError, ReferenceError, SyntaxError,ArgumentError };

// ExceptionState is a scope-like class and provides a way to store an exception.
class ExceptionState {
  // ExceptionState should only allocate at stack.
  WEBF_DISALLOW_NEW();

 public:
  void ThrowException(JSContext* ctx, ErrorType type, const std::string& message);
  void ThrowException(JSContext* ctx, JSValue exception);
  bool HasException();

  ExceptionState& ReturnThis();

  JSValue ToQuickJS();
  static JSValue CurrentException(JSContext* ctx);

 private:
  JSValue exception_{JS_NULL};
};

}  // namespace webf

#endif  // BRIDGE_EXCEPTION_STATE_H
