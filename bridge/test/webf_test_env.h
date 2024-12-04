/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_TEST_WEBF_TEST_ENV_H_
#define BRIDGE_TEST_WEBF_TEST_ENV_H_

#include <memory>
//#include "bindings/qjs/cppgc/mutation_scope.h"
#include "core/dart_methods.h"
#include "core/executing_context.h"
#include "core/page.h"
#include "foundation/logging.h"

using namespace webf;

// Trigger a callbacks before GC free the eventTargets.
//using TEST_OnEventTargetDisposed = void (*)(EventTarget* event_target);
//struct UnitTestEnv {
//  TEST_OnEventTargetDisposed on_event_target_disposed{nullptr};
//};

// Mock dart methods and add async timer to emulate webf environment in C++ unit test.

namespace webf {

class WebFTestEnv {
 public:
  WebFTestEnv(DartIsolateContext* owner_isolate_context, webf::WebFPage* page);
  ~WebFTestEnv();

  webf::WebFPage* page() { return page_; }

 private:
  webf::WebFPage* page_;
  webf::DartIsolateContext* isolate_context_;
};

std::unique_ptr<WebFTestEnv> TEST_init(OnJSError onJsError);
std::unique_ptr<WebFTestEnv> TEST_init();
std::unique_ptr<WebFPage> TEST_allocateNewPage(OnJSError onJsError);
void TEST_runLoop(ExecutingContext* context);
std::vector<uint64_t> TEST_getMockDartMethods(OnJSError onJSError);
void TEST_mockTestEnvDartMethods(void* testContext, OnJSError onJSError);
//void TEST_registerEventTargetDisposedCallback(int32_t context_unique_id, TEST_OnEventTargetDisposed callback);
//std::shared_ptr<UnitTestEnv> TEST_getEnv(int32_t context_unique_id);
}  // namespace webf
   // void TEST_dispatchEvent(int32_t contextId, EventTarget* eventTarget, const std::string type);
   // void TEST_callNativeMethod(void* nativePtr, void* returnValue, void* method, int32_t argc, void* argv);

#endif  // BRIDGE_TEST_WEBF_TEST_ENV_H_
