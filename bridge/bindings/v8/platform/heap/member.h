/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_HEAP_MEMBER_H_
#define WEBF_HEAP_MEMBER_H_

#include <v8/cppgc/member.h>
#include "bindings/v8/platform/wtf/type_traits.h"
#include "bindings/v8/platform/heap/persistent.h"
#include "bindings/v8/platform/wtf/hash_traits.h"
#include "bindings/v8/platform/heap/write_barrier.h"
#include "foundation/macros.h"

namespace webf {

template <typename T>
using Member = cppgc::Member<T>;

template <typename T>
using WeakMember = cppgc::WeakMember<T>;

template <typename T>
using UntracedMember = cppgc::UntracedMember<T>;

namespace subtle {
template <typename T>
using UncompressedMember = cppgc::subtle::UncompressedMember<T>;
}

template <typename T>
inline bool IsHashTableDeletedValue(const Member<T>& m) {
  return m == cppgc::kSentinelPointer;
}

constexpr auto kMemberDeletedValue = cppgc::kSentinelPointer;

template <typename T>
struct ThreadingTrait<Member<T>> {
  WEBF_STATIC_ONLY(ThreadingTrait);
  static constexpr ThreadAffinity kAffinity = ThreadingTrait<T>::kAffinity;
};

template <typename T>
struct ThreadingTrait<WeakMember<T>> {
  WEBF_STATIC_ONLY(ThreadingTrait);
  static constexpr ThreadAffinity kAffinity = ThreadingTrait<T>::kAffinity;
};

template <typename T>
struct ThreadingTrait<UntracedMember<T>> {
  WEBF_STATIC_ONLY(ThreadingTrait);
  static constexpr ThreadAffinity kAffinity = ThreadingTrait<T>::kAffinity;
};

template <typename T>
inline void swap(Member<T>& a, Member<T>& b) {
  a.Swap(b);
}

static constexpr bool kBlinkMemberGCHasDebugChecks =
    !std::is_same<cppgc::internal::DefaultMemberCheckingPolicy,
                  cppgc::internal::DisabledCheckingPolicy>::value;

// We should never bloat the Member<> wrapper.
// NOTE: The Member<void*> works as we never use this Member in a trace method.
static_assert(kBlinkMemberGCHasDebugChecks ||
                  sizeof(Member<void*>) <= sizeof(void*),
              "Member<> should stay small!");

//}  // namespace webf
//
//namespace webf {

template <typename T>
struct IsTraceable<Member<T>> {
  WEBF_STATIC_ONLY(IsTraceable);
  static const bool value = true;
};

template <typename T>
struct IsWeak<WeakMember<T>> : std::true_type {};

template <typename T>
struct IsTraceable<WeakMember<T>> {
  WEBF_STATIC_ONLY(IsTraceable);
  static const bool value = true;
};

// Peeker type that allows for using all kinds of Member, Persistent, and T*
// interchangeably. This is necessary for collection methods that are called
// directly with any of those types.
template <typename T>
class ValuePeeker final {
  WEBF_DISALLOW_NEW();

 public:
  // NOLINTNEXTLINE
  ALWAYS_INLINE ValuePeeker(T* ptr) : ptr_(ptr) {}
  template <typename U>
  // NOLINTNEXTLINE
  ALWAYS_INLINE ValuePeeker(const Member<U>& m) : ptr_(m.Get()) {}
  template <typename U>
  // NOLINTNEXTLINE
  ALWAYS_INLINE ValuePeeker(const WeakMember<U>& m) : ptr_(m.Get()) {}
  template <typename U>
  // NOLINTNEXTLINE
  ALWAYS_INLINE ValuePeeker(const UntracedMember<U>& m)
      : ptr_(m.Get()) {}
  template <typename U>
  // NOLINTNEXTLINE
  ALWAYS_INLINE ValuePeeker(const Persistent<U>& p) : ptr_(p.Get()) {}
  template <typename U>
  // NOLINTNEXTLINE
  ALWAYS_INLINE ValuePeeker(const WeakPersistent<U>& p)
      : ptr_(p.Get()) {}

  // NOLINTNEXTLINE
  ALWAYS_INLINE operator T*() const { return ptr_; }
  // NOLINTNEXTLINE
  ALWAYS_INLINE operator Member<T>() const { return ptr_; }
  // NOLINTNEXTLINE
  ALWAYS_INLINE operator WeakMember<T>() const { return ptr_; }
  // NOLINTNEXTLINE
  ALWAYS_INLINE operator UntracedMember<T>() const { return ptr_; }

