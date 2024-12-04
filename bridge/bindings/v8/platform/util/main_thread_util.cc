/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "bindings/v8/platform/util/main_thread_util.h"

#include "bindings/v8/for_build/build_config.h"
//#include "bindings/v8/base/check.h"
//#include "third_party/abseil-cpp/absl/base/attributes.h"
//#include "bindings/v8/platform/wtf/allocator/partitions.h"
//#include "bindings/v8/platform/wtf/date_math.h"
#include "bindings/v8/platform/wtf/dtoa.h"
//#include "bindings/v8/platform/wtf/functional.h"
#include "bindings/v8/platform/wtf/stack_util.h"
//#include "bindings/v8/platform/wtf/text/atomic_string.h"
//#include "bindings/v8/platform/wtf/text/copy_lchars_from_uchar_source.h"
//#include "bindings/v8/platform/wtf/text/string_statics.h"

namespace webf {

namespace {

bool g_initialized = false;

#if defined(COMPONENT_BUILD) && BUILDFLAG(IS_WIN)
ABSL_CONST_INIT thread_local bool g_is_main_thread = false;
#endif

}  // namespace

base::PlatformThreadId g_main_thread_identifier;

#if BUILDFLAG(IS_ANDROID)
// On Android going through libc (gettid) is faster than runtime-lib emulation.
bool IsMainThread() {
  return CurrentThread() == g_main_thread_identifier;
}
#elif defined(COMPONENT_BUILD) && BUILDFLAG(IS_WIN)
bool IsMainThread() {
  return g_is_main_thread;
}
#else
// TODO Don't introduce ABSL for now
//ABSL_CONST_INIT thread_local bool g_is_main_thread = false;
thread_local bool g_is_main_thread = false;
#endif

void Initialize() {
  // TODO webf
//  // WTF, and Blink in general, cannot handle being re-initialized.
//  // Make that explicit here.
//  CHECK(!g_initialized);
//  g_initialized = true;
//#if !BUILDFLAG(IS_ANDROID)
//  g_is_main_thread = true;
//#endif
//  g_main_thread_identifier = CurrentThread();
//
//  Threading::Initialize();
//
//  internal::InitializeDoubleConverter();
//
//  internal::InitializeMainThreadStackEstimate();

  // TODO not include WTF's AtomicString and StringStatics for now
  // AtomicString::Init();
//  StringStatics::Init();
}

}  // namespace webf

