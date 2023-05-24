/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include <atomic>
#include <cassert>
#include <thread>

#include "bindings/qjs/native_string_utils.h"
#include "core/dart_context.h"
#include "core/page.h"
#include "foundation/logging.h"
#include "foundation/ui_command_buffer.h"
#include "include/webf_bridge.h"

#if defined(_WIN32)
#define SYSTEM_NAME "windows"  // Windows
#elif defined(_WIN64)
#define SYSTEM_NAME "windows"  // Windows
#elif defined(__CYGWIN__) && !defined(_WIN32)
#define SYSTEM_NAME "windows"  // Windows (Cygwin POSIX under Microsoft Window)
#elif defined(__ANDROID__)
#define SYSTEM_NAME "android"  // Android (implies Linux, so it must come first)
#elif defined(__linux__)
#define SYSTEM_NAME "linux"                    // Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other
#elif defined(__APPLE__) && defined(__MACH__)  // Apple OSX and iOS (Darwin)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1
#define SYSTEM_NAME "ios"  // Apple iOS Simulator
#elif TARGET_OS_IPHONE == 1
#define SYSTEM_NAME "ios"  // Apple iOS
#elif TARGET_OS_MAC == 1
#define SYSTEM_NAME "macos"  // Apple macOS
#endif
#else
#define SYSTEM_NAME "unknown"
#endif

thread_local webf::DartContext* dart_context_ = nullptr;

void* initDartIsolateContext(uint64_t* dart_methods, int32_t dart_methods_len) {
  if (dart_context_ == nullptr) {
    dart_context_ = new webf::DartContext();
  }

  auto dart_isolate_context = std::make_unique<webf::DartIsolateContext>(dart_context_, dart_methods, dart_methods_len);
  auto* ptr = dart_isolate_context.get();
  dart_context_->AddIsolate(std::move(dart_isolate_context));
  return ptr;
}

void* allocateNewPage(void* dart_isolate_context, int32_t targetContextId) {
  assert(dart_isolate_context != nullptr);
  auto page =
      std::make_unique<webf::WebFPage>((webf::DartIsolateContext*)dart_isolate_context, targetContextId, nullptr);
  void* ptr = page.get();
  ((webf::DartIsolateContext*)dart_isolate_context)->AddNewPage(std::move(page));
  return ptr;
}

void disposePage(void* dart_isolate_context, void* page_) {
  auto* page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());
  ((webf::DartIsolateContext*)dart_isolate_context)->RemovePage(page);
}

int8_t evaluateScripts(void* page_,
                       SharedNativeString* code,
                       uint8_t** parsed_bytecodes,
                       uint64_t* bytecode_len,
                       const char* bundleFilename,
                       int32_t startLine) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());
  return page->evaluateScript(reinterpret_cast<webf::SharedNativeString*>(code), parsed_bytecodes, bytecode_len,
                              bundleFilename, startLine)
             ? 1
             : 0;
}

int8_t evaluateQuickjsByteCode(void* page_, uint8_t* bytes, int32_t byteLen) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());
  return page->evaluateByteCode(bytes, byteLen) ? 1 : 0;
}

void parseHTML(void* page_, const char* code, int32_t length) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());
  page->parseHTML(code, length);
}

NativeValue* invokeModuleEvent(void* page_,
                               SharedNativeString* module_name,
                               const char* eventType,
                               void* event,
                               NativeValue* extra) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());
  auto* result = page->invokeModuleEvent(reinterpret_cast<webf::SharedNativeString*>(module_name), eventType, event,
                                         reinterpret_cast<webf::NativeValue*>(extra));
  return reinterpret_cast<NativeValue*>(result);
}

static WebFInfo* webfInfo{nullptr};

WebFInfo* getWebFInfo() {
  if (webfInfo == nullptr) {
    webfInfo = new WebFInfo();
    webfInfo->app_name = "WebF";
    webfInfo->app_revision = APP_REV;
    webfInfo->app_version = APP_VERSION;
    webfInfo->system_name = SYSTEM_NAME;
  }

  return webfInfo;
}

void dispatchUITask(void* page_, void* context, void* callback) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());
  reinterpret_cast<void (*)(void*)>(callback)(context);
}

void* getUICommandItems(void* page_) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());
  return page->GetExecutingContext()->uiCommandBuffer()->data();
}

int64_t getUICommandItemSize(void* page_) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());
  return page->GetExecutingContext()->uiCommandBuffer()->size();
}

void clearUICommandItems(void* page_) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  assert(std::this_thread::get_id() == page->currentThread());
  page->GetExecutingContext()->uiCommandBuffer()->clear();
}

void registerPluginByteCode(uint8_t* bytes, int32_t length, const char* pluginName) {
  webf::ExecutingContext::plugin_byte_code[pluginName] = webf::NativeByteCode{bytes, length};
}

void registerPluginCode(const char* code, int32_t length, const char* pluginName) {
  webf::ExecutingContext::plugin_string_code[pluginName] = std::string(code, length);
}

int32_t profileModeEnabled() {
#if ENABLE_PROFILE
  return 1;
#else
  return 0;
#endif
}

// Callbacks when dart context object was finalized by Dart GC.
static void finalize_dart_context(void* isolate_callback_data, void* peer) {
  auto* dart_isolate_context = (webf::DartIsolateContext*) peer;
  auto* dart_context = dart_isolate_context->dartContext();
  ((webf::DartIsolateContext*) peer)->dartContext()->RemoveIsolate(dart_isolate_context);
  // Remove the whole dart context if there are no dart isolates alive.
  if (dart_context->IsIsolateEmpty()) {
    delete dart_context;
    dart_context_ = nullptr;
  }
}

void init_dart_dynamic_linking(void* data) {
  if (Dart_InitializeApiDL(data) != 0) {
    printf("Failed to initialize dart VM API\n");
  }
}

void register_dart_context_finalizer(Dart_Handle dart_handle, void* dart_isolate_context) {
  Dart_NewFinalizableHandle_DL(dart_handle, reinterpret_cast<void*>(dart_isolate_context), sizeof(webf::DartContext),
                                                     finalize_dart_context);
}
