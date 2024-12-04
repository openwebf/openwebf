/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_HEAP_CUSTOM_SPACES_H_
#define WEBF_HEAP_CUSTOM_SPACES_H_

#include <memory>
#include <vector>

#include "bindings/v8/platform/platform_export.h"
#include <v8/cppgc/custom-space.h>

namespace webf {

// The following defines custom spaces that are used to partition Oilpan's heap.
// Each custom space is assigned to a type partition using `cppgc::SpaceTrait`.
// It is expected that `kSpaceIndex` uniquely identifies a space and that the
// indices of all custom spaces form a sequence starting at 0. See
// `cppgc::CustomSpace` for details.

class PLATFORM_EXPORT CompactableHeapVectorBackingSpace
    : public cppgc::CustomSpace<CompactableHeapVectorBackingSpace> {
 public:
  static constexpr cppgc::CustomSpaceIndex kSpaceIndex = 0;
  static constexpr bool kSupportsCompaction = true;
};

class PLATFORM_EXPORT CompactableHeapHashTableBackingSpace
    : public cppgc::CustomSpace<CompactableHeapHashTableBackingSpace> {
 public:
  static constexpr cppgc::CustomSpaceIndex kSpaceIndex = 1;
  static constexpr bool kSupportsCompaction = true;
};

class PLATFORM_EXPORT NodeSpace : public cppgc::CustomSpace<NodeSpace> {
 public:
  static constexpr cppgc::CustomSpaceIndex kSpaceIndex = 2;
};

class PLATFORM_EXPORT CSSValueSpace : public cppgc::CustomSpace<CSSValueSpace> {
 public:
  static constexpr cppgc::CustomSpaceIndex kSpaceIndex = 3;
};

class PLATFORM_EXPORT LayoutObjectSpace
    : public cppgc::CustomSpace<LayoutObjectSpace> {
 public:
  static constexpr cppgc::CustomSpaceIndex kSpaceIndex = 4;
};

struct PLATFORM_EXPORT CustomSpaces final {
  static std::vector<std::unique_ptr<cppgc::CustomSpaceBase>>
  CreateCustomSpaces();
};

}  // namespace webf

#endif  // WEBF_HEAP_CUSTOM_SPACES_H_

