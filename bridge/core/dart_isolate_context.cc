/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#if WEBF_V8_JS_ENGINE
#include <v8/v8-platform.h>
#include "v8/libplatform/libplatform.h"
#endif
#include <unordered_set>
#include "dart_isolate_context.h"
//#include "event_factory.h"
//#include "html_element_factory.h"
#include "names_installer.h"
#include "page.h"
//#include "svg_element_factory.h"

namespace webf {

thread_local std::unordered_set<DartWireContext*> alive_wires;

PageGroup::~PageGroup() {
  for (auto page : pages_) {
    delete page;
  }
}

void PageGroup::AddNewPage(webf::WebFPage* new_page) {
  assert(std::find(pages_.begin(), pages_.end(), new_page) == pages_.end());
  pages_.push_back(new_page);
}

void PageGroup::RemovePage(webf::WebFPage* page) {
  pages_.erase(std::find(pages_.begin(), pages_.end(), page));
}

void WatchDartWire(DartWireContext* wire) {
  alive_wires.emplace(wire);
}

bool IsDartWireAlive(DartWireContext* wire) {
  return alive_wires.find(wire) != alive_wires.end();
}

void DeleteDartWire(DartWireContext* wire) {
  alive_wires.erase(wire);
  delete wire;
}

#if WEBF_QUICKJS_JS_ENGINE
static void ClearUpWires(JSRuntime* runtime) {
  for (auto& wire : alive_wires) {
    JS_FreeValueRT(runtime, wire->jsObject.QJSValue());
    wire->disposed = true;
  }
  alive_wires.clear();
}
#endif

const std::unique_ptr<DartContextData>& DartIsolateContext::EnsureData() const {
  if (data_ == nullptr) {
    data_ = std::make_unique<DartContextData>();
  }
  return data_;
}

#if WEBF_V8_JS_ENGINE
std::unique_ptr<v8::Platform> platform = nullptr;
thread_local v8::Isolate* isolate_{nullptr};
#elif WEBF_QUICKJS_JS_ENGINE
thread_local JSRuntime* runtime_{nullptr};
#endif
thread_local uint32_t running_dart_isolates = 0;
thread_local bool is_name_installed_ = false;

#if WEBF_QUICKJS_JS_ENGINE
void InitializeBuiltInStrings(JSContext* ctx) {
  if (!is_name_installed_) {
    names_installer::Init(ctx);
    is_name_installed_ = true;
  }
}
#elif WEBF_V8_JS_ENGINE
void InitializeBuiltInStrings(v8::Isolate* isolate) {
  if (!is_name_installed_) {
//    names_installer::Init(isolate);
    is_name_installed_ = true;
  }
}
#endif

void DartIsolateContext::InitializeJSRuntime() {
#if WEBF_QUICKJS_JS_ENGINE
  if (runtime_ != nullptr)
    return;
  runtime_ = JS_NewRuntime();
  // Avoid stack overflow when running in multiple threads.
  JS_UpdateStackTop(runtime_);
  // Bump up the built-in classId. To make sure the created classId are larger than JS_CLASS_CUSTOM_CLASS_INIT_COUNT.
  for (int i = 0; i < JS_CLASS_CUSTOM_CLASS_INIT_COUNT - JS_CLASS_GC_TRACKER + 2; i++) {
    JSClassID id{0};
    JS_NewClassID(&id);
  }
#elif WEBF_V8_JS_ENGINE
  if (platform == nullptr) {
    platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
  }
  // Create a new Isolate and make it the current one.
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate_ = v8::Isolate::New(create_params);
#endif
}

void DartIsolateContext::FinalizeJSRuntime() {
  if (running_dart_isolates > 0 || runtime_ == nullptr) {
    return;
  }
#if WEBF_QUICKJS_JS_ENGINE
  if (runtime_ == nullptr) {
    return;
  }
#elif WEBF_V8_JS_ENGINE
  if (isolate_ == nullptr) {
    return;
  }
#endif


  // Prebuilt strings stored in JSRuntime. Only needs to dispose when runtime disposed.
//  names_installer::Dispose();
//  HTMLElementFactory::Dispose();
//  SVGElementFactory::Dispose();
//  EventFactory::Dispose();

#if WEBF_QUICKJS_JS_ENGINE
  ClearUpWires(runtime_);
  JS_TurnOnGC(runtime_);
  JS_FreeRuntime(runtime_);
  runtime_ = nullptr;
#elif WEBF_V8_JS_ENGINE
  isolate_->Dispose();
  isolate_ = nullptr;
#endif
  is_name_installed_ = false;
}

DartIsolateContext::DartIsolateContext(const uint64_t* dart_methods, int32_t dart_methods_length, bool profile_enabled)
    : is_valid_(true),
      running_thread_(std::this_thread::get_id()),
//      profiler_(std::make_unique<WebFProfiler>(profile_enabled)),
      dart_method_ptr_(std::make_unique<DartMethodPointer>(this, dart_methods, dart_methods_length)) {
  is_valid_ = true;
  running_dart_isolates++;
}

#if WEBF_QUICKJS_JS_ENGINE
JSRuntime* DartIsolateContext::runtime() {
  assert_m(runtime_ != nullptr, "nullptr is unsafe");
  return runtime_;
}
#elif WEBF_V8_JS_ENGINE
v8::Isolate* DartIsolateContext::isolate() {
  return isolate_;
}
#endif

DartIsolateContext::~DartIsolateContext() {}

void DartIsolateContext::Dispose(multi_threading::Callback callback) {
  dispatcher_->Dispose([this, &callback]() {
    is_valid_ = false;
    data_.reset();
    pages_in_ui_thread_.clear();
    running_dart_isolates--;
    FinalizeJSRuntime();
    callback();
  });
}

void DartIsolateContext::InitializeNewPageInJSThread(PageGroup* page_group,
                                                     DartIsolateContext* dart_isolate_context,
                                                     double page_context_id,
                                                     int32_t sync_buffer_size,
                                                     Dart_Handle dart_handle,
                                                     AllocateNewPageCallback result_callback) {
//  dart_isolate_context->profiler()->StartTrackInitialize();
  DartIsolateContext::InitializeJSRuntime();

  v8::HandleScope handle_scope(dart_isolate_context->isolate());

  auto* page = new WebFPage(dart_isolate_context, true, sync_buffer_size, page_context_id, nullptr);

//  dart_isolate_context->profiler()->FinishTrackInitialize();

  dart_isolate_context->dispatcher_->PostToDart(true, HandleNewPageResult, page_group, dart_handle, result_callback,
                                                page);
}

void DartIsolateContext::DisposePageAndKilledJSThread(DartIsolateContext* dart_isolate_context,
                                                      WebFPage* page,
                                                      int thread_group_id,
                                                      Dart_Handle dart_handle,
                                                      DisposePageCallback result_callback) {
  delete page;
  dart_isolate_context->dispatcher_->PostToDart(true, HandleDisposePageAndKillJSThread, dart_isolate_context,
                                                thread_group_id, dart_handle, result_callback);
}

void DartIsolateContext::DisposePageInJSThread(DartIsolateContext* dart_isolate_context,
                                               WebFPage* page,
                                               Dart_Handle dart_handle,
                                               DisposePageCallback result_callback) {
  delete page;
  dart_isolate_context->dispatcher_->PostToDart(true, HandleDisposePage, dart_handle, result_callback);
}

void* DartIsolateContext::AddNewPage(double thread_identity,
                                     int32_t sync_buffer_size,
                                     Dart_Handle dart_handle,
                                     AllocateNewPageCallback result_callback) {
  bool is_in_flutter_ui_thread = thread_identity < 0;
  assert(is_in_flutter_ui_thread == false);

  int thread_group_id = static_cast<int>(thread_identity);

  PageGroup* page_group;
  if (!dispatcher_->IsThreadGroupExist(thread_group_id)) {
    dispatcher_->AllocateNewJSThread(thread_group_id);
    page_group = new PageGroup();
    dispatcher_->SetOpaqueForJSThread(thread_group_id, page_group, [](void* p) {
      delete static_cast<PageGroup*>(p);
      DartIsolateContext::FinalizeJSRuntime();
    });
  } else {
    page_group = static_cast<PageGroup*>(dispatcher_->GetOpaque(thread_group_id));
  }

  dispatcher_->PostToJs(true, thread_group_id, InitializeNewPageInJSThread, page_group, this, thread_identity,
                        sync_buffer_size, dart_handle, result_callback);
  return nullptr;
}

std::unique_ptr<WebFPage> DartIsolateContext::InitializeNewPageSync(DartIsolateContext* dart_isolate_context,
                                                                    size_t sync_buffer_size,
                                                                    double page_context_id) {
//  dart_isolate_context->profiler()->StartTrackInitialize();
  DartIsolateContext::InitializeJSRuntime();
  auto page = std::make_unique<WebFPage>(dart_isolate_context, false, sync_buffer_size, page_context_id, nullptr);
//  dart_isolate_context->profiler()->FinishTrackInitialize();

  return page;
}

void* DartIsolateContext::AddNewPageSync(double thread_identity) {
  auto page = InitializeNewPageSync(this, 0, thread_identity);

  void* p = page.get();
  pages_in_ui_thread_.emplace(std::move(page));
  return p;
}

void DartIsolateContext::HandleNewPageResult(PageGroup* page_group,
                                             Dart_Handle persistent_handle,
                                             AllocateNewPageCallback result_callback,
                                             WebFPage* new_page) {
  page_group->AddNewPage(new_page);
  Dart_Handle handle = Dart_HandleFromPersistent_DL(persistent_handle);
  result_callback(handle, new_page);
  Dart_DeletePersistentHandle_DL(persistent_handle);
}

void DartIsolateContext::HandleDisposePage(Dart_Handle persistent_handle, DisposePageCallback result_callback) {
  Dart_Handle handle = Dart_HandleFromPersistent_DL(persistent_handle);
  result_callback(handle);
  Dart_DeletePersistentHandle_DL(persistent_handle);
}

void DartIsolateContext::HandleDisposePageAndKillJSThread(DartIsolateContext* dart_isolate_context,
                                                          int thread_group_id,
                                                          Dart_Handle persistent_handle,
                                                          DisposePageCallback result_callback) {
  dart_isolate_context->dispatcher_->KillJSThreadSync(thread_group_id);

  Dart_Handle handle = Dart_HandleFromPersistent_DL(persistent_handle);
  result_callback(handle);
  Dart_DeletePersistentHandle_DL(persistent_handle);
}

void DartIsolateContext::RemovePage(double thread_identity,
                                    WebFPage* page,
                                    Dart_Handle dart_handle,
                                    DisposePageCallback result_callback) {
  bool is_in_flutter_ui_thread = thread_identity < 0;
  assert(is_in_flutter_ui_thread == false);

  int thread_group_id = static_cast<int>(page->contextId());
  auto page_group = static_cast<PageGroup*>(dispatcher_->GetOpaque(thread_group_id));

  page_group->RemovePage(page);

  if (page_group->Empty()) {
    page->executingContext()->SetContextInValid();
    dispatcher_->PostToJs(true, thread_group_id, DisposePageAndKilledJSThread, this, page, thread_group_id, dart_handle,
                          result_callback);
  } else {
    dispatcher_->PostToJs(true, thread_group_id, DisposePageInJSThread, this, page, dart_handle, result_callback);
  }
}

void DartIsolateContext::RemovePageSync(double thread_identity, WebFPage* page) {
  for (auto it = pages_in_ui_thread_.begin(); it != pages_in_ui_thread_.end(); ++it) {
    if (it->get() == page) {
      pages_in_ui_thread_.erase(it);
      break;
    }
  }

  if (pages_in_ui_thread_.empty()) {
    FinalizeJSRuntime();
  }
}

}  // namespace webf
