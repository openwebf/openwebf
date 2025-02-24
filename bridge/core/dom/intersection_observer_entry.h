// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (C) 2024-present The WebF authors. All rights reserved.

#ifndef WEBF_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_ENTRY_H_
#define WEBF_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_ENTRY_H_

#include "bindings/qjs/cppgc/member.h"
#include "bindings/qjs/script_wrappable.h"

namespace webf {

class Element;

struct NativeIntersectionObserverEntry : public DartReadable {
  int8_t is_intersecting;
  double intersectionRatio;
  NativeBindingObject* target;
};

class IntersectionObserverEntry final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  IntersectionObserverEntry() = delete;
  explicit IntersectionObserverEntry(ExecutingContext* context,
                                     bool isIntersecting,
                                     double intersectionRatio,
                                     Element* target);

  // TODO(pengfei12.guo): not supported
  // IDL interface
  // double time() const { return time_; }
  double intersectionRatio() const { return intersectionRatio_; }
  // DOMRectReadOnly* boundingClientRect() const;
  // DOMRectReadOnly* rootBounds() const;
  // DOMRectReadOnly* intersectionRect() const;
  // bool isVisible() const {
  // return geometry_.IsVisible();
  //}

  bool isIntersecting() const { return isIntersecting_; }

  Element* target() const { return target_.Get(); }

  // TODO(pengfei12.guo): IntersectionGeometry not supported
  // blink-internal interface
  // const IntersectionGeometry& GetGeometry() const { return geometry_; }
  void Trace(GCVisitor*) const override;

 private:
  // IntersectionGeometry geometry_;
  double intersectionRatio_;
  bool isIntersecting_;
  // DOMHighResTimeStamp time_;
  Member<Element> target_;
};

}  // namespace webf

#endif  // WEBF_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_ENTRY_H_
