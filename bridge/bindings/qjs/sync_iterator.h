/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_BINDING_SYNC_ITERATOR_BASE
#define WEBF_BINDING_SYNC_ITERATOR_BASE

#include "bindings/qjs/cppgc/gc_visitor.h"
#include "bindings/qjs/exception_state.h"
#include "bindings/qjs/script_wrappable.h"
#include "core/script_state.h"

namespace webf {

class PairSyncIterationSource;

// SyncIteratorBase is the common base class of all sync iterator classes.
// Most importantly this class provides a way of type dispatching (e.g.
// overload resolutions, SFINAE technique, etc.) so that it's possible to
// distinguish sync iterators from anything else. Also it provides common
// implementation of sync iterators.
class SyncIterator : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();
 public:
  using ImplType = SyncIterator*;
  // https://webidl.spec.whatwg.org/#default-iterator-object-kind
  enum class Kind {
    kKey,
    kValue,
    kKeyValue,
  };

  ~SyncIterator() override = default;

  ScriptValue next(ExceptionState& exception_state);
  bool done();
  ScriptValue value();
  ScriptValue Symbol_iterator();

  void Trace(GCVisitor* visitor) const override;

  explicit SyncIterator(JSContext* ctx, std::shared_ptr<PairSyncIterationSource> iteration_source, Kind kind)
      : iteration_source_(std::move(iteration_source)), kind_(kind), ScriptWrappable(ctx) {}

 private:
  const std::shared_ptr<PairSyncIterationSource> iteration_source_;
  const Kind kind_;
};

}  // namespace webf

#endif