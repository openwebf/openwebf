/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "performance_entry.h"
#include "bindings/qjs/converter_impl.h"
#include "core/executing_context.h"

namespace webf {

PerformanceEntry::PerformanceEntry(ExecutingContext* context,
                                   const AtomicString& name,
                                   int64_t start_time,
                                   int64_t end_time)
    : ScriptWrappable(context->ctx()), name_(name), start_time_(start_time), duration_(end_time - start_time) {}

PerformanceEntry::PerformanceEntry(int64_t duration,
                                   ExecutingContext* context,
                                   const AtomicString& name,
                                   int64_t start_time)
    : ScriptWrappable(context->ctx()), name_(name), start_time_(start_time), duration_(duration) {}

const AtomicString PerformanceEntry::name() const {
  return name_;
}

int64_t PerformanceEntry::startTime() const {
  return start_time_;
}

int64_t PerformanceEntry::duration() const {
  return duration_;
}

int64_t PerformanceEntry::uniqueId() const {
  return unique_id_;
}

ScriptValue PerformanceEntry::toJSON(ExceptionState& exception_state) {
  JSValue object = JS_NewObject(ctx());
  JS_SetPropertyStr(ctx(), object, "name", Converter<IDLDOMString>::ToValue(ctx(), name_));
  JS_SetPropertyStr(ctx(), object, "entryType", Converter<IDLDOMString>::ToValue(ctx(), entryType()));
  JS_SetPropertyStr(ctx(), object, "startTime", Converter<IDLInt64>::ToValue(ctx(), start_time_));
  JS_SetPropertyStr(ctx(), object, "duration", Converter<IDLInt64>::ToValue(ctx(), duration_));
  ScriptValue result = ScriptValue(ctx(), object);
  JS_FreeValue(ctx(), object);
  return result;
}

bool PerformanceEntry::IsPerformanceMeasure() const {
  return false;
}

bool PerformanceEntry::IsPerformanceMark() const {
  return false;
}

const PerformanceEntryPublicMethods* PerformanceEntry::performanceEntryPublicMethods() {
  static PerformanceEntryPublicMethods performance_entry_public_methods;
  return &performance_entry_public_methods;
}

}  // namespace webf
