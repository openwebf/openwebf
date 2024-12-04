/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_SAFE_CONVERSIONS_ARM_IMPL_H
#define WEBF_SAFE_CONVERSIONS_ARM_IMPL_H
// IWYU pragma: private, include "base/numerics/safe_conversions.h"

#include <stdint.h>
#include <type_traits>

#include "bindings/v8/base/numerics/safe_conversions_impl.h"

namespace base {
namespace internal {

// Fast saturation to a destination type.
template <typename Dst, typename Src>
struct SaturateFastAsmOp {
  static constexpr bool is_supported =
      kEnableAsmCode && std::is_signed_v<Src> && std::is_integral_v<Dst> &&
      std::is_integral_v<Src> &&
      IntegerBitsPlusSign<Src>::value <= IntegerBitsPlusSign<int32_t>::value &&
      IntegerBitsPlusSign<Dst>::value <= IntegerBitsPlusSign<int32_t>::value &&
      !IsTypeInRangeForNumericType<Dst, Src>::value;

  __attribute__((always_inline)) static Dst Do(Src value) {
    int32_t src = value;
    typename std::conditional<std::is_signed_v<Dst>, int32_t, uint32_t>::type
        result;
    if (std::is_signed_v<Dst>) {
      asm("ssat %[dst], %[shift], %[src]"
          : [dst] "=r"(result)
          : [src] "r"(src), [shift] "n"(IntegerBitsPlusSign<Dst>::value <= 32
                                            ? IntegerBitsPlusSign<Dst>::value
                                            : 32));
    } else {
      asm("usat %[dst], %[shift], %[src]"
          : [dst] "=r"(result)
          : [src] "r"(src), [shift] "n"(IntegerBitsPlusSign<Dst>::value < 32
                                            ? IntegerBitsPlusSign<Dst>::value
                                            : 31));
    }
    return static_cast<Dst>(result);
  }
};

}  // namespace internal
}  // namespace base

#endif  // WEBF_SAFE_CONVERSIONS_ARM_IMPL_H
