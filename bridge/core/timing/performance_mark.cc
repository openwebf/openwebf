/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "performance_mark.h"
#include "bindings/qjs/cppgc/gc_visitor.h"
#include "bindings/qjs/script_value.h"
#include "performance.h"

namespace webf {

PerformanceMark* PerformanceMark::Create(ExecutingContext* context,
                                         const AtomicString& name,
                                         const std::shared_ptr<PerformanceMarkOptions>& mark_options,
                                         ExceptionState& exception_state) {
  auto* performance = context->performance();
  int64_t start = 0;
  ScriptValue detail;
  if (mark_options != nullptr) {
    if (mark_options->hasStartTime()) {
      start = mark_options->startTime();
      if (start < 0) {
        exception_state.ThrowException(context->ctx(), ErrorType::TypeError,
                                       "'" + name.ToStdString(context->ctx()) + "' cannot have a negative Start time.");
        return nullptr;
      }
    } else {
      start = performance->now(exception_state);
    }

    if (mark_options->hasDetail()) {
      detail = mark_options->detail();
    }
  } else {
    start = performance->now(exception_state);
  }

  return MakeGarbageCollected<PerformanceMark>(context, name, start, detail);
}

PerformanceMark::PerformanceMark(ExecutingContext* context,
                                 const AtomicString& name,
                                 int64_t start_time,
                                 const ScriptValue& detail)
    : PerformanceEntry(context, name, start_time, start_time), detail_(detail) {}

AtomicString PerformanceMark::entryType() const {
  return performance_entry_names::kmark;
}

ScriptValue PerformanceMark::detail() const {
  return detail_;
}

void PerformanceMark::Trace(GCVisitor* visitor) const {
  detail_.Trace(visitor);
}

bool PerformanceMark::IsPerformanceMark() const {
  return true;
}

const PerformanceMarkPublicMethods* PerformanceMark::performanceMarkPublicMethods() {
  static PerformanceMarkPublicMethods performance_mark_public_methods;
  return &performance_mark_public_methods;
}

}  // namespace webf
