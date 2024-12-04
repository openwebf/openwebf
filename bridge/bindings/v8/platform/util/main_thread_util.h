/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_MAIN_THREAD_UTIL_H
#define WEBF_MAIN_THREAD_UTIL_H

#include "bindings/v8/base/threading/platform_thread.h"
#include "bindings/v8/for_build/build_config.h"
//#include "third_party/abseil-cpp/absl/base/attributes.h"

namespace webf {

extern base::PlatformThreadId g_main_thread_identifier;

// This function must be called exactly once from the main thread before using
// anything else in WTF.
void Initialize();

// thread_local variables can't be exported on Windows, so we use an extra
// function call on component builds. Also, thread_local on Android is emulated
// by the runtime lib; gettid(3) in bionic already caches tid in a TLS variable.
#if BUILDFLAG(IS_ANDROID) || (defined(COMPONENT_BUILD) && BUILDFLAG(IS_WIN))
WTF_EXPORT bool IsMainThread();
#else
// TODO Don't introduce ABSL for now
// ABSL_CONST_INIT extern thread_local bool g_is_main_thread;
extern thread_local bool g_is_main_thread;
inline bool IsMainThread() {
  return g_is_main_thread;
}
#endif

}  // namespace webf

using webf::IsMainThread;

#endif  // WEBF_MAIN_THREAD_UTIL_H
