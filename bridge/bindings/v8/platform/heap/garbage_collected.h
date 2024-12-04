/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_GARBAGE_COLLECTED_H
#define WEBF_GARBAGE_COLLECTED_H

#include <type_traits>

#include <v8/v8.h>
#include <v8/v8-cppgc.h>
#include <v8/cppgc/allocation.h>
#include <v8/cppgc/type-traits.h>
#include <concepts>
#include "bindings/v8/platform/heap/thread_state_storage.h"

namespace cppgc {
class LivenessBroker;
class Visitor;
}  // namespace cppgc

namespace webf {

template <typename T>
using GarbageCollected = cppgc::GarbageCollected<T>;

using GarbageCollectedMixin = cppgc::GarbageCollectedMixin;

using LivenessBroker = cppgc::LivenessBroker;

using Visitor = cppgc::Visitor;

// Default MakeGarbageCollected: Constructs an instance of T, which is a garbage
// collected type.
template <typename T, typename... Args>
T* MakeGarbageCollected(Args&&... args) {
  return cppgc::MakeGarbageCollected<T>(
      ThreadStateStorageFor<ThreadingTrait<T>::kAffinity>::GetState()
          ->allocation_handle(),
      std::forward<Args>(args)...);
}

using AdditionalBytes = cppgc::AdditionalBytes;

// Constructs an instance of T, which is a garbage collected type. This special
// version takes size which enables constructing inline objects.
template <typename T, typename... Args>
T* MakeGarbageCollected(AdditionalBytes additional_bytes, Args&&... args) {
  return cppgc::MakeGarbageCollected<T>(
      ThreadStateStorageFor<ThreadingTrait<T>::kAffinity>::GetState()
          ->allocation_handle(),
      std::forward<AdditionalBytes>(additional_bytes),
      std::forward<Args>(args)...);
}

}  // namespace blink

namespace base::internal {

// v8 lives outside the Chromium repository and cannot rely on //base concepts
// like `DISALLOW_UNRETAINED()`.
//template <typename T>
//requires cppgc::IsGarbageCollectedOrMixinTypeV<T>
//    inline constexpr bool kCustomizeSupportsUnretained<T> = false;

}  // namespace base::internal

#endif  // WEBF_GARBAGE_COLLECTED_H