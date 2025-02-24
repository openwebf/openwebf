// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (C) 2024-present The WebF authors. All rights reserved.

#include "core/dom/intersection_observer_entry.h"
#include "core/dom/element.h"

namespace webf {

IntersectionObserverEntry::IntersectionObserverEntry(ExecutingContext* context,
                                                     bool isIntersecting,
                                                     double intersectionRatio,
                                                     Element* target)
    : ScriptWrappable(context->ctx()),
      isIntersecting_(isIntersecting),
      intersectionRatio_(intersectionRatio),
      target_(target) {}

void IntersectionObserverEntry::Trace(GCVisitor* visitor) const {
  visitor->TraceMember(target_);
}

}  // namespace webf
