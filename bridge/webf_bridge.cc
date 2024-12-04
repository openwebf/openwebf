/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "include/webf_bridge.h"
#include "core/dart_isolate_context.h"
#include "core/html/parser/html_parser.h"
#include "core/page.h"
#include "foundation/native_type.h"
#include "include/dart_api.h"
#include "multiple_threading/dispatcher.h"
#include "multiple_threading/task.h"

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

static std::atomic<int64_t> unique_page_id{1};

int64_t newPageIdSync() {
  return unique_page_id++;
}

void* initDartIsolateContextSync(int64_t dart_port,
                                 uint64_t* dart_methods,
                                 int32_t dart_methods_len,
                                 int8_t enable_profile) {
  auto dispatcher = std::make_unique<webf::multi_threading::Dispatcher>(dart_port);

#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: initDartIsolateContextSync Call BEGIN";
#endif
  auto* dart_isolate_context = new webf::DartIsolateContext(dart_methods, dart_methods_len, enable_profile == 1);
  dart_isolate_context->SetDispatcher(std::move(dispatcher));

#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: initDartIsolateContextSync Call END";
#endif

  return dart_isolate_context;
}

void* allocateNewPageSync(double thread_identity, void* ptr) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: allocateNewPageSync Call BEGIN";
#endif
  auto* dart_isolate_context = (webf::DartIsolateContext*)ptr;
  assert(dart_isolate_context != nullptr);

  void* result = static_cast<webf::DartIsolateContext*>(dart_isolate_context)->AddNewPageSync(thread_identity);
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: allocateNewPageSync Call END";
#endif

  return result;
}

void allocateNewPage(double thread_identity,
                     int32_t sync_buffer_size,
                     void* ptr,
                     Dart_Handle dart_handle,
                     AllocateNewPageCallback result_callback) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: allocateNewPage Call BEGIN";
#endif
  auto* dart_isolate_context = (webf::DartIsolateContext*)ptr;
  assert(dart_isolate_context != nullptr);
  Dart_PersistentHandle persistent_handle = Dart_NewPersistentHandle_DL(dart_handle);

  static_cast<webf::DartIsolateContext*>(dart_isolate_context)
      ->AddNewPage(thread_identity, sync_buffer_size, persistent_handle, result_callback);
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: allocateNewPage Call END";
#endif
}

void disposePage(double thread_identity,
                 void* ptr,
                 void* page_,
                 Dart_Handle dart_handle,
                 DisposePageCallback result_callback) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: disposePage Call BEGIN";
#endif

  auto* dart_isolate_context = (webf::DartIsolateContext*)ptr;

  Dart_PersistentHandle persistent_handle = Dart_NewPersistentHandle_DL(dart_handle);

  ((webf::DartIsolateContext*)dart_isolate_context)
      ->RemovePage(thread_identity, static_cast<webf::WebFPage*>(page_), persistent_handle, result_callback);
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: disposePage Call END";
#endif
}

void disposePageSync(double thread_identity, void* ptr, void* page_) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: disposePageSync Call BEGIN";
#endif
  auto* dart_isolate_context = (webf::DartIsolateContext*)ptr;
  ((webf::DartIsolateContext*)dart_isolate_context)
      ->RemovePageSync(thread_identity, static_cast<webf::WebFPage*>(page_));
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher]: disposePageSync Call END";
#endif
}

void evaluateScripts(void* page_,
                     const char* code,
                     uint64_t code_len,
                     uint8_t** parsed_bytecodes,
                     uint64_t* bytecode_len,
                     const char* bundleFilename,
                     int32_t start_line,
                     int64_t profile_id,
                     Dart_Handle dart_handle,
                     EvaluateScriptsCallback result_callback) {
#if ENABLE_LOG
  WEBF_LOG(VERBOSE) << "[Dart] evaluateScriptsWrapper call" << std::endl;
#endif
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  Dart_PersistentHandle persistent_handle = Dart_NewPersistentHandle_DL(dart_handle);
  page->executingContext()->dartIsolateContext()->dispatcher()->PostToJs(
      page->isDedicated(), static_cast<int32_t>(page->contextId()), webf::WebFPage::EvaluateScriptsInternal, page_,
      code, code_len, parsed_bytecodes, bytecode_len, bundleFilename, start_line, profile_id, persistent_handle,
      result_callback);
}

void dumpQuickjsByteCode(void* page_,
                         int64_t profile_id,
                         const char* code,
                         int32_t code_len,
                         uint8_t** parsed_bytecodes,
                         uint64_t* bytecode_len,
                         const char* url,
                         Dart_Handle dart_handle,
                         DumpQuickjsByteCodeCallback result_callback) {
#if ENABLE_LOG
  WEBF_LOG(VERBOSE) << "[Dart] dumpQuickjsByteCode call" << std::endl;
#endif

  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  Dart_PersistentHandle persistent_handle = Dart_NewPersistentHandle_DL(dart_handle);
  page->dartIsolateContext()->dispatcher()->PostToJs(
      page->isDedicated(), static_cast<int32_t>(page->contextId()), webf::WebFPage::DumpQuickJsByteCodeInternal, page,
      profile_id, code, code_len, parsed_bytecodes, bytecode_len, url, persistent_handle, result_callback);
}

void evaluateQuickjsByteCode(void* page_,
                             uint8_t* bytes,
                             int32_t byteLen,
                             int64_t profile_id,
                             Dart_Handle dart_handle,
                             EvaluateQuickjsByteCodeCallback result_callback) {
#if ENABLE_LOG
  WEBF_LOG(VERBOSE) << "[Dart] evaluateQuickjsByteCodeWrapper call" << std::endl;
#endif
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  Dart_PersistentHandle persistent_handle = Dart_NewPersistentHandle_DL(dart_handle);
  page->dartIsolateContext()->dispatcher()->PostToJs(page->isDedicated(), static_cast<int32_t>(page->contextId()),
                                                     webf::WebFPage::EvaluateQuickjsByteCodeInternal, page_, bytes,
                                                     byteLen, profile_id, persistent_handle, result_callback);
}

