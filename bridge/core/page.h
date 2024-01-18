/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_JS_QJS_BRIDGE_H_
#define WEBF_JS_QJS_BRIDGE_H_

#include <quickjs/quickjs.h>
#include <atomic>
#include <deque>
#include <thread>
#include <vector>

#include "core/executing_context.h"
#include "foundation/native_string.h"

namespace webf {

class WebFPage;
class DartContext;

using JSBridgeDisposeCallback = void (*)(WebFPage* bridge);
using ConsoleMessageHandler = std::function<void(void* ctx, const std::string& message, int logLevel)>;

/// WebFPage is class which manage all js objects create by <WebF> flutter widget.
/// Every <WebF> flutter widgets have a corresponding WebFPage, and all objects created by JavaScript are stored here,
/// and there is no data sharing between objects between different WebFPages.
/// It's safe to allocate many WebFPages at the same times on one thread, but not safe for multi-threads, only one
/// thread can enter to WebFPage at the same time.
class WebFPage final {
 public:
  static ConsoleMessageHandler consoleMessageHandler;
  WebFPage() = delete;
  WebFPage(DartIsolateContext* dart_isolate_context,
           bool is_dedicated,
           size_t sync_buffer_size,
           double context_id,
           const JSExceptionHandler& handler);
  ~WebFPage();

  // Bytecodes which registered by webf plugins.
  static std::unordered_map<std::string, NativeByteCode> pluginByteCode;

  // evaluate JavaScript source codes in standard mode.
  bool evaluateScript(const char* script,
                      uint64_t script_len,
                      uint8_t** parsed_bytecodes,
                      uint64_t* bytecode_len,
                      const char* url,
                      int startLine);
  bool parseHTML(const char* code, size_t length);
  void evaluateScript(const char* script, size_t length, const char* url, int startLine);
  uint8_t* dumpByteCode(const char* script, size_t length, const char* url, uint64_t* byteLength);
  bool evaluateByteCode(uint8_t* bytes, size_t byteLength);

  std::thread::id currentThread() const;

  [[nodiscard]] ExecutingContext* executingContext() const { return context_; }
  [[nodiscard]] DartIsolateContext* dartIsolateContext() const { return dart_isolate_context_; }

  NativeValue* invokeModuleEvent(SharedNativeString* moduleName,
                                 const char* eventType,
                                 void* event,
                                 NativeValue* extra);
  void reportError(const char* errmsg);

  FORCE_INLINE bool isDedicated() { return context_->isDedicated(); };
  FORCE_INLINE double contextId() { return context_->contextId(); }

#if IS_TEST
  // the owner pointer which take JSBridge as property.
  void* owner;
  JSBridgeDisposeCallback disposeCallback{nullptr};
#endif
 private:
  const std::thread::id ownerThreadId;
  // FIXME: we must to use raw pointer instead of unique_ptr because we needs to access context_ when dispose page.
  // TODO: Raw pointer is dangerous and just works but it's fragile. We needs refactor this for more stable and
  // maintainable.
  DartIsolateContext* dart_isolate_context_;
  ExecutingContext* context_;
  JSExceptionHandler handler_;
};

}  // namespace webf

#endif  // WEBF_JS_QJS_BRIDGE_H_
