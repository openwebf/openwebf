/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/
#ifndef WEBF_TRACE_WRAPPER_V8_REFERENCE_H
#define WEBF_TRACE_WRAPPER_V8_REFERENCE_H

#include <type_traits>
#include <utility>
#include <v8/v8.h>
#include "foundation/macros.h"
#include "bindings/v8/platform/wtf/type_traits.h"
#include "bindings/v8/platform/wtf/vector_traits.h"

namespace webf {

template <typename T>
using TraceWrapperV8Reference = v8::TracedReference<T>;

}  // namespace blink

namespace webf {

template <typename T>
struct IsTraceable<webf::TraceWrapperV8Reference<T>> {
  WEBF_STATIC_ONLY(IsTraceable);
  static const bool value = true;
};

template <typename T>
struct VectorTraits<webf::TraceWrapperV8Reference<T>>
    : VectorTraitsBase<webf::TraceWrapperV8Reference<T>> {
  WEBF_STATIC_ONLY(VectorTraits);

  static constexpr bool kNeedsDestruction =
      !std::is_trivially_destructible<webf::TraceWrapperV8Reference<T>>::value;

  // TraceWrapperV8Reference has non-trivial construction/copying/moving.
  // However, write barriers in Vector are properly emitted through
  // ConstructTraits and as such the type can be trivially initialized, cleared,
  // copied, and moved.
  static constexpr bool kCanInitializeWithMemset = true;
  static constexpr bool kCanClearUnusedSlotsWithMemset = true;
  // v8::TracedReference assumes that references uniquely point to an internal
  // node.
  static constexpr bool kCanCopyWithMemcpy = false;
  // TODO(chromium:1322114): Temporarily disable move with memcpy to evaluate
  // impact on crashers. Move should always be followed by a clear (non-dtor).
  static constexpr bool kCanMoveWithMemcpy = false;

  // TraceWrapperV8Reference supports concurrent tracing.
  static constexpr bool kCanTraceConcurrently = true;

  // Wanted behavior that should not break for performance reasons.
  static_assert(!kNeedsDestruction,
                "TraceWrapperV8Reference should be trivially destructible.");
};

//template <typename T>
//struct HashTraits<webf::TraceWrapperV8Reference<T>>
//    : GenericHashTraits<webf::TraceWrapperV8Reference<T>> {
//  WEBF_STATIC_ONLY(HashTraits);
//  static constexpr bool kCanTraceConcurrently = true;
//};

}  // namespace webf

#endif  // WEBF_TRACE_WRAPPER_V8_REFERENCE_H
