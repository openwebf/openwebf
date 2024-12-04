/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_HEAP_PERSISTENT_H_
#define WEBF_HEAP_PERSISTENT_H_

#include <v8/v8.h>
#include <v8/cppgc/cross-thread-persistent.h>
#include <v8/cppgc/persistent.h>
#include <v8/cppgc/source-location.h>
#include "bindings/v8/platform/heap/heap_buildflags.h"
#include "bindings/v8/platform/wtf/vector_traits.h"
#include "bindings/v8/platform/wtf/hash_traits.h"
#include "foundation/macros.h"

// Required to optimize away locations for builds that do not need them to avoid
// binary size blowup.
#if BUILDFLAG(VERBOSE_PERSISTENT)
#define PERSISTENT_LOCATION_FROM_HERE webf::PersistentLocation::Current()
#else  // !BUILDFLAG(VERBOSE_PERSISTENT)
#define PERSISTENT_LOCATION_FROM_HERE webf::PersistentLocation()
#endif  // !BUILDFLAG(VERBOSE_PERSISTENT)

namespace webf {

template <typename T>
using Persistent = cppgc::Persistent<T>;

template <typename T>
using WeakPersistent = cppgc::WeakPersistent<T>;

using PersistentLocation = cppgc::SourceLocation;

template <typename T>
Persistent<T> WrapPersistent(
    T* value,
    const PersistentLocation& loc = PERSISTENT_LOCATION_FROM_HERE) {
  return Persistent<T>(value, loc);
}

template <typename T>
WeakPersistent<T> WrapWeakPersistent(
    T* value,
    const PersistentLocation& loc = PERSISTENT_LOCATION_FROM_HERE) {
  return WeakPersistent<T>(value, loc);
}

template <typename U, typename T, typename weakness>
cppgc::internal::BasicPersistent<U, weakness> DownCast(
    const cppgc::internal::BasicPersistent<T, weakness>& p) {
  return p.template To<U>();
}

template <typename U, typename T, typename weakness>
cppgc::internal::BasicCrossThreadPersistent<U, weakness> DownCast(
    const cppgc::internal::BasicCrossThreadPersistent<T, weakness>& p) {
  return p.template To<U>();
}

template <typename T,
          typename = std::enable_if_t<webf::IsGarbageCollectedType<T>::value>>
Persistent<T> WrapPersistentIfNeeded(T* value) {
  return Persistent<T>(value);
}

template <typename T>
T& WrapPersistentIfNeeded(T& value) {
  return value;
}

}  // namespace webf

namespace webf {

template <typename T>
struct PersistentVectorTraitsBase : VectorTraitsBase<T> {
  WEBF_STATIC_ONLY(PersistentVectorTraitsBase);
  static const bool kCanInitializeWithMemset = true;
};

template <typename T>
struct VectorTraits<webf::Persistent<T>>
    : PersistentVectorTraitsBase<webf::Persistent<T>> {};

template <typename T>
struct VectorTraits<webf::WeakPersistent<T>>
    : PersistentVectorTraitsBase<webf::WeakPersistent<T>> {};

template <typename T, typename PersistentType>
struct BasePersistentHashTraits : SimpleClassHashTraits<PersistentType> {
  template <typename U>
  static unsigned GetHash(const U& key) {
    return webf::GetHash<T*>(key);
  }

  template <typename U, typename V>
  static bool Equal(const U& a, const V& b) {
    return a == b;
  }

  // TODO: Implement proper const'ness for iterator types. Requires support
  // in the marking Visitor.
  using PeekInType = T*;
  using IteratorGetType = PersistentType*;
  using IteratorConstGetType = const PersistentType*;
  using IteratorReferenceType = PersistentType&;
  using IteratorConstReferenceType = const PersistentType&;

  using PeekOutType = T*;

  template <typename U>
  static void Store(const U& value, PersistentType& storage) {
    storage = value;
  }

  static PeekOutType Peek(const PersistentType& value) { return value; }

  static void ConstructDeletedValue(PersistentType& slot) {
    new (&slot) PersistentType(cppgc::kSentinelPointer);
  }

  static bool IsDeletedValue(const PersistentType& value) {
    return value.Get() == cppgc::kSentinelPointer;
  }
};

template <typename T>
struct HashTraits<webf::Persistent<T>>
    : BasePersistentHashTraits<T, webf::Persistent<T>> {};

template <typename T>
struct HashTraits<webf::WeakPersistent<T>>
    : BasePersistentHashTraits<T, webf::WeakPersistent<T>> {};

}  // namespace webf

namespace base {

template <typename T>
struct IsWeakReceiver;

template <typename T>
struct IsWeakReceiver<webf::WeakPersistent<T>> : std::true_type {};

template <typename>
struct MaybeValidTraits;

// TODO(https://crbug.com/653394): Consider returning a thread-safe best
// guess of validity. MaybeValid() can be invoked from an arbitrary thread.
template <typename T>
struct MaybeValidTraits<webf::WeakPersistent<T>> {
  static bool MaybeValid(const webf::WeakPersistent<T>& p) { return true; }
};

}  // namespace base

#endif  // WEBF_HEAP_PERSISTENT_H_
