/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_DART_METHODS_H_
#define WEBF_DART_METHODS_H_

/// Functions implements at dart side, including timer, Rendering and module API.
/// Communicate via Dart FFI.

#include <memory>
#include <thread>
#include "foundation/native_string.h"
#include "foundation/native_value.h"
#include "include/dart_api.h"

#if defined(_WIN32)
#define WEBF_EXPORT_C extern "C" __declspec(dllexport)
#define WEBF_EXPORT __declspec(dllexport)
#else
#define WEBF_EXPORT_C extern "C" __attribute__((visibility("default"))) __attribute__((used))
#define WEBF_EXPORT __attribute__((__visibility__("default")))
#endif

namespace webf {

using InvokeModuleResultCallback = void (*)(Dart_PersistentHandle persistent_handle, NativeValue* result);
using AsyncCallback = void (*)(void* callback_context, double context_id, char* errmsg);
using AsyncRAFCallback = void (*)(void* callback_context, double context_id, double result, char* errmsg);
using AsyncModuleCallback = NativeValue* (*)(void* callback_context,
                                             double context_id,
                                             const char* errmsg,
                                             NativeValue* value,
                                             Dart_PersistentHandle persistent_handle,
                                             InvokeModuleResultCallback result_callback);

using AsyncBlobCallback =
    void (*)(void* callback_context, double context_id, char* error, uint8_t* bytes, int32_t length);
typedef NativeValue* (*InvokeModule)(void* callback_context,
                                     double context_id,
                                     int64_t profile_link_id,
                                     SharedNativeString* moduleName,
                                     SharedNativeString* method,
                                     NativeValue* params,
                                     uint32_t argc,
                                     AsyncModuleCallback callback);
typedef void (*RequestBatchUpdate)(double context_id);
typedef void (*ReloadApp)(double context_id);
typedef void (*SetTimeout)(int32_t new_timer_id,
                           void* callback_context,
                           double context_id,
                           AsyncCallback callback,
                           int32_t timeout);
typedef void (*SetInterval)(int32_t new_timer_id,
                            void* callback_context,
                            double context_id,
                            AsyncCallback callback,
                            int32_t timeout);
typedef void (*RequestAnimationFrame)(int32_t new_frame_id,
                                      void* callback_context,
                                      double context_id,
                                      AsyncRAFCallback callback);
typedef void (*ClearTimeout)(double context_id, int32_t timerId);
typedef void (*CancelAnimationFrame)(double context_id, int32_t id);
typedef void (*ToBlob)(void* callback_context,
                       double context_id,
                       AsyncBlobCallback blobCallback,
                       void* element_ptr,
                       double devicePixelRatio);
typedef void (*OnJSError)(double context_id, const char*);
typedef void (*OnJSLog)(double context_id, int32_t level, const char*);
typedef void (*FlushUICommand)(double context_id, void* native_binding_object);
typedef void (
    *CreateBindingObject)(double context_id, void* native_binding_object, int32_t type, void* args, int32_t argc);
typedef int8_t (*GetWidgetElementShape)(double context_id, void* native_binding_object, NativeValue* value);

using MatchImageSnapshotCallback = void (*)(void* callback_context, double context_id, int8_t, char* errmsg);
using MatchImageSnapshot = void (*)(void* callback_context,
                                    double context_id,
                                    uint8_t* bytes,
                                    int32_t length,
                                    SharedNativeString* name,
                                    MatchImageSnapshotCallback callback);
using MatchImageSnapshotBytes = void (*)(void* callback_context,
                                         double context_id,
                                         uint8_t* image_a_bytes,
                                         int32_t image_a_size,
                                         uint8_t* image_b_bytes,
                                         int32_t image_b_size,
                                         MatchImageSnapshotCallback callback);
using Environment = const char* (*)();

#if ENABLE_PROFILE
struct NativePerformanceEntryList {
  uint64_t* entries;
  int32_t length;
};
typedef NativePerformanceEntryList* (*GetPerformanceEntries)(int32_t);
#endif

struct MousePointer {
  double context_id;
  double x;
  double y;
  double change;
  int32_t signal_kind;
  double delta_x;
  double delta_y;
};
using SimulatePointer =
    void (*)(void* ptr, MousePointer*, int32_t length, int32_t pointer, AsyncCallback async_callback);
using SimulateInputText = void (*)(SharedNativeString* nativeString);

enum FlushUICommandReason : uint32_t {
  kStandard = 1,
  kDependentsOnElement = 1 << 2,
  kDependentsOnLayout = 1 << 3,
  kDependentsAll = 1 << 4
};

inline bool isUICommandReasonDependsOnElement(uint32_t reason) {
  return (reason & kDependentsOnElement) != 0;
}

inline bool isUICommandReasonDependsOnLayout(uint32_t reason) {
  return (reason & kDependentsOnLayout) != 0;
}

inline bool isUICommandReasonDependsOnAll(uint32_t reason) {
  return (reason & kDependentsAll) != 0;
}

class DartIsolateContext;

class DartMethodPointer {
  DartMethodPointer() = delete;

