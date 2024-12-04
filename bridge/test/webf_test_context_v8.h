/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_WEBF_TEST_CONTEXT_H
#define BRIDGE_WEBF_TEST_CONTEXT_H


#include "core/executing_context.h"
#include "core/page.h"
#include "webf_bridge_test.h"

namespace webf {

//struct ImageSnapShotContext {
//  JSValue callback;
//  ExecutingContext* context;
//  list_head link;
//};

class WebFTestContext final {
 public:
  explicit WebFTestContext() = delete;
  explicit WebFTestContext(ExecutingContext* context);
  ~WebFTestContext();

  /// Evaluate JavaScript source code with build-in test frameworks, use in test only.
  bool parseTestHTML(const uint16_t* code, size_t codeLength);
  void invokeExecuteTest(Dart_PersistentHandle persistent_handle, ExecuteResultCallback executeCallback);
  void registerTestEnvDartMethods(uint64_t* methodBytes, int32_t length);

  WebFPage* page() const { return page_; }

//  std::shared_ptr<QJSFunction> execute_test_callback_{nullptr};
//  JSValue execute_test_proxy_object_{JS_NULL};

 private:
  /// the pointer of JSContext, ownership belongs to JSContext
  ExecutingContext* context_{nullptr};
  WebFPage* page_;
};

}  // namespace webf

#endif  // BRIDGE_WEBF_TEST_CONTEXT_H
