/*
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "bindings/qjs/sync_iterator.h"
#include "bindings/qjs/iterable.h"

namespace webf {

ScriptValue SyncIterator::value() {
  return iteration_source_->Value();
}

ScriptValue SyncIterator::Symbol_iterator() {
  auto iterator_return_func = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    return JS_DupValue(ctx, this_val);
  };
  return ScriptValue(ctx(), JS_NewCFunction(ctx(), iterator_return_func, "iterator", 0));
}

bool SyncIterator::done() {
  return iteration_source_->Done();
}

void SyncIterator::Trace(GCVisitor* visitor) const {
  iteration_source_->Trace(visitor);
  ScriptWrappable::Trace(visitor);
}

ScriptValue SyncIterator::next(ExceptionState& exception_state) {
  return iteration_source_->Next(kind_, exception_state);
}

}