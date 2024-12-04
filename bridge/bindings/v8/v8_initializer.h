/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_BINDINGS_CORE_V8_V8_INITIALIZER_H_
#define WEBF_BINDINGS_CORE_V8_V8_INITIALIZER_H_

#include <v8/v8.h>
#include <v8/v8-callbacks.h>
#include "foundation/macros.h"

namespace webf {

// Specifies how the near V8 heap limit event was handled by the callback.
// This enum is also used for UMA histogram recording. It must be kept in sync
// with the corresponding enum in tools/metrics/histograms/enums.xml. See that
// enum for the detailed description of each case.
//
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class NearV8HeapLimitHandling {
  kForwardedToBrowser = 0,
  kIgnoredDueToSmallUptime = 1,
  kIgnoredDueToChangedHeapLimit = 2,
  kIgnoredDueToWorker = 3,
  kIgnoredDueToCooldownTime = 4,
  kMaxValue = kIgnoredDueToCooldownTime
};

// A callback function called when V8 reaches the heap limit.
using NearV8HeapLimitCallback = NearV8HeapLimitHandling (*)();

class V8Initializer {
  WEBF_STATIC_ONLY(V8Initializer);

 public:
  // This must be called before InitializeMainThread.
  static void SetNearV8HeapLimitOnMainThreadCallback(
      NearV8HeapLimitCallback callback);

  static v8::Isolate* InitializeMainThread();

  static void InitializeIsolateHolder(const intptr_t* reference_table,
                                      const std::string js_command_line_flag);
  static void InitializeV8Common(v8::Isolate*);

  // TODO webf not need use for now
//  static void MessageHandlerInMainThread(v8::Local<v8::Message>,
//                                         v8::Local<v8::Value>);

//  static v8::ModifyCodeGenerationFromStringsResult
//  CodeGenerationCheckCallbackInMainThread(v8::Local<v8::Context> context,
//                                          v8::Local<v8::Value> source,
//                                          bool is_code_like);
//  static void FailedAccessCheckCallbackInMainThread(
//      v8::Local<v8::Object> holder,
//      v8::AccessType type,
//      v8::Local<v8::Value> data);
//  static void PromiseRejectHandlerInMainThread(v8::PromiseRejectMessage data);
};

}  // namespace webf

#endif  // WEBF_BINDINGS_CORE_V8_V8_INITIALIZER_H_

