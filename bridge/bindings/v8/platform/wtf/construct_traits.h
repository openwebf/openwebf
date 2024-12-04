/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_CONSTRUCT_TRAITS_H_
#define WEBF_CONSTRUCT_TRAITS_H_

#include "bindings/v8/platform/wtf/type_traits.h"
#include "bindings/v8/platform/wtf/vector_traits.h"
#include "foundation/macros.h"

namespace webf {

// ConstructTraits is used to construct elements in WTF collections.
// All in-place constructions that may assign Oilpan objects must be
// dispatched through ConstructAndNotifyElement.
template <typename T, typename Traits, typename Allocator>
class ConstructTraits {
  WEBF_STATIC_ONLY(ConstructTraits);

 public:
  // Construct a single element that would otherwise be constructed using
  // placement new.
  template <typename... Args>
  static T* Construct(void* location, Args&&... args) {
    return ::new (base::NotNullTag::kNotNull, location)
        T(std::forward<Args>(args)...);
  }

  // After constructing elements using memcopy or memmove (or similar)
  // |NotifyNewElement| needs to be called to propagate that information.
  static void NotifyNewElement(T* element) {
    Allocator::template NotifyNewObject<T, Traits>(element);
  }

  template <typename... Args>
  static T* ConstructAndNotifyElement(void* location, Args&&... args) {
    T* object = Construct(location, std::forward<Args>(args)...);
    NotifyNewElement(object);
    return object;
  }

  static void NotifyNewElements(T* array, size_t len) {
    Allocator::template NotifyNewObjects<T, Traits>(array, len);
  }
};

}  // namespace webf

#endif  // WEBF_CONSTRUCT_TRAITS_H_
