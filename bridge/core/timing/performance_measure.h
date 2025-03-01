/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_CORE_TIMING_PERFORMANCE_MEASURE_H_
#define WEBF_CORE_TIMING_PERFORMANCE_MEASURE_H_

#include "bindings/qjs/script_value.h"
#include "core/executing_context.h"
#include "performance_entry.h"
#include "plugin_api/performance_measure.h"

namespace webf {

class PerformanceMeasure : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PerformanceMeasure* Create(ExecutingContext* context,
                                    const AtomicString& name,
                                    int64_t start_time,
                                    int64_t end_time,
                                    const ScriptValue& detail,
                                    ExceptionState& exception_state);

  explicit PerformanceMeasure(ExecutingContext* context,
                              const AtomicString& name,
                              int64_t start_time,
                              int64_t end_time,
                              const ScriptValue& detail,
                              ExceptionState& exception_state);

  ScriptValue detail() const;

  AtomicString entryType() const override;
  bool IsPerformanceMeasure() const override;
  const PerformanceMeasurePublicMethods* performanceMeasurePublicMethods();

 private:
  ScriptValue detail_;
};

template <>
struct DowncastTraits<PerformanceMeasure> {
  static bool AllowFrom(const PerformanceEntry& entry) { return entry.IsPerformanceMeasure(); }
};

}  // namespace webf

#endif  // WEBF_CORE_TIMING_PERFORMANCE_MEASURE_H_
