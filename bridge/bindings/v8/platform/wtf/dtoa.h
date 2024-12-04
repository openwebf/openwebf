/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef PLATFORM_WEBF_DTOA_H_
#define PLATFORM_WEBF_DTOA_H_

#include "bindings/v8/base/numerics/safe_conversions.h"
//#include "bindings/v8/platform/wtf/text/ascii_ctype.h"
//#include "bindings/v8/platform/wtf/text/wtf_uchar.h"

namespace webf {

// Size = 80 for sizeof(DtoaBuffer) + some sign bits, decimal point, 'e',
// exponent digits.
const unsigned kNumberToStringBufferLength = 96;
typedef char NumberToStringBuffer[kNumberToStringBufferLength];

const char* NumberToString(double, NumberToStringBuffer);
const char* NumberToFixedPrecisionString(
    double,
    unsigned significant_figures,
    NumberToStringBuffer);
const char* NumberToFixedWidthString(double,
                                                unsigned decimal_places,
                                                NumberToStringBuffer);

//double ParseDouble(const LChar* string,
//                              size_t length,
//                              size_t& parsed_length);
//double ParseDouble(const UChar* string,
//                              size_t length,
//                              size_t& parsed_length);

namespace internal {

void InitializeDoubleConverter();

}  // namespace internal

}  // namespace webf

using webf::NumberToFixedPrecisionString;
using webf::NumberToFixedWidthString;
using webf::NumberToString;
using webf::NumberToStringBuffer;
//using webf::ParseDouble;

#endif  // PLATFORM_WEBF_DTOA_H_