 public:
  explicit DartMethodPointer(DartIsolateContext* dart_isolate_context,
                             const uint64_t* dart_methods,
                             int32_t dartMethodsLength);
  NativeValue* invokeModule(bool is_dedicated,
                            void* callback_context,
                            double context_id,
                            int64_t profile_link_id,
                            SharedNativeString* moduleName,
                            SharedNativeString* method,
                            NativeValue* params,
                            uint32_t argc,
                            AsyncModuleCallback callback);

  void requestBatchUpdate(bool is_dedicated, double context_id);
  void reloadApp(bool is_dedicated, double context_id);
  int32_t setTimeout(bool is_dedicated,
                     void* callback_context,
                     double context_id,
                     AsyncCallback callback,
                     int32_t timeout);
  int32_t setInterval(bool is_dedicated,
                      void* callback_context,
                      double context_id,
                      AsyncCallback callback,
                      int32_t timeout);
  void clearTimeout(bool is_dedicated, double context_id, int32_t timerId);
  int32_t requestAnimationFrame(bool is_dedicated,
                                void* callback_context,
                                double context_id,
                                AsyncRAFCallback callback);
  void cancelAnimationFrame(bool is_dedicated, double context_id, int32_t id);
  void toBlob(bool is_dedicated,
              void* callback_context,
              double context_id,
              AsyncBlobCallback blobCallback,
              void* element_ptr,
              double devicePixelRatio);
  void flushUICommand(bool is_dedicated, double context_id, void* native_binding_object);
  void createBindingObject(bool is_dedicated,
                           double context_id,
                           void* native_binding_object,
                           int32_t type,
                           void* args,
                           int32_t argc);
  bool getWidgetElementShape(bool is_dedicated, double context_id, void* native_binding_object, NativeValue* value);

  void onJSError(bool is_dedicated, double context_id, const char*);
  void onJSLog(bool is_dedicated, double context_id, int32_t level, const char*);

  void matchImageSnapshot(bool is_dedicated,
                          void* callback_context,
                          double context_id,
                          uint8_t* bytes,
                          int32_t length,
                          SharedNativeString* name,
                          MatchImageSnapshotCallback callback);

  void matchImageSnapshotBytes(bool is_dedicated,
                               void* callback_context,
                               double context_id,
                               uint8_t* image_a_bytes,
                               int32_t image_a_size,
                               uint8_t* image_b_bytes,
                               int32_t image_b_size,
                               MatchImageSnapshotCallback callback);

  const char* environment(bool is_dedicated, double context_id);
  void simulatePointer(bool is_dedicated,
                       void* ptr,
                       MousePointer*,
                       int32_t length,
                       int32_t pointer,
                       AsyncCallback async_callback);
  void simulateInputText(bool is_dedicated, SharedNativeString* nativeString);

  void SetOnJSError(OnJSError func);
  void SetMatchImageSnapshot(MatchImageSnapshot func);
  void SetMatchImageSnapshotBytes(MatchImageSnapshotBytes func);
  void SetEnvironment(Environment func);
  void SetSimulatePointer(SimulatePointer func);
  void SetSimulateInputText(SimulateInputText func);

 private:
  DartIsolateContext* dart_isolate_context_{nullptr};
  InvokeModule invoke_module_{nullptr};
  RequestBatchUpdate request_batch_update_{nullptr};
  ReloadApp reload_app_{nullptr};
  SetTimeout set_timeout_{nullptr};
  SetInterval set_interval_{nullptr};
  ClearTimeout clear_timeout_{nullptr};
  RequestAnimationFrame request_animation_frame_{nullptr};
  CancelAnimationFrame cancel_animation_frame_{nullptr};
  ToBlob to_blob_{nullptr};
  FlushUICommand flush_ui_command_{nullptr};
  CreateBindingObject create_binding_object_{nullptr};
  GetWidgetElementShape get_widget_element_shape_{nullptr};
  OnJSError on_js_error_{nullptr};
  OnJSLog on_js_log_{nullptr};
  MatchImageSnapshot match_image_snapshot_{nullptr};
  MatchImageSnapshotBytes match_image_snapshot_bytes_{nullptr};
  Environment environment_{nullptr};
  SimulatePointer simulate_pointer_{nullptr};
  SimulateInputText simulate_input_text_{nullptr};
};

}  // namespace webf

#endif
