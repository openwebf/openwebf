/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "dart_methods.h"
#include <stdio.h>
#include <cassert>
#include "dart_isolate_context.h"
#include "foundation/native_type.h"

using namespace webf;

namespace webf {

int32_t start_timer_id = 1;

DartMethodPointer::DartMethodPointer(DartIsolateContext* dart_isolate_context,
                                     const uint64_t* dart_methods,
                                     int32_t dart_methods_length)
    : dart_isolate_context_(dart_isolate_context) {
  size_t i = 0;
  invoke_module_ = reinterpret_cast<InvokeModule>(dart_methods[i++]);
  request_batch_update_ = reinterpret_cast<RequestBatchUpdate>(dart_methods[i++]);
  reload_app_ = reinterpret_cast<ReloadApp>(dart_methods[i++]);
  set_timeout_ = reinterpret_cast<SetTimeout>(dart_methods[i++]);
  set_interval_ = reinterpret_cast<SetInterval>(dart_methods[i++]);
  clear_timeout_ = reinterpret_cast<ClearTimeout>(dart_methods[i++]);
  request_animation_frame_ = reinterpret_cast<RequestAnimationFrame>(dart_methods[i++]);
  cancel_animation_frame_ = reinterpret_cast<CancelAnimationFrame>(dart_methods[i++]);
  to_blob_ = reinterpret_cast<ToBlob>(dart_methods[i++]);
  flush_ui_command_ = reinterpret_cast<FlushUICommand>(dart_methods[i++]);
  create_binding_object_ = reinterpret_cast<CreateBindingObject>(dart_methods[i++]);
  get_widget_element_shape_ = reinterpret_cast<GetWidgetElementShape>(dart_methods[i++]);
  on_js_error_ = reinterpret_cast<OnJSError>(dart_methods[i++]);
  on_js_log_ = reinterpret_cast<OnJSLog>(dart_methods[i++]);

  assert_m(i == dart_methods_length, "Dart native methods count is not equal with C++ side method registrations.");
}

NativeValue* DartMethodPointer::invokeModule(bool is_dedicated,
                                             void* callback_context,
                                             double context_id,
                                             int64_t profile_link_id,
                                             SharedNativeString* moduleName,
                                             SharedNativeString* method,
                                             NativeValue* params,
                                             uint32_t argc,
                                             AsyncModuleCallback callback) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::invokeModule callSync START";
#endif
  NativeValue* result = dart_isolate_context_->dispatcher()->PostToDartSync(
      is_dedicated, context_id,
      [&](bool cancel, void* callback_context, double context_id, int64_t profile_link_id,
          SharedNativeString* moduleName, SharedNativeString* method, NativeValue* params, uint32_t argc,
          AsyncModuleCallback callback) -> webf::NativeValue* {
        if (cancel)
          return nullptr;
        return invoke_module_(callback_context, context_id, profile_link_id, moduleName, method, params, argc,
                              callback);
      },
      callback_context, context_id, profile_link_id, moduleName, method, params, argc, callback);

#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::invokeModule callSync END";
#endif

  return result;
}

void DartMethodPointer::requestBatchUpdate(bool is_dedicated, double context_id) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::requestBatchUpdate Call";
#endif

  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, request_batch_update_, context_id);
}

void DartMethodPointer::reloadApp(bool is_dedicated, double context_id) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::reloadApp Call";
#endif

  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, reload_app_, context_id);
}

int32_t DartMethodPointer::setTimeout(bool is_dedicated,
                                      void* callback_context,
                                      double context_id,
                                      AsyncCallback callback,
                                      int32_t timeout) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::setTimeout callSync START";
#endif

  int32_t new_timer_id = start_timer_id++;

  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, set_timeout_, new_timer_id, callback_context,
                                                  context_id, callback, timeout);

#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::setTimeout callSync END";
#endif

  return new_timer_id;
}

int32_t DartMethodPointer::setInterval(bool is_dedicated,
                                       void* callback_context,
                                       double context_id,
                                       AsyncCallback callback,
                                       int32_t timeout) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::setInterval callSync START";
#endif

  int32_t new_timer_id = start_timer_id++;

  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, set_interval_, new_timer_id, callback_context,
                                                  context_id, callback, timeout);
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::setInterval callSync END";
#endif
  return new_timer_id;
}

void DartMethodPointer::clearTimeout(bool is_dedicated, double context_id, int32_t timer_id) {
#if ENABLE_LOG
  WEBF_LOG(VERBOSE) << "[CPP] ClearTimeoutWrapper call" << std::endl;
#endif

  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, clear_timeout_, context_id, timer_id);
}

int32_t DartMethodPointer::requestAnimationFrame(bool is_dedicated,
                                                 void* callback_context,
                                                 double context_id,
                                                 AsyncRAFCallback callback) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::requestAnimationFrame call START";
#endif

  int32_t new_frame_id = start_timer_id++;

  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, request_animation_frame_, new_frame_id,
                                                  callback_context, context_id, callback);

#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::requestAnimationFrame call END";
#endif
  return new_frame_id;
}

void DartMethodPointer::cancelAnimationFrame(bool is_dedicated, double context_id, int32_t id) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::cancelAnimationFrame call START";
#endif

  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, cancel_animation_frame_, context_id, id);
}

void DartMethodPointer::toBlob(bool is_dedicated,
                               void* callback_context,
                               double context_id,
                               AsyncBlobCallback blobCallback,
                               void* element_ptr,
                               double devicePixelRatio) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::toBlob call START";
