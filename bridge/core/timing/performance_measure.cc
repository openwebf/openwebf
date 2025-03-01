/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "performance_measure.h"
#include "performance_entry_names.h"

namespace webf {

PerformanceMeasure* PerformanceMeasure::Create(ExecutingContext* context,
                                               const AtomicString& name,
                                               int64_t start_time,
                                               int64_t end_time,
                                               const ScriptValue& detail,
                                               ExceptionState& exception_state) {
  return MakeGarbageCollected<PerformanceMeasure>(context, name, start_time, end_time, detail, exception_state);
}

PerformanceMeasure::PerformanceMeasure(ExecutingContext* context,
                                       const AtomicString& name,
                                       int64_t start_time,
                                       int64_t end_time,
                                       const ScriptValue& detail,
                                       ExceptionState& exception_state)
    : PerformanceEntry(end_time - start_time, context, name, start_time), detail_(detail) {}

AtomicString PerformanceMeasure::entryType() const {
  return performance_entry_names::kmeasure;
}

ScriptValue PerformanceMeasure::detail() const {
  return detail_;
}

bool PerformanceMeasure::IsPerformanceMeasure() const {
  return true;
}

const PerformanceMeasurePublicMethods* PerformanceMeasure::performanceMeasurePublicMethods() {
  static PerformanceMeasurePublicMethods performance_measure_public_methods;
  return &performance_measure_public_methods;
}

}  // namespace webf
