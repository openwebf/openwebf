/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_DART_CONTEXT_H_
#define WEBF_DART_CONTEXT_H_

#if WEBF_V8_JS_ENGINE
#include <v8/v8.h>
#elif WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/script_value.h"
#endif

#include <set>

#include "dart_context_data.h"
#include "dart_methods.h"
//#include "foundation/profiler.h"
#include "multiple_threading/dispatcher.h"

namespace webf {

class WebFPage;
class DartIsolateContext;

class PageGroup {
 public:
  ~PageGroup();
  void AddNewPage(WebFPage* new_page);
  void RemovePage(WebFPage* page);
  bool Empty() { return pages_.empty(); }

  std::vector<WebFPage*>* pages() { return &pages_; };

 private:
  std::vector<WebFPage*> pages_;
};

struct DartWireContext {
//  ScriptValue jsObject;
  bool is_dedicated;
  double context_id;
  bool disposed;
  multi_threading::Dispatcher* dispatcher;
};

#if WEBF_QUICKJS_JS_ENGINE
void InitializeBuiltInStrings(JSContext* ctx);
#elif WEBF_V8_JS_ENGINE
void InitializeBuiltInStrings(v8::Isolate* isolate);
#endif

void WatchDartWire(DartWireContext* wire);
bool IsDartWireAlive(DartWireContext* wire);
void DeleteDartWire(DartWireContext* wire);

// DartIsolateContext has a 1:1 correspondence with a dart isolates.
class DartIsolateContext {
 public:
  explicit DartIsolateContext(const uint64_t* dart_methods, int32_t dart_methods_length, bool profile_enabled);

#if WEBF_QUICKJS_JS_ENGINE
  JSRuntime* runtime();
#elif WEBF_V8_JS_ENGINE
  v8::Isolate* isolate();
#endif
  FORCE_INLINE bool valid() { return is_valid_; }
  FORCE_INLINE DartMethodPointer* dartMethodPtr() const { return dart_method_ptr_.get(); }
  FORCE_INLINE const std::unique_ptr<multi_threading::Dispatcher>& dispatcher() const { return dispatcher_; }
  FORCE_INLINE void SetDispatcher(std::unique_ptr<multi_threading::Dispatcher>&& dispatcher) {
    dispatcher_ = std::move(dispatcher);
  }
//  FORCE_INLINE WebFProfiler* profiler() const { return profiler_.get(); };

  const std::unique_ptr<DartContextData>& EnsureData() const;

  void* AddNewPage(double thread_identity,
                   int32_t sync_buffer_size,
                   Dart_Handle dart_handle,
                   AllocateNewPageCallback result_callback);
  void* AddNewPageSync(double thread_identity);
  void RemovePage(double thread_identity, WebFPage* page, Dart_Handle dart_handle, DisposePageCallback result_callback);
  void RemovePageSync(double thread_identity, WebFPage* page);

  ~DartIsolateContext();
  void Dispose(multi_threading::Callback callback);

 private:
  static void InitializeJSRuntime();
  static void FinalizeJSRuntime();
  static std::unique_ptr<WebFPage> InitializeNewPageSync(DartIsolateContext* dart_isolate_context,
                                                         size_t sync_buffer_size,
                                                         double page_context_id);
  static void InitializeNewPageInJSThread(PageGroup* page_group,
                                          DartIsolateContext* dart_isolate_context,
                                          double page_context_id,
                                          int32_t sync_buffer_size,
                                          Dart_Handle dart_handle,
                                          AllocateNewPageCallback result_callback);
  static void DisposePageAndKilledJSThread(DartIsolateContext* dart_isolate_context,
                                           WebFPage* page,
                                           int thread_group_id,
                                           Dart_Handle dart_handle,
                                           DisposePageCallback result_callback);
  static void DisposePageInJSThread(DartIsolateContext* dart_isolate_context,
                                    WebFPage* page,
                                    Dart_Handle dart_handle,
                                    DisposePageCallback result_callback);
  static void HandleNewPageResult(PageGroup* page_group,
                                  Dart_Handle persistent_handle,
                                  AllocateNewPageCallback result_callback,
                                  WebFPage* new_page);
  static void HandleDisposePage(Dart_Handle persistent_handle, DisposePageCallback result_callback);
  static void HandleDisposePageAndKillJSThread(DartIsolateContext* dart_isolate_context,
                                               int thread_group_id,
                                               Dart_Handle persistent_handle,
                                               DisposePageCallback result_callback);

//  std::unique_ptr<WebFProfiler> profiler_;
  int is_valid_{false};
  std::thread::id running_thread_;
  mutable std::unique_ptr<DartContextData> data_;
  std::unordered_set<std::unique_ptr<WebFPage>> pages_in_ui_thread_;
  std::unique_ptr<multi_threading::Dispatcher> dispatcher_ = nullptr;
  // Dart methods ptr should keep alive when ExecutingContext is disposing.
  const std::unique_ptr<DartMethodPointer> dart_method_ptr_ = nullptr;
};

}  // namespace webf

#endif