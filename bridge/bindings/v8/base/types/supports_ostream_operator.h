/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef BASE_TYPES_SUPPORTS_OSTREAM_OPERATOR_H_
#define BASE_TYPES_SUPPORTS_OSTREAM_OPERATOR_H_

#include <ostream>
#include <type_traits>
#include <utility>

namespace base::internal {

// Detects whether using operator<< would work.
//
// Note that the above #include of <ostream> is necessary to guarantee
// consistent results here for basic types.
template <typename T>
concept SupportsOstreamOperator =
    requires(const T& t, std::ostream& os) { os << t; };

}  // namespace base::internal

#endif  // BASE_TYPES_SUPPORTS_OSTREAM_OPERATOR_H_

