/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/


#ifndef WEBF_WRITE_BARRIER_H
#define WEBF_WRITE_BARRIER_H

#include <type_traits>

#include <v8/cppgc/member.h>
#include <v8/cppgc/heap-consistency.h>
#include "bindings/v8/platform/wtf/type_traits.h"
#include "foundation/macros.h"

namespace webf {

class WriteBarrier final {
  WEBF_STATIC_ONLY(WriteBarrier);

  using HeapConsistency = cppgc::subtle::HeapConsistency;

 public:
  template <typename T>
  ALWAYS_INLINE static void DispatchForObject(T* element) {
    static_assert(!webf::IsMemberOrWeakMemberType<std::decay_t<T>>::value,
                  "Member and WeakMember should use the other overload.");
    HeapConsistency::WriteBarrierParams params;
    switch (HeapConsistency::GetWriteBarrierType(element, *element, params)) {
      case HeapConsistency::WriteBarrierType::kMarking:
        HeapConsistency::DijkstraWriteBarrier(params, *element);
        break;
      case HeapConsistency::WriteBarrierType::kGenerational:
        HeapConsistency::GenerationalBarrier(params, element);
        break;
      case HeapConsistency::WriteBarrierType::kNone:
        break;
    }
  }

  // Cannot refer to webf::Member and friends here due to cyclic includes.
  template <typename T,
            typename WeaknessTag,
            typename StorageType,
            typename WriteBarrierPolicy,
            typename CheckingPolicy>
  ALWAYS_INLINE static void DispatchForObject(
      cppgc::internal::BasicMember<T,
                                   WeaknessTag,
                                   StorageType,
                                   WriteBarrierPolicy,
                                   CheckingPolicy>* element) {
    HeapConsistency::WriteBarrierParams params;
    switch (HeapConsistency::GetWriteBarrierType(*element, params)) {
      case HeapConsistency::WriteBarrierType::kMarking:
        HeapConsistency::DijkstraWriteBarrier(params, element->Get());
        break;
      case HeapConsistency::WriteBarrierType::kGenerational:
        HeapConsistency::GenerationalBarrier(params, element);
        break;
      case HeapConsistency::WriteBarrierType::kNone:
        break;
    }
  }

  // Cannot refer to webf::Member and friends here due to cyclic includes.
  template <typename T,
            typename WeaknessTag,
            typename StorageType,
            typename WriteBarrierPolicy,
            typename CheckingPolicy>
  ALWAYS_INLINE static bool IsWriteBarrierNeeded(
      cppgc::internal::BasicMember<T,
                                   WeaknessTag,
                                   StorageType,
                                   WriteBarrierPolicy,
                                   CheckingPolicy>* element) {
    HeapConsistency::WriteBarrierParams params;
    return HeapConsistency::GetWriteBarrierType(*element, params) !=
           HeapConsistency::WriteBarrierType::kNone;
  }

  static void DispatchForUncompressedSlot(void* slot, void* value) {
    HeapConsistency::WriteBarrierParams params;
    switch (HeapConsistency::GetWriteBarrierType(slot, value, params)) {
      case HeapConsistency::WriteBarrierType::kMarking:
        HeapConsistency::DijkstraWriteBarrier(params, value);
        break;
      case HeapConsistency::WriteBarrierType::kGenerational:
        HeapConsistency::GenerationalBarrierForUncompressedSlot(params, slot);
        break;
      case HeapConsistency::WriteBarrierType::kNone:
        break;
    }
  }
};

}  // namespace webf

#endif  // WEBF_WRITE_BARRIER_H
