/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "bindings/v8/base/threading/platform_thread.h"
//#include "bindings/v8/base/threading/thread_id_name_manager.h"

//#include "base/task/current_thread.h"
//#include "third_party/abseil-cpp/absl/base/attributes.h"

#if BUILDFLAG(IS_FUCHSIA)
#include "base/fuchsia/scheduler.h"
#endif

namespace base {

namespace {

// TODO Don't introduce ABSL for now
// ABSL_CONST_INIT thread_local ThreadType current_thread_type =
thread_local ThreadType current_thread_type =
    ThreadType::kDefault;

}  // namespace

// static
void PlatformThreadBase::SetCurrentThreadType(ThreadType thread_type) {
  // TODO webf
//  MessagePumpType message_pump_type = MessagePumpType::DEFAULT;
//  if (CurrentIOThread::IsSet()) {
//    message_pump_type = MessagePumpType::IO;
//  }
//#if !BUILDFLAG(IS_NACL)
//  else if (CurrentUIThread::IsSet()) {
//    message_pump_type = MessagePumpType::UI;
//  }
//#endif
//  internal::SetCurrentThreadType(thread_type, message_pump_type);
}

// static
ThreadType PlatformThreadBase::GetCurrentThreadType() {
  return current_thread_type;
}

// static
// TODO WEBF
//std::optional<TimeDelta> PlatformThreadBase::GetThreadLeewayOverride() {
//#if BUILDFLAG(IS_FUCHSIA)
//  // On Fuchsia, all audio threads run with the CPU scheduling profile that uses
//  // an interval of |kAudioSchedulingPeriod|. Using the default leeway may lead
//  // to some tasks posted to audio threads to be executed too late (see
//  // http://crbug.com/1368858).
//  if (GetCurrentThreadType() == ThreadType::kRealtimeAudio)
//    return kAudioSchedulingPeriod;
//#endif
//  return std::nullopt;
//}

// static
void PlatformThreadBase::SetNameCommon(const std::string& name) {
  // TODO WEBF
//  ThreadIdNameManager::GetInstance()->SetName(name);
}

namespace internal {

// TODO webf not include for now
//void SetCurrentThreadType(ThreadType thread_type,
//                          MessagePumpType pump_type_hint) {
//  CHECK_LE(thread_type, ThreadType::kMaxValue);
//  SetCurrentThreadTypeImpl(thread_type, pump_type_hint);
//  current_thread_type = thread_type;
//}

}  // namespace internal

}  // namespace base
