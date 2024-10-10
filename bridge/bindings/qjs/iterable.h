/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_BINDINGS_QJS_ITERABLE_H_
#define WEBF_BINDINGS_QJS_ITERABLE_H_

#include "bindings/qjs/sync_iterator.h"

namespace webf {

ScriptValue ESCreateIterResultObject(JSContext* ctx, bool done, const ScriptValue& value);
ScriptValue ESCreateIterResultObject(JSContext* ctx, bool done, const ScriptValue& value1, const ScriptValue& value2);

class PairSyncIterationSource {
 public:
  virtual ScriptValue Next(SyncIterator::Kind, ExceptionState& exception_state) = 0;

  virtual ScriptValue Value() = 0;
  virtual bool Done() = 0;

  virtual void Trace(GCVisitor* visitor) = 0;
  virtual JSContext* ctx() = 0;

 private:
};

class PairSyncIterable {
 public:
  SyncIterator* keys(ExceptionState& exception_state) {
    std::shared_ptr<PairSyncIterationSource> source = CreateIterationSource(exception_state);
    if (!source)
      return nullptr;
    return MakeGarbageCollected<SyncIterator>(source->ctx(), source, SyncIterator::Kind::kKey);
  }

  SyncIterator* values(ExceptionState& exception_state) {
    std::shared_ptr<PairSyncIterationSource> source = CreateIterationSource(exception_state);
    if (!source)
      return nullptr;
    return MakeGarbageCollected<SyncIterator>(source->ctx(), source, SyncIterator::Kind::kValue);
  }

  SyncIterator* entries(ExceptionState& exception_state) {
    std::shared_ptr<PairSyncIterationSource> source = CreateIterationSource(exception_state);
    if (!source)
      return nullptr;
    return MakeGarbageCollected<SyncIterator>(source->ctx(), source, SyncIterator::Kind::kKeyValue);
  }

  virtual void forEach(const std::shared_ptr<QJSFunction>& callback, ExceptionState& exception_state) = 0;
  virtual void forEach(const std::shared_ptr<QJSFunction>& callback,
                       const ScriptValue& this_arg,
                       ExceptionState& exception_state) = 0;

 private:
  virtual std::shared_ptr<PairSyncIterationSource> CreateIterationSource(ExceptionState& exception_state) = 0;
};

}  // namespace webf

#endif  // WEBF_BINDINGS_QJS_ITERABLE_H_
