/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef PLATFORM_WEBF_STACK_UTIL_H_
#define PLATFORM_WEBF_STACK_UTIL_H_

#include <stddef.h>
#include <stdint.h>
#include "bindings/v8/base/compiler_specific.h"
#include "bindings/v8/for_build/build_config.h"

namespace webf {

size_t GetUnderestimatedStackSize();
void* GetStackStart();

// Returns the current stack position such that it works correctly with ASAN and
// SafeStack. Must be marked noinline because it relies on compiler intrinsics
// that report the current stack frame and if inlined it could report a position
// above the current stack position.
NOINLINE uintptr_t GetCurrentStackPosition();

namespace internal {

extern uintptr_t g_main_thread_stack_start;
extern uintptr_t g_main_thread_underestimated_stack_size;

void InitializeMainThreadStackEstimate();

#if BUILDFLAG(IS_WIN) && defined(COMPILER_MSVC)
size_t ThreadStackSize();
#endif

}  // namespace internal

// Returns true if the function is not called on the main thread. Note carefully
// that this function may have false positives, i.e. it can return true even if
// we are on the main thread. If the function returns false, we are certainly
// on the main thread.
inline bool MayNotBeMainThread() {
  uintptr_t dummy;
  uintptr_t address_diff =
      internal::g_main_thread_stack_start - reinterpret_cast<uintptr_t>(&dummy);
  // This is a fast way to judge if we are in the main thread.
  // If |&dummy| is within |s_mainThreadUnderestimatedStackSize| byte from
  // the stack start of the main thread, we judge that we are in
  // the main thread.
  return address_diff >= internal::g_main_thread_underestimated_stack_size;
}

}  // namespace webf

#endif  // PLATFORM_WEBF_STACK_UTIL_H_

