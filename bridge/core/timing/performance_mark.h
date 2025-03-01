/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_CORE_TIMING_PERFORMANCE_MARK_H_
#define WEBF_CORE_TIMING_PERFORMANCE_MARK_H_

#include "bindings/qjs/script_value.h"
#include "performance_entry.h"
#include "performance_entry_names.h"
#include "plugin_api/performance_mark.h"
#include "qjs_performance_mark_options.h"

namespace webf {

class PerformanceMark : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PerformanceMark* Create(ExecutingContext* context,
                                 const AtomicString& name,
                                 const std::shared_ptr<PerformanceMarkOptions>& mark_options,
                                 ExceptionState& exception_state);

  explicit PerformanceMark(ExecutingContext* context,
                           const AtomicString& name,
                           int64_t start_time,
                           const ScriptValue& detail);

  AtomicString entryType() const override;
  ScriptValue detail() const;

  void Trace(GCVisitor* visitor) const override;

  bool IsPerformanceMark() const override;
  const PerformanceMarkPublicMethods* performanceMarkPublicMethods();

 private:
  ScriptValue detail_;
};

template <>
struct DowncastTraits<PerformanceMark> {
  static bool AllowFrom(const PerformanceEntry& entry) { return entry.IsPerformanceMark(); }
};

}  // namespace webf

#endif  // WEBF_CORE_TIMING_PERFORMANCE_MARK_H_