#endif

  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, to_blob_, callback_context, context_id, blobCallback,
                                                  element_ptr, devicePixelRatio);
}

void DartMethodPointer::flushUICommand(bool is_dedicated, double context_id, void* native_binding_object) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::flushUICommand SYNC call START";
#endif

  dart_isolate_context_->dispatcher()->PostToDartSync(
      is_dedicated, context_id,
      [&](bool cancel, double context_id, void* native_binding_object) -> void {
        if (cancel)
          return;

        flush_ui_command_(context_id, native_binding_object);
      },
      context_id, native_binding_object);

#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::flushUICommand SYNC call END";
#endif
}

void DartMethodPointer::createBindingObject(bool is_dedicated,
                                            double context_id,
                                            void* native_binding_object,
                                            int32_t type,
                                            void* args,
                                            int32_t argc) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::createBindingObject SYNC call START";
#endif

  dart_isolate_context_->dispatcher()->PostToDartSync(
      is_dedicated, context_id,
      [&](bool cancel, double context_id, void* native_binding_object, int32_t type, void* args, int32_t argc) -> void {
        if (cancel)
          return;
        create_binding_object_(context_id, native_binding_object, type, args, argc);
      },
      context_id, native_binding_object, type, args, argc);

#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::createBindingObject SYNC call END";
#endif
}

bool DartMethodPointer::getWidgetElementShape(bool is_dedicated,
                                              double context_id,
                                              void* native_binding_object,
                                              NativeValue* value) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::getWidgetElementShape SYNC call START";
#endif

  int8_t is_success = dart_isolate_context_->dispatcher()->PostToDartSync(
      is_dedicated, context_id,
      [&](bool cancel, double context_id, void* native_binding_object, NativeValue* value) -> int8_t {
        if (cancel)
          return 0;
        return get_widget_element_shape_(context_id, native_binding_object, value);
      },
      context_id, native_binding_object, value);

#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::getWidgetElementShape SYNC call END";
#endif

  return is_success == 1;
}

void DartMethodPointer::onJSError(bool is_dedicated, double context_id, const char* error) {
  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, on_js_error_, context_id, error);
}

void DartMethodPointer::onJSLog(bool is_dedicated, double context_id, int32_t level, const char* log) {
  if (on_js_log_ == nullptr)
    return;
  int log_length = strlen(log) + 1;
  char* log_str = (char*)dart_malloc(sizeof(char) * log_length);
  snprintf(log_str, log_length, "%s", log);

  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, on_js_log_, context_id, level, log_str);
}

void DartMethodPointer::matchImageSnapshot(bool is_dedicated,
                                           void* callback_context,
                                           double context_id,
                                           uint8_t* bytes,
                                           int32_t length,
                                           SharedNativeString* name,
                                           MatchImageSnapshotCallback callback) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::createBindingObject call START";
#endif
  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, match_image_snapshot_, callback_context, context_id,
                                                  bytes, length, name, callback);
}

void DartMethodPointer::matchImageSnapshotBytes(bool is_dedicated,
                                                void* callback_context,
                                                double context_id,
                                                uint8_t* image_a_bytes,
                                                int32_t image_a_size,
                                                uint8_t* image_b_bytes,
                                                int32_t image_b_size,
                                                MatchImageSnapshotCallback callback) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::matchImageSnapshotBytes call START";
#endif
  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, match_image_snapshot_bytes_, callback_context,
                                                  context_id, image_a_bytes, image_a_size, image_b_bytes, image_b_size,
                                                  callback);
}

const char* DartMethodPointer::environment(bool is_dedicated, double context_id) {
#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::environment callSync START";
#endif
  const char* result =
      dart_isolate_context_->dispatcher()->PostToDartSync(is_dedicated, context_id, [&](bool cancel) -> const char* {
        if (cancel)
          return nullptr;
        return environment_();
      });

#if ENABLE_LOG
  WEBF_LOG(INFO) << "[Dispatcher] DartMethodPointer::environment callSync END";
#endif

  return result;
}

void DartMethodPointer::simulatePointer(bool is_dedicated,
                                        void* ptr,
                                        MousePointer* mouse_pointer,
                                        int32_t length,
                                        int32_t pointer,
                                        AsyncCallback async_callback) {
  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, simulate_pointer_, ptr, mouse_pointer, length, pointer,
                                                  async_callback);
}

void DartMethodPointer::simulateInputText(bool is_dedicated, SharedNativeString* nativeString) {
  dart_isolate_context_->dispatcher()->PostToDart(is_dedicated, simulate_input_text_, nativeString);
}

void DartMethodPointer::SetOnJSError(webf::OnJSError func) {
  on_js_error_ = func;
}

void DartMethodPointer::SetMatchImageSnapshot(MatchImageSnapshot func) {
  match_image_snapshot_ = func;
}

void DartMethodPointer::SetMatchImageSnapshotBytes(MatchImageSnapshotBytes func) {
  match_image_snapshot_bytes_ = func;
}

void DartMethodPointer::SetEnvironment(Environment func) {
  environment_ = func;
}

void DartMethodPointer::SetSimulateInputText(SimulateInputText func) {
  simulate_input_text_ = func;
}

void DartMethodPointer::SetSimulatePointer(SimulatePointer func) {
  simulate_pointer_ = func;
}

}  // namespace webf
