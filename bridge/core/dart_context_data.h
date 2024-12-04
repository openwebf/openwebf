/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_CORE_DART_CONTEXT_DATA_H_
#define WEBF_CORE_DART_CONTEXT_DATA_H_

#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#if WEBF_V8_JS_ENGINE
#include "bindings/v8/atomic_string.h"
#elif WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/atomic_string.h"
#endif

namespace webf {

struct WidgetElementShape {
  std::unordered_set<std::string> built_in_properties_;
  std::unordered_set<std::string> built_in_methods_;
  std::unordered_set<std::string> built_in_async_methods_;
};

class DartContextData {
 public:
  const WidgetElementShape* GetWidgetElementShape(const std::string& key);
  bool HasWidgetElementShape(const std::string& key);
  void SetWidgetElementShape(const std::string& key, const std::shared_ptr<WidgetElementShape>& shape);

 private:
  std::mutex context_data_mutex_;
  // WidgetElements' properties and methods are defined in the dart Side.
  // When a new kind of WidgetElement first created, Dart code will sync properties and methods to C++ code to generate
  // prop getter and setter and functions for JS code. This map store the properties and methods of WidgetElement which
  // already created.
  std::unordered_map<std::string, std::shared_ptr<WidgetElementShape>> widget_element_shapes_;
};

}  // namespace webf

#endif  // WEBF_CORE_DART_CONTEXT_DATA_H_
