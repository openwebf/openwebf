/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef BASE_RANGES_FUNCTIONAL_H_
#define BASE_RANGES_FUNCTIONAL_H_

#include <functional>
#include <type_traits>
#include <utility>

namespace base {

namespace ranges {

// Simplified implementations of C++20's std::ranges comparison function
// objects. As opposed to the std::ranges implementation, these versions do not
// constrain the passed-in types.
//
// Reference: https://wg21.link/range.cmp
using equal_to = std::equal_to<>;
using not_equal_to = std::not_equal_to<>;
using greater = std::greater<>;
using less = std::less<>;
using greater_equal = std::greater_equal<>;
using less_equal = std::less_equal<>;

}  // namespace ranges

}  // namespace base

#endif  // BASE_RANGES_FUNCTIONAL_H_

