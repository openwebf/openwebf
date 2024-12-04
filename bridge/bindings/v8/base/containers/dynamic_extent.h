/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef BASE_CONTAINERS_DYNAMIC_EXTENT_H_
#define BASE_CONTAINERS_DYNAMIC_EXTENT_H_

#include <cstddef>
#include <limits>

namespace base {

// [views.constants]
inline constexpr size_t dynamic_extent = std::numeric_limits<size_t>::max();

}  // namespace base

#endif  // BASE_CONTAINERS_DYNAMIC_EXTENT_H_

