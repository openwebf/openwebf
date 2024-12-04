/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_SIZE_T_H_
#define WEBF_SIZE_T_H_

#include <limits.h>
#include <stdint.h>

namespace webf {

// TLDR: size_t != wtf_size_t
//
// WTF defines wtf_size_t as an unsigned 32 bit integer. This is to align
// with the maximum heap allocation object size and save memory. This deviates
// from Chromium C++ style guide which calls for interfaces to use the
// stdint.h types (aka. int32_t) on the exposed interface.
//
// Matching the external API to match the internal API have a number of
// required properties:
//  - Internal storage for Vector and String are all uint32_t based
//  - Max heap allocation size is kMaxHeapObjectSize (much less than UINTMAX)
//  - static_casts from size_t to uint32_t are not good enough and checked_casts
//    would need to be used.
//  - checked_casts are too slow
//  - consumers of APIs such as WTF::Vector may store their indicies in some
//    other storage and using size_t consumes extra data.
//
// It may be possible in the future to move Vector and String to be size_t
// based and this definition may not be necessary, so long as the internal
// type matches the external type.
using webf_size_t = uint32_t;
const webf_size_t kNotFound = UINT_MAX;

}  // namespace WTF

using webf::kNotFound;
using webf::webf_size_t;

#endif  // WEBF_SIZE_T_H_