void parseHTML(void* page_,
               char* code,
               int32_t length,
               int64_t profile_id,
               Dart_Handle dart_handle,
               ParseHTMLCallback result_callback) {
#if ENABLE_LOG
  WEBF_LOG(VERBOSE) << "[Dart] parseHTMLWrapper call" << std::endl;
#endif
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  Dart_PersistentHandle persistent_handle = Dart_NewPersistentHandle_DL(dart_handle);
  page->executingContext()->dartIsolateContext()->dispatcher()->PostToJs(
      page->isDedicated(), static_cast<int32_t>(page->contextId()), webf::WebFPage::ParseHTMLInternal, page_, code,
      length, profile_id, persistent_handle, result_callback);
}

void registerPluginByteCode(uint8_t* bytes, int32_t length, const char* pluginName) {
//  webf::ExecutingContext::plugin_byte_code[pluginName] = webf::NativeByteCode{bytes, length};
}

void registerPluginCode(const char* code, int32_t length, const char* pluginName) {
//  webf::ExecutingContext::plugin_string_code[pluginName] = std::string(code, length);
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

void* parseSVGResult(const char* code, int32_t length) {
  return nullptr;
//  auto* result = webf::HTMLParser::parseSVGResult(code, length);
//  return result;
}

void freeSVGResult(void* svgTree) {
//  webf::HTMLParser::freeSVGResult(reinterpret_cast<GumboOutput*>(svgTree));
}

void invokeModuleEvent(void* page_,
                       SharedNativeString* module,
                       const char* eventType,
                       void* event,
                       NativeValue* extra,
                       Dart_Handle dart_handle,
                       InvokeModuleEventCallback result_callback) {
  Dart_PersistentHandle persistent_handle = Dart_NewPersistentHandle_DL(dart_handle);
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  auto dart_isolate_context = page->executingContext()->dartIsolateContext();
  auto is_dedicated = page->executingContext()->isDedicated();
  auto context_id = page->contextId();
  dart_isolate_context->dispatcher()->PostToJs(is_dedicated, static_cast<int32_t>(context_id),
                                               webf::WebFPage::InvokeModuleEventInternal, page_, module, eventType,
                                               event, extra, persistent_handle, result_callback);
}

void collectNativeProfileData(void* ptr, const char** data, uint32_t* len) {
  auto* dart_isolate_context = static_cast<webf::DartIsolateContext*>(ptr);
  std::string result = dart_isolate_context->profiler()->ToJSON();

  *data = static_cast<const char*>(webf::dart_malloc(sizeof(char) * result.size() + 1));
  memcpy((void*)*data, result.c_str(), sizeof(char) * result.size() + 1);
  *len = static_cast<uint32_t>(result.size());
}

void clearNativeProfileData(void* ptr) {
//  auto* dart_isolate_context = static_cast<webf::DartIsolateContext*>(ptr);
//  dart_isolate_context->profiler()->clear();
}

void dispatchUITask(void* page_, void* context, void* callback) {
  auto page = reinterpret_cast<webf::WebFPage*>(page_);
  reinterpret_cast<void (*)(void*)>(callback)(context);
}

void* getUICommandItems(void* page_) {
//  auto page = reinterpret_cast<webf::WebFPage*>(page_);
//  return page->executingContext()->uiCommandBuffer()->data();
    return nullptr;
}

uint32_t getUICommandKindFlag(void* page_) {
//  auto page = reinterpret_cast<webf::WebFPage*>(page_);
//  return page->executingContext()->uiCommandBuffer()->kindFlag();
  return 0;
}

int64_t getUICommandItemSize(void* page_) {
//  auto page = reinterpret_cast<webf::WebFPage*>(page_);
//  return page->executingContext()->uiCommandBuffer()->size();
  return 0;
}

void clearUICommandItems(void* page_) {
//  auto page = reinterpret_cast<webf::WebFPage*>(page_);
//  page->executingContext()->uiCommandBuffer()->clear();
}

// Callbacks when dart context object was finalized by Dart GC.
static void finalize_dart_context(void* peer) {
  WEBF_LOG(VERBOSE) << "[Dispatcher]: BEGIN FINALIZE DART CONTEXT: ";
  auto* dart_isolate_context = (webf::DartIsolateContext*)peer;
  dart_isolate_context->Dispose([dart_isolate_context]() {
    free(dart_isolate_context);
#if ENABLE_LOG
    WEBF_LOG(VERBOSE) << "[Dispatcher]: SUCCESS FINALIZE DART CONTEXT";
#endif
  });
}

void init_dart_dynamic_linking(void* data) {
  if (Dart_InitializeApiDL(data) != 0) {
    printf("Failed to initialize dart VM API\n");
  }
}

void on_dart_context_finalized(void* dart_isolate_context) {
  finalize_dart_context(dart_isolate_context);
}

int8_t isJSThreadBlocked(void* dart_isolate_context_, double context_id) {
  auto* dart_isolate_context = static_cast<webf::DartIsolateContext*>(dart_isolate_context_);
  auto thread_group_id = static_cast<int32_t>(context_id);
  return dart_isolate_context->dispatcher()->IsThreadBlocked(thread_group_id) ? 1 : 0;
}

// run in the dart isolate thread
void executeNativeCallback(DartWork* work_ptr) {
  auto dart_work = *(work_ptr);
  dart_work(false);
  delete work_ptr;
}
