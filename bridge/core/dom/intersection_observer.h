// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (C) 2024-present The WebF authors. All rights reserved.

#ifndef WEBF_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_H_
#define WEBF_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_H_

#include <unordered_set>
#include <vector>
#include "bindings/qjs/atomic_string.h"
#include "bindings/qjs/cppgc/garbage_collected.h"
#include "bindings/qjs/cppgc/member.h"
#include "bindings/qjs/exception_state.h"
#include "core/binding_object.h"
#include "out/qjs_intersection_observer_init.h"

namespace webf {

class Element;
class ExceptionState;
class IntersectionObserverEntry;
class Node;
class ScriptState;

class IntersectionObserver final : public BindingObject {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // The IntersectionObserver can be configured to notify based on changes to
  // how much of the target element's area intersects with the root, or based on
  // changes to how much of the root element's area intersects with the
  // target. Examples illustrating the distinction:
  //
  //     1.0 of target,         0.5 of target,         1.0 of target,
  //      0.25 of root           0.5 of root            1.0 of root
  //  +------------------+   +------------------+   *~~~~~~~~~~~~~~~~~~*
  //  |   //////////     |   |                  |   ;//////////////////;
  //  |   //////////     |   |                  |   ;//////////////////;
  //  |   //////////     |   ;//////////////////;   ;//////////////////;
  //  |                  |   ;//////////////////;   ;//////////////////;
  //  +------------------+   *~~~~~~~~~~~~~~~~~~*   *~~~~~~~~~~~~~~~~~~*
  //                         ////////////////////
  //                         ////////////////////
  //                         ////////////////////
  // enum ThresholdInterpretation { kFractionOfTarget, kFractionOfRoot };

  //  This value can be used to detect transitions between non-intersecting or
  //  edge-adjacent (i.e., zero area) state, and intersecting by any non-zero
  //  number of pixels.
  //  static constexpr float kMinimumThreshold =
  //      IntersectionGeometry::kMinimumThreshold;

  // Used to specify when callbacks should be invoked with new notifications.
  // Blink-internal users of IntersectionObserver will have their callbacks
  // invoked synchronously either at the end of a lifecycle update or in the
  // middle of the lifecycle post layout. Javascript observers will PostTask to
  // invoke their callbacks.
  // enum DeliveryBehavior { kDeliverDuringPostLayoutSteps, kDeliverDuringPostLifecycleSteps, kPostTaskToDeliver };

  // Used to specify whether the margins apply to the root element or the source
  // element. The effect of the root element margins is that intermediate
  // scrollers clip content by its bounding box without considering margins.
  // That is, margins only apply to the last scroller (root). The effect of
  // source element margins is that the margins apply to the first / deepest
  // clipper, but do not apply to any other clippers. Note that in a case of a
  // single clipper, the two approaches are equivalent.
  //
  // Note that the percentage margin is resolved against the root rect, even
  // when the margin is applied to the target.
  // enum MarginTarget { kApplyMarginToRoot, kApplyMarginToTarget };

  static IntersectionObserver* Create(ExecutingContext* context,
                                      const std::shared_ptr<QJSFunction>& function,
                                      ExceptionState& exception_state);

  static IntersectionObserver* Create(ExecutingContext* context,
                                      const std::shared_ptr<QJSFunction>& function,
                                      const std::shared_ptr<IntersectionObserverInit>& observer_init,
                                      ExceptionState& exception_state);

  IntersectionObserver(ExecutingContext* context, const std::shared_ptr<QJSFunction>& function);
  IntersectionObserver(ExecutingContext* context,
                       const std::shared_ptr<QJSFunction>& function,
                       const std::shared_ptr<IntersectionObserverInit>& observer_init);

  // TODO(pengfei12.guo): Params not supported
  // struct Params {
  //   WEBF_STACK_ALLOCATED();
  //
  //  public:
  //   Node* root;
  //   Vector<Length> margin;
  //   MarginTarget margin_target = kApplyMarginToRoot;
  //   Vector<Length> scroll_margin;
  //
  //   // Elements should be in the range [0,1], and are interpreted according to
  //   // the given `semantics`.
  //   Vector<float> thresholds;
  //   ThresholdInterpretation semantics = kFractionOfTarget;
  //
  //   DeliveryBehavior behavior = kDeliverDuringPostLifecycleSteps;
  //   // Specifies the minimum period between change notifications.
  //   base::TimeDelta delay;
  //   bool track_visibility = false;
  //   bool always_report_root_bounds = false;
  //   // Indicates whether the overflow clip edge should be used instead of the
  //   // bounding box if appropriate.
  //   bool use_overflow_clip_edge = false;
  //   bool needs_initial_observation_with_detached_target = true;
  // };

