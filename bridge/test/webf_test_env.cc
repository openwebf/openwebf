/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include <chrono>
#include <vector>

#include "bindings/qjs/native_string_utils.h"
//#include "core/dom/frame_request_callback_collection.h"
//#include "core/frame/dom_timer.h"
#include "core/page.h"
#include "foundation/native_string.h"
//#include "foundation/native_value_converter.h"
#include "webf_bridge_test.h"
#include "webf_test_env.h"

#if WEBF_QUICKJS_JS_ENGINE
#include "webf_test_context_qjs.h"
#else
#include "webf_test_context_v8.h"
#endif

namespace webf {
class WebFTestContext;

std::unordered_map<int, WebFTestContext*> test_context_map;

#if WEBF_QUICKJS_JS_ENGINE

typedef struct {
  struct list_head link;
  int64_t timeout;
  webf::DOMTimer* timer;
  double contextId;
  bool isInterval;
  AsyncCallback func;
} JSOSTimer;

typedef struct {
  struct list_head link;
  webf::FrameCallback* callback;
  double contextId;
  AsyncRAFCallback handler;
  int32_t callbackId;
} JSFrameCallback;

typedef struct JSThreadState {
  std::unordered_map<int32_t, JSOSTimer*> os_timers; /* list of timer.link */
  std::unordered_map<int32_t, JSFrameCallback*> os_frameCallbacks;
} JSThreadState;

static void unlink_timer(JSThreadState* ts, int32_t timerId) {
  ts->os_timers.erase(timerId);
}

static void unlink_callback(JSThreadState* ts, JSFrameCallback* th) {
  ts->os_frameCallbacks.erase(th->callbackId);
}

#endif

NativeValue* TEST_invokeModule(void* callbackContext,
                               double contextId,
                               int64_t profile_link_id,
                               SharedNativeString* moduleName,
                               SharedNativeString* method,
                               SharedNativeString* params,
                               AsyncModuleCallback callback) {
  std::string module = nativeStringToStdString(moduleName);

  if (module == "throwError") {
    callback(callbackContext, contextId, nativeStringToStdString(method).c_str(), nullptr, nullptr, nullptr);
  }

  if (module == "MethodChannel") {
    NativeValue data = Native_NewCString("{\"result\": 1234}");
    callback(callbackContext, contextId, nullptr, &data, nullptr, nullptr);
  }

  auto* result = static_cast<NativeValue*>(malloc(sizeof(NativeValue)));
  NativeValue tmp = Native_NewCString(module);
  memcpy(result, &tmp, sizeof(NativeValue));
  return result;
};

void TEST_requestBatchUpdate(double contextId){};

void TEST_reloadApp(double contextId) {}

void TEST_setTimeout(int32_t new_timer_id,
                     webf::DOMTimer* timer,
                     double contextId,
                     AsyncCallback callback,
                     int32_t timeout) {
#if WEBF_QUICKJS_JS_ENGINE

  auto* context = timer->context();
  JSRuntime* rt = context->dartIsolateContext()->runtime();
  JSThreadState* ts = static_cast<JSThreadState*>(JS_GetRuntimeOpaque(rt));
  JSOSTimer* th = static_cast<JSOSTimer*>(js_mallocz(context->ctx(), sizeof(*th)));
  auto now = std::chrono::system_clock::now();
  std::time_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  th->timeout = current_time + timeout;
  th->func = callback;
  th->timer = timer;
  th->contextId = contextId;
  th->isInterval = false;

  ts->os_timers[new_timer_id] = th;

#endif
}

void TEST_setInterval(int32_t new_timer_id,
                      webf::DOMTimer* timer,
                      double contextId,
                      AsyncCallback callback,
                      int32_t timeout) {
#if WEBF_QUICKJS_JS_ENGINE

  auto* context = timer->context();
  JSRuntime* rt = context->dartIsolateContext()->runtime();
  JSThreadState* ts = static_cast<JSThreadState*>(JS_GetRuntimeOpaque(rt));
  JSOSTimer* th = static_cast<JSOSTimer*>(js_mallocz(context->ctx(), sizeof(*th)));
  auto now = std::chrono::system_clock::now();
  std::time_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  th->timeout = current_time + timeout;
  th->func = callback;
  th->timer = timer;
  th->contextId = contextId;
  th->isInterval = true;

  ts->os_timers[new_timer_id] = th;

#endif
}

int32_t callbackId = 0;

//void TEST_requestAnimationFrame(int32_t new_id,
//                                webf::FrameCallback* frameCallback,
//                                double contextId,
//                                AsyncRAFCallback handler) {
//#if WEBF_QUICKJS_JS_ENGINE
//  auto* context = frameCallback->context();
//  JSRuntime* rt = context->dartIsolateContext()->runtime();
//  JSThreadState* ts = static_cast<JSThreadState*>(JS_GetRuntimeOpaque(rt));
//  JSFrameCallback* th = static_cast<JSFrameCallback*>(js_mallocz(context->ctx(), sizeof(*th)));
//  th->handler = handler;
//  th->callback = frameCallback;
//  th->contextId = context->contextId();
//  th->callbackId = new_id;
//
//  ts->os_frameCallbacks[new_id] = th;
//
//#endif
//}
//
//void TEST_cancelAnimationFrame(double contextId, int32_t id) {
//#if WEBF_QUICKJS_JS_ENGINE
//  auto* page = test_context_map[contextId]->page();
//  auto* context = page->executingContext();
//  JSThreadState* ts = static_cast<JSThreadState*>(JS_GetRuntimeOpaque(context->dartIsolateContext()->runtime()));
//  ts->os_frameCallbacks.erase(id);
//#endif
//}
//
//void TEST_clearTimeout(double contextId, int32_t timerId) {
//#if WEBF_QUICKJS_JS_ENGINE
//  auto* page = test_context_map[contextId]->page();
//  auto* context = page->executingContext();
//  JSThreadState* ts = static_cast<JSThreadState*>(JS_GetRuntimeOpaque(context->dartIsolateContext()->runtime()));
//  ts->os_timers.erase(timerId);
//#endif
//}

NativeScreen* TEST_getScreen(double contextId) {
  return nullptr;
};

double TEST_devicePixelRatio(double contextId) {
  return 1.0;
}

SharedNativeString* TEST_platformBrightness(double contextId) {
  return nullptr;
}

void TEST_toBlob(void* ptr,
                 double contextId,
                 AsyncBlobCallback blobCallback,
                 int32_t elementId,
                 double devicePixelRatio) {
  uint8_t bytes[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
  blobCallback(ptr, contextId, nullptr, bytes, 5);
}

void TEST_flushUICommand(double contextId) {
  auto* page = test_context_map[contextId]->page();
  clearUICommandItems(reinterpret_cast<void*>(page));
}

void TEST_CreateBindingObject(double context_id, void* native_binding_object, int32_t type, void* args, int32_t argc) {}

void TEST_LoadNativeLibrary(double context_id,
                            SharedNativeString* lib_name,
                            void* initialize_data,
                            void* import_data,
                            LoadNativeLibraryCallback callback) {}

void TEST_GetWidgetElementShape() {}

void TEST_onJsLog(double contextId, int32_t level, const char*) {}

#if ENABLE_PROFILE
NativePerformanceEntryList* TEST_getPerformanceEntries(int32_t) {
  return nullptr;
}
#endif

std::once_flag testInitOnceFlag;
double contextId = -1;

WebFTestEnv::WebFTestEnv(DartIsolateContext* owner_isolate_context, webf::WebFPage* page)
    : page_(page), isolate_context_(owner_isolate_context) {
//  owner_isolate_context->profiler()->StartTrackInitialize();
}

WebFTestEnv::~WebFTestEnv() {
//  isolate_context_->profiler()->FinishTrackInitialize();
  delete isolate_context_;
}

std::unique_ptr<WebFTestEnv> TEST_init(OnJSError onJsError) {
  auto mockedDartMethods = TEST_getMockDartMethods(onJsError);
  auto* dart_isolate_context = initDartIsolateContextSync(0, mockedDartMethods.data(), mockedDartMethods.size(), true);
  double pageContextId = contextId -= 1;
  auto* page = allocateNewPageSync(pageContextId, dart_isolate_context);
  void* testContext = initTestFramework(page);
  test_context_map[pageContextId] = reinterpret_cast<WebFTestContext*>(testContext);
  TEST_mockTestEnvDartMethods(testContext, onJsError);
#if WEBF_QUICKJS_JS_ENGINE
  JS_TurnOnGC(static_cast<DartIsolateContext*>(dart_isolate_context)->runtime());
  JSThreadState* th = new JSThreadState();
  JS_SetRuntimeOpaque(
      reinterpret_cast<WebFTestContext*>(testContext)->page()->executingContext()->dartIsolateContext()->runtime(), th);
#endif
  return std::make_unique<WebFTestEnv>((webf::DartIsolateContext*)dart_isolate_context, (webf::WebFPage*)page);
}

std::unique_ptr<WebFTestEnv> TEST_init() {
  return TEST_init(nullptr);
}

std::unique_ptr<webf::WebFPage> TEST_allocateNewPage(OnJSError onJsError) {
  auto mockedDartMethods = TEST_getMockDartMethods(onJsError);
  auto dart_isolate_context = std::unique_ptr<DartIsolateContext>(
      (DartIsolateContext*)initDartIsolateContextSync(0, mockedDartMethods.data(), mockedDartMethods.size(), true));
  int pageContextId = contextId -= 1;
  auto* page = allocateNewPageSync(pageContextId, dart_isolate_context.get());
  void* testContext = initTestFramework(page);
  test_context_map[pageContextId] = reinterpret_cast<WebFTestContext*>(testContext);

  return std::unique_ptr<webf::WebFPage>(reinterpret_cast<webf::WebFPage*>(page));
}

static bool jsPool(webf::ExecutingContext* context) {
#if WEBF_QUICKJS_JS_ENGINE
  JSRuntime* rt = context->dartIsolateContext()->runtime();
  JSThreadState* ts = static_cast<JSThreadState*>(JS_GetRuntimeOpaque(rt));
  int64_t cur_time, delay;
  struct list_head* el;

  if (ts->os_timers.empty() && ts->os_frameCallbacks.empty())
    return true; /* no more events */

  if (!ts->os_timers.empty()) {
    auto now = std::chrono::system_clock::now();
    cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    for (auto& entry : ts->os_timers) {
      JSOSTimer* th = entry.second;
      delay = th->timeout - cur_time;
      if (delay <= 0) {
        AsyncCallback func;
        /* the timer expired */
        func = th->func;

        if (th->isInterval) {
          func(th->timer, th->contextId, nullptr);
        } else {
          th->func = nullptr;
          int32_t timerId = th->timer->timerId();
          func(th->timer, th->contextId, nullptr);
          unlink_timer(ts, timerId);
        }

        return false;
      }
    }
  }

  if (!ts->os_frameCallbacks.empty()) {
    for (auto& entry : ts->os_frameCallbacks) {
      JSFrameCallback* th = entry.second;
      AsyncRAFCallback handler = th->handler;
      th->handler = nullptr;
      handler(th->callback, th->contextId, 0, nullptr);
      unlink_callback(ts, th);
      return false;
    }
  }

#endif

  return false;
}

void TEST_runLoop(webf::ExecutingContext* context) {
  for (;;) {
    context->DrainMicrotasks();
    if (jsPool(context))
      break;
  }
}

void TEST_onJSLog(double contextId, int32_t level, const char*) {}
void TEST_onMatchImageSnapshot(void* callbackContext,
                               double contextId,
                               uint8_t* bytes,
                               int32_t length,
                               SharedNativeString* name,
                               MatchImageSnapshotCallback callback) {
  callback(callbackContext, contextId, 1, nullptr);
}

void TEST_onMatchImageSnapshotBytes(void* callback_context,
                                    double context_id,
                                    uint8_t* image_a_bytes,
                                    int32_t image_a_size,
                                    uint8_t* image_b_bytes,
                                    int32_t image_b_size,
                                    MatchImageSnapshotCallback callback) {
  callback(callback_context, contextId, 1, nullptr);
}

const char* TEST_environment() {
  return "";
}

void TEST_simulatePointer(MousePointer*, int32_t length, int32_t pointer) {}

void TEST_simulateInputText(SharedNativeString* nativeString) {}

std::vector<uint64_t> TEST_getMockDartMethods(OnJSError onJSError) {
  std::vector<uint64_t> mockMethods{reinterpret_cast<uint64_t>(TEST_invokeModule),
                                    reinterpret_cast<uint64_t>(TEST_requestBatchUpdate),
                                    reinterpret_cast<uint64_t>(TEST_reloadApp),
                                    reinterpret_cast<uint64_t>(TEST_setTimeout),
                                    reinterpret_cast<uint64_t>(TEST_setInterval),
//                                    reinterpret_cast<uint64_t>(TEST_clearTimeout),
//                                    reinterpret_cast<uint64_t>(TEST_requestAnimationFrame),
//                                    reinterpret_cast<uint64_t>(TEST_cancelAnimationFrame),
                                    reinterpret_cast<uint64_t>(TEST_toBlob),
                                    reinterpret_cast<uint64_t>(TEST_flushUICommand),
                                    reinterpret_cast<uint64_t>(TEST_CreateBindingObject),
                                    reinterpret_cast<uint64_t>(TEST_LoadNativeLibrary),
                                    reinterpret_cast<uint64_t>(TEST_GetWidgetElementShape)};

  WEBF_LOG(VERBOSE) << " ON JS ERROR" << onJSError;
  mockMethods.emplace_back(reinterpret_cast<uint64_t>(onJSError));
  mockMethods.emplace_back(reinterpret_cast<uint64_t>(TEST_onJsLog));
  return mockMethods;
}

void TEST_mockTestEnvDartMethods(void* testContext, OnJSError onJSError) {
  std::vector<uint64_t> mockMethods{
      reinterpret_cast<uint64_t>(onJSError),
      reinterpret_cast<uint64_t>(TEST_onMatchImageSnapshot),
      reinterpret_cast<uint64_t>(TEST_onMatchImageSnapshotBytes),
      reinterpret_cast<uint64_t>(TEST_environment),
      reinterpret_cast<uint64_t>(TEST_simulatePointer),
      reinterpret_cast<uint64_t>(TEST_simulateInputText),
  };

  registerTestEnvDartMethods(testContext, mockMethods.data(), mockMethods.size());
}

std::unordered_map<int32_t, std::shared_ptr<UnitTestEnv>> unitTestEnvMap;
std::shared_ptr<UnitTestEnv> TEST_getEnv(int32_t contextUniqueId) {
  if (unitTestEnvMap.count(contextUniqueId) == 0) {
    unitTestEnvMap[contextUniqueId] = std::make_shared<UnitTestEnv>();
  }

  return unitTestEnvMap[contextUniqueId];
}

void TEST_registerEventTargetDisposedCallback(int32_t context_unique_id, TEST_OnEventTargetDisposed callback) {
  if (unitTestEnvMap.count(context_unique_id) == 0) {
    unitTestEnvMap[context_unique_id] = std::make_shared<UnitTestEnv>();
  }

  unitTestEnvMap[context_unique_id]->on_event_target_disposed = callback;
}

}  // namespace webf
