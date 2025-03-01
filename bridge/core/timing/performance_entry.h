/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_CORE_TIMING_PERFORMANCE_ENTRY_H_
#define WEBF_CORE_TIMING_PERFORMANCE_ENTRY_H_

#include "bindings/qjs/atomic_string.h"
#include "bindings/qjs/cppgc/member.h"
#include "bindings/qjs/exception_state.h"
#include "bindings/qjs/script_wrappable.h"
#include "plugin_api/performance_entry.h"

namespace webf {

#define PERFORMANCE_ENTRY_NONE_UNIQUE_ID -1024

class PerformanceEntry : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using ImplType = Member<PerformanceEntry>;
  explicit PerformanceEntry(ExecutingContext* context,
                            const AtomicString& name,
                            int64_t start_time,
                            int64_t finish_time);
  explicit PerformanceEntry(int64_t duration, ExecutingContext* context, const AtomicString& name, int64_t start_time);

  const AtomicString name() const;
  virtual AtomicString entryType() const = 0;
  int64_t startTime() const;
  int64_t duration() const;
  int64_t uniqueId() const;

  ScriptValue toJSON(ExceptionState& exception_state);

  virtual bool IsPerformanceMeasure() const;
  virtual bool IsPerformanceMark() const;
  const PerformanceEntryPublicMethods* performanceEntryPublicMethods();

 private:
  AtomicString name_;
  int64_t start_time_;
  int64_t duration_;
  int64_t unique_id_ = PERFORMANCE_ENTRY_NONE_UNIQUE_ID;
};

}  // namespace webf

#endif  // WEBF_CORE_TIMING_PERFORMANCE_ENTRY_H_