  NativeValue HandleCallFromDartSide(const AtomicString& method,
                                     int32_t argc,
                                     const NativeValue* argv,
                                     Dart_Handle dart_object) override;

  // API methods.
  void observe(Element*, ExceptionState&);
  void unobserve(Element*, ExceptionState&);
  void disconnect(ExceptionState&);
  // TODO(pengfei12.guo): not supported
  // std::vector<Member<IntersectionObserverEntry>> takeRecords(ExceptionState&);

  // API attributes.
  [[nodiscard]] Node* root() const { return root_; }
  // TODO(pengfei12.guo): not supported
  // AtomicString rootMargin() const;
  // TODO(pengfei12.guo): not supported
  // AtomicString scrollMargin() const;

  [[nodiscard]] const std::vector<double>& thresholds() const { return thresholds_; }
  // TODO(pengfei12.guo): not supported
  // DOMHighResTimeStamp delay() const {
  //   if (delay_ != std::numeric_limits<int64_t>::min() && delay_ != std::numeric_limits<int64_t>::max()) {
  //     return delay_ / 1000;
  //   }
  //   return (delay_ < 0) ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int64_t>::max();
  // }

  // An observer can either track intersections with an explicit root Node,
  // or with the the top-level frame's viewport (the "implicit root").  When
  // tracking the implicit root, root_ will be null, but because root_ is a
  // weak pointer, we cannot surmise that this observer tracks the implicit
  // root just because root_ is null.  Hence root_is_implicit_.
  [[nodiscard]] bool RootIsImplicit() const {
    // return root_is_implicit_;
    // If the root option is not specified, the viewport is used as the root element by default.
    return root_ == nullptr;
  }

  // TODO(pengfei12.guo): TimeDelta not support
  // base::TimeDelta GetEffectiveDelay() const;

  // TODO(pengfei12.guo): RootMargin not support
  // std::vector<Length> RootMargin() const {
  //  return margin_target_ == kApplyMarginToRoot ? margin_ : Vector<Length>();
  //}

  // TODO(pengfei12.guo): TargetMargin not support
  // Vector<Length> TargetMargin() const {
  //  return margin_target_ == kApplyMarginToTarget ? margin_ : Vector<Length>();
  //}

  // TODO(pengfei12.guo): ScrollMargin not support
  // Vector<Length> ScrollMargin() const { return scroll_margin_; }

  // TODO(pengfei12.guo): ComputeIntersections impl by dart
  // Returns the number of IntersectionObservations that recomputed geometry.
  // int64_t ComputeIntersections(unsigned flags, ComputeIntersectionsContext&);

  // TODO(pengfei12.guo): GetUkmMetricId not support
  // bool IsInternal() const;

  // TODO(pengfei12.guo): GetUkmMetricId not support
  // The metric id for tracking update time via UpdateTime metrics, or null for
  // internal intersection observers without explicit metrics.
  // std::optional<LocalFrameUkmAggregator::MetricId> GetUkmMetricId() const {
  //  return ukm_metric_id_;
  //}

  // Returns false if this observer has an explicit root node which has been
  // deleted; true otherwise.
  bool RootIsValid() const;

  void Trace(GCVisitor*) const override;

 private:
  // We use UntracedMember<> here to do custom weak processing.
  Node* root_;
  std::vector<double> thresholds_;

  // TODO(pengfei12.guo): not support
  // const std::vector<Length> margin_;
  // const std::vector<Length> scroll_margin_;
  // const MarginTarget margin_target_;
  // const unsigned root_is_implicit_ : 1;
  // const unsigned track_visibility_ : 1;
  // const unsigned track_fraction_of_root_ : 1;
  // const unsigned always_report_root_bounds_ : 1;
  // const unsigned use_overflow_clip_edge_ : 1;

  std::shared_ptr<QJSFunction> function_;
};

}  // namespace webf

#endif  // WEBF_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_H_
