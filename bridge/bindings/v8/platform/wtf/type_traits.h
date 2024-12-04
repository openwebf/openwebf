/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
 */



#ifndef WEBF_TYPE_TRAITS_H
#define WEBF_TYPE_TRAITS_H

#include <cstddef>
#include <type_traits>
#include <utility>
#include "bindings/v8/base/compiler_specific.h"
#include "bindings/v8/for_build/build_config.h"
#include <v8/cppgc/type-traits.h>
#include <concepts>
#include <functional>

namespace webf {

// Returns a string that contains the type name of |T| as a substring.
template <typename T>
inline const char* GetStringWithTypeName() {
  return PRETTY_FUNCTION;
}

template <typename T, typename U>
struct IsSubclass {
 private:
  typedef char YesType;
  struct NoType {
    char padding[8];
  };

  static YesType SubclassCheck(U*);
  static NoType SubclassCheck(...);
  static T* t_;

 public:
  static const bool value = sizeof(SubclassCheck(t_)) == sizeof(YesType);
};

template <typename T, template <typename... V> class U>
struct IsSubclassOfTemplate {
 private:
  typedef char YesType;
  struct NoType {
    char padding[8];
  };

  template <typename... W>
  static YesType SubclassCheck(U<W...>*);
  static NoType SubclassCheck(...);
  static T* t_;

 public:
  static const bool value = sizeof(SubclassCheck(t_)) == sizeof(YesType);
};

template <typename T, template <typename V, size_t W> class U>
struct IsSubclassOfTemplateTypenameSize {
 private:
  typedef char YesType;
  struct NoType {
    char padding[8];
  };

  template <typename X, size_t Y>
  static YesType SubclassCheck(U<X, Y>*);
  static NoType SubclassCheck(...);
  static T* t_;

 public:
  static const bool value = sizeof(SubclassCheck(t_)) == sizeof(YesType);
};

template <typename T, template <typename V, size_t W, typename X> class U>
struct IsSubclassOfTemplateTypenameSizeTypename {
 private:
  typedef char YesType;
  struct NoType {
    char padding[8];
  };

  template <typename Y, size_t Z, typename A>
  static YesType SubclassCheck(U<Y, Z, A>*);
  static NoType SubclassCheck(...);
  static T* t_;

 public:
  static const bool value = sizeof(SubclassCheck(t_)) == sizeof(YesType);
};

template <typename T>
struct IsTraceable : cppgc::internal::IsTraceable<T> {};

template <typename T>
struct IsGarbageCollectedType
    : cppgc::internal::IsGarbageCollectedOrMixinType<T> {};

template <typename T>
struct IsWeak : cppgc::internal::IsWeak<T> {};

template <typename T>
struct IsMemberType : std::bool_constant<cppgc::IsMemberTypeV<T>> {};

template <typename T>
struct IsWeakMemberType : std::bool_constant<cppgc::IsWeakMemberTypeV<T>> {};

template <typename T>
struct IsMemberOrWeakMemberType
    : std::bool_constant<cppgc::IsMemberTypeV<T> ||
                         cppgc::IsWeakMemberTypeV<T>> {};

template <typename T>
struct IsAnyMemberType
    : std::bool_constant<IsMemberOrWeakMemberType<T>::value ||
                         cppgc::IsUntracedMemberTypeV<T>> {};

template <typename T, typename U>
struct IsTraceable<std::pair<T, U>>
    : std::bool_constant<IsTraceable<T>::value || IsTraceable<U>::value> {};

enum WeakHandlingFlag {
  kNoWeakHandling,
  kWeakHandling,
};

// This is for tracing inside collections that have special support for weak
// pointers.
//
// Structure:
// - `Trace()`: Traces the contents.
// - `IsAlive()`: Returns true if the contents are still considered alive, and
// false otherwise.
//
// Default implementation for non-weak types is to use the regular non-weak
// TraceTrait. Default implementation for types with weakness is to
// delegate to sub types until reaching WeakMember or KeyValuePair which
// have defined weakness semantics.
template <WeakHandlingFlag weakness, typename T, typename Traits>
struct TraceInCollectionTrait;

template <typename T>
inline constexpr WeakHandlingFlag kWeakHandlingTrait =
    IsWeak<T>::value ? kWeakHandling : kNoWeakHandling;

// This is used to check that DISALLOW_NEW objects are not
// stored in off-heap Vectors, HashTables etc.
template <typename T>
concept IsDisallowNew = requires { typename T::IsDisallowNewMarker; };

template <>
class IsGarbageCollectedType<void> {
 public:
  static const bool value = false;
};

template <typename T,
          bool = std::is_function<typename std::remove_const<
                     typename std::remove_pointer<T>::type>::type>::value ||
                 std::is_void<typename std::remove_const<
                     typename std::remove_pointer<T>::type>::type>::value>
class IsPointerToGarbageCollectedType {
 public:
  static const bool value = false;
};

template <typename T>
class IsPointerToGarbageCollectedType<T*, false> {
 public:
  static const bool value = IsGarbageCollectedType<T>::value;
};

template <typename T>
concept IsStackAllocatedType =
    requires { typename T::IsStackAllocatedTypeMarker; };

template <typename T>
struct IsPointerToGced {
 private:
  typedef char YesType;
  struct NoType {
    char padding[8];
  };

  template <typename X,
            typename = std::enable_if_t<webf::IsGarbageCollectedType<X>::value>>
  static YesType SubclassCheck(X**);
  static NoType SubclassCheck(...);
  static T* t_;

 public:
  static const bool value = sizeof(SubclassCheck(t_)) == sizeof(YesType);
};

}  // namespace webf

using webf::IsGarbageCollectedType;

#endif  // WEBF_TYPE_TRAITS_H