 private:
  T* ptr_;
};

// Default hash for hash tables with Member<>-derived elements.
template <typename T, typename MemberType>
struct BaseMemberHashTraits : SimpleClassHashTraits<MemberType> {
  WEBF_STATIC_ONLY(BaseMemberHashTraits);

  // Heap hash containers allow to operate with raw pointers, e.g.
  //   HeapHashSet<Member<GCed>> set;
  //   set.find(raw_ptr);
  // Therefore, provide two hashing functions, one for raw pointers, another for
  // Member. Prefer compressing raw pointers instead of decompressing Members,
  // assuming the former is cheaper.
  static unsigned GetHash(const T* key) {
#if defined(CPPGC_POINTER_COMPRESSION)
    cppgc::internal::CompressedPointer st(key);
#else
    cppgc::internal::RawPointer st(key);
#endif
    return webf::GetHash(st.GetAsInteger());
  }
  template <typename Member,
            std::enable_if_t<webf::IsAnyMemberType<Member>::value>* = nullptr>
  static unsigned GetHash(const Member& m) {
    return webf::GetHash(m.GetRawStorage().GetAsInteger());
  }

  static constexpr bool kEmptyValueIsZero = true;

  using PeekInType = ValuePeeker<T>;
  using PeekOutType = T*;
  using IteratorGetType = MemberType*;
  using IteratorConstGetType = const MemberType*;
  using IteratorReferenceType = MemberType&;
  using IteratorConstReferenceType = const MemberType&;

  static PeekOutType Peek(const MemberType& value) { return value.Get(); }

  static void ConstructDeletedValue(MemberType& slot) {
    slot = cppgc::kSentinelPointer;
  }

  static bool IsDeletedValue(const MemberType& value) {
    return value == cppgc::kSentinelPointer;
  }
};

// Custom HashTraits<Member<Type>> can inherit this type.
template <typename T>
struct MemberHashTraits : BaseMemberHashTraits<T, webf::Member<T>> {
  static constexpr bool kCanTraceConcurrently = true;
};
template <typename T>
struct HashTraits<webf::Member<T>> : MemberHashTraits<T> {};

// Custom HashTraits<WeakMember<Type>> can inherit this type.
template <typename T>
struct WeakMemberHashTraits : BaseMemberHashTraits<T, webf::WeakMember<T>> {
  static constexpr bool kCanTraceConcurrently = true;
};
template <typename T>
struct HashTraits<webf::WeakMember<T>> : WeakMemberHashTraits<T> {};

// Custom HashTraits<UntracedMember<Type>> can inherit this type.
template <typename T>
struct UntracedMemberHashTraits
    : BaseMemberHashTraits<T, webf::UntracedMember<T>> {};
template <typename T>
struct HashTraits<webf::UntracedMember<T>> : UntracedMemberHashTraits<T> {};

template <typename T>
class MemberConstructTraits {
  WEBF_STATIC_ONLY(MemberConstructTraits);

 public:
  template <typename... Args>
  static T* Construct(void* location, Args&&... args) {
    // `Construct()` creates a new Member which must not be visible to the
    // concurrent marker yet, similar to regular ctors in Member.
    return new (base::NotNullTag::kNotNull, location) T(std::forward<Args>(args)...);
  }

  template <typename... Args>
  static T* ConstructAndNotifyElement(void* location, Args&&... args) {
    // `ConstructAndNotifyElement()` updates an existing Member which might
    // also be concurrently traced while we update it. The regular ctors
    // for Member don't use an atomic write which can lead to data races.
    T* object = new (base::NotNullTag::kNotNull, location)
        T(std::forward<Args>(args)..., typename T::AtomicInitializerTag());
    NotifyNewElement(object);
    return object;
  }

  static void NotifyNewElement(T* element) {
    webf::WriteBarrier::DispatchForObject(element);
  }

  static void NotifyNewElements(T* array, size_t len) {
    // Checking the first element is sufficient for determining whether a
    // marking or generational barrier is required.
    if (LIKELY((len == 0) || !webf::WriteBarrier::IsWriteBarrierNeeded(array)))
      return;

    while (len-- > 0) {
      webf::WriteBarrier::DispatchForObject(array);
      array++;
    }
  }
};

//template <typename T, typename Traits, typename Allocator>
//class ConstructTraits<webf::Member<T>, Traits, Allocator> final
//    : public MemberConstructTraits<webf::Member<T>> {};

//template <typename T, typename Traits, typename Allocator>
//class ConstructTraits<webf::WeakMember<T>, Traits, Allocator> final
//    : public MemberConstructTraits<webf::WeakMember<T>> {};

}  // namespace webf

#endif  // WEBF_HEAP_MEMBER_H_
