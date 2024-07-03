#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <ostream>

namespace webf {

extern "C" {

void init_webf_polyfill(RustValue<void, void> handle);

}  // extern "C"

}  // namespace webf
