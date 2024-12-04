/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef BASE_CONTAINERS_UTIL_H_
#define BASE_CONTAINERS_UTIL_H_

#include <stdint.h>

namespace base {

// TODO(crbug.com/40565371): What we really need is for checked_math.h to be
// able to do checked arithmetic on pointers.
template <typename T>
inline uintptr_t get_uintptr(const T* t) {
  return reinterpret_cast<uintptr_t>(t);
}

}  // namespace base

#endif  // BASE_CONTAINERS_UTIL_H_

