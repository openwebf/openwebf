/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "bindings/v8/v8_initializer.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include <v8/v8-profiler.h>
#include <string>
#include "bindings/v8/platform/util/main_thread_util.h"
#include <cassert>
//#include "bindings/v8/platform/script_state.h"
#include "bindings/v8/platform/v8_per_isolate_data.h"

#if defined(V8_USE_EXTERNAL_STARTUP_DATA)
#include "gin/public/v8_snapshot_file_type.h"
#endif

namespace webf {

#if BUILDFLAG(IS_WIN)
// Defined in v8_initializer_win.cc.
bool FilterETWSessionByURLCallback(v8::Local<v8::Context> context,
                                   const std::string& json_payload);
#endif  // BUILDFLAG(IS_WIN)

namespace {

//v8::MaybeLocal<v8::Promise> HostImportModuleDynamically(
//    v8::Local<v8::Context> context,
//    v8::Local<v8::Data> v8_host_defined_options,
//    v8::Local<v8::Value> v8_referrer_resource_url,
//    v8::Local<v8::String> v8_specifier,
//    v8::Local<v8::FixedArray> v8_import_attributes) {
//  v8::Isolate* isolate = context->GetIsolate();
//  ScriptState* script_state = ScriptState::From(isolate, context);
//
//  Modulator* modulator = Modulator::From(script_state);
//  if (!modulator) {
//    // Inactive browsing context (detached frames) doesn't have a modulator.
//    // We chose to return a rejected promise (which may never get to catch(),
//    // since MicrotaskQueue for a detached frame is never consumed).
//    //
//    // This is a hack to satisfy V8 API expectation, which are:
//    // - return non-empty v8::Promise value
//    //   (can either be fulfilled/rejected), or
//    // - throw exception && return Empty value
//    // See crbug.com/972960 .
//    //
//    // We use the v8 promise API directly here.
//    // We can't use ScriptPromiseResolverBase here since it assumes a valid
//    // ScriptState.
//    v8::Local<v8::Promise::Resolver> resolver;
//    if (!v8::Promise::Resolver::New(script_state->GetContext())
//             .ToLocal(&resolver)) {
//      // Note: V8 should have thrown an exception in this case,
//      //       so we return Empty.
//      return v8::MaybeLocal<v8::Promise>();
//    }
//
//    v8::Local<v8::Promise> promise = resolver->GetPromise();
//    v8::Local<v8::Value> error = V8ThrowException::CreateError(
//        script_state->GetIsolate(),
//        "Cannot import module from an inactive browsing context.");
//    resolver->Reject(script_state->GetContext(), error).ToChecked();
//    return promise;
//  }
//
//  String specifier =
//      ToCoreStringWithNullCheck(script_state->GetIsolate(), v8_specifier);
//  KURL referrer_resource_url;
//  if (v8_referrer_resource_url->IsString()) {
//    String referrer_resource_url_str =
//        ToCoreString(script_state->GetIsolate(),
//                     v8::Local<v8::String>::Cast(v8_referrer_resource_url));
//    if (!referrer_resource_url_str.empty())
//      referrer_resource_url = KURL(NullURL(), referrer_resource_url_str);
//  }
//
//  ModuleRequest module_request(
//      specifier, TextPosition::MinimumPosition(),
//      ModuleRecord::ToBlinkImportAttributes(
//          script_state->GetContext(), v8::Local<v8::Module>(),
//          v8_import_attributes, /*v8_import_attributes_has_positions=*/false));
//
//  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver<IDLAny>>(
//      script_state,
//      ExceptionContext(ExceptionContextType::kUnknown, "", "import"));
//
//  String invalid_attribute_key;
//  if (module_request.HasInvalidImportAttributeKey(&invalid_attribute_key)) {
//    resolver->Reject(V8ThrowException::CreateTypeError(
//        script_state->GetIsolate(),
//        "Invalid attribute key \"" + invalid_attribute_key + "\"."));
//  } else {
//    ReferrerScriptInfo referrer_info =
//        ReferrerScriptInfo::FromV8HostDefinedOptions(
//            context, v8_host_defined_options, referrer_resource_url);
//
//    modulator->ResolveDynamically(module_request, referrer_info, resolver);
//  }
//
//  return resolver->Promise().V8Promise();
//}

// https://html.spec.whatwg.org/C/#hostgetimportmetaproperties
//void HostGetImportMetaProperties(v8::Local<v8::Context> context,
//                                 v8::Local<v8::Module> module,
//                                 v8::Local<v8::Object> meta) {
//  v8::Isolate* isolate = context->GetIsolate();
//  ScriptState* script_state = ScriptState::From(isolate, context);
//  v8::HandleScope handle_scope(isolate);
//
//  Modulator* modulator = Modulator::From(script_state);
//  if (!modulator)
//    return;
//
//  ModuleImportMeta host_meta = modulator->HostGetImportMetaProperties(module);
//
//  // 6. Return « Record { [[Key]]: "url", [[Value]]: urlString }, Record {
//  // [[Key]]: "resolve", [[Value]]: resolveFunction } ». [spec text]
//  v8::Local<v8::String> url_key = V8String(isolate, "url");
//  v8::Local<v8::String> url_value = V8String(isolate, host_meta.Url());
//
//  v8::Local<v8::String> resolve_key = V8String(isolate, "resolve");
//  v8::Local<v8::Function> resolve_value =
//      host_meta.MakeResolveV8Function(modulator);
//  resolve_value->SetName(resolve_key);
//
//  meta->CreateDataProperty(context, url_key, url_value).ToChecked();
//  meta->CreateDataProperty(context, resolve_key, resolve_value).ToChecked();
//}

struct PrintV8OOM {
  const char* location;
  const v8::OOMDetails& details;
};

std::ostream& operator<<(std::ostream& os, const PrintV8OOM& oom_details) {
//  const auto [location, details] = oom_details;
//  os << "V8 " << (details.is_heap_oom ? "javascript" : "process") << " OOM ("
//     << location;
//  if (details.detail) {
//    os << "; detail: " << details.detail;
//  }
//  os << ").";
  return os;
}

}  // namespace

// static
void V8Initializer::InitializeV8Common(v8::Isolate* isolate) {
  // Set up garbage collection before setting up anything else as V8 may trigger
  // GCs during Blink setup.
//  V8PerIsolateData::From(isolate)->SetGCCallbacks(
//      isolate, V8GCController::GcPrologue, V8GCController::GcEpilogue);
//  ThreadState::Current()->AttachToIsolate(
//      isolate, EmbedderGraphBuilder::BuildEmbedderGraphCallback);
//  V8PerIsolateData::From(isolate)->SetActiveScriptWrappableManager(
//      MakeGarbageCollected<ActiveScriptWrappableManager>());

  isolate->SetMicrotasksPolicy(v8::MicrotasksPolicy::kScoped);
//  isolate->SetHostImportModuleDynamicallyCallback(HostImportModuleDynamically);
//  isolate->SetHostInitializeImportMetaObjectCallback(
//      HostGetImportMetaProperties);
//  isolate->SetMetricsRecorder(std::make_shared<V8MetricsRecorder>(isolate));

#if BUILDFLAG(IS_WIN)
  isolate->SetFilterETWSessionByURLCallback(FilterETWSessionByURLCallback);
#endif  // BUILDFLAG(IS_WIN)

//  V8ContextSnapshot::EnsureInterfaceTemplates(isolate);

//  if (v8::HeapProfiler* profiler = isolate->GetHeapProfiler()) {
//    profiler->SetGetDetachednessCallback(
//        V8GCController::DetachednessFromWrapper, nullptr);
//  }
}

// Callback functions called when V8 encounters a fatal or OOM error.
// Keep them outside the anonymous namespace such that ChromeCrash recognizes
// them.
void ReportV8FatalError(const char* location, const char* message) {
//  LOG(FATAL) << "V8 error: " << message << " (" << location << ").";
}

void ReportV8OOMError(const char* location, const v8::OOMDetails& details) {
//  if (location) {
//    static crash_reporter::CrashKeyString<64> location_key("v8-oom-location");
//    location_key.Set(location);
//  }
//
//  if (details.detail) {
//    static crash_reporter::CrashKeyString<128> detail_key("v8-oom-detail");
//    detail_key.Set(details.detail);
//  }
//
//  LOG(ERROR) << PrintV8OOM{location, details};
//  OOM_CRASH(0);
}

namespace {
//class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
// public:
//  ArrayBufferAllocator() : total_allocation_(0) {
//    // size_t may be equivalent to uint32_t or uint64_t, cast all values to
//    // uint64_t to compare.
//    // TODO webf not include SysInfo for now
////    uint64_t virtual_size = base::SysInfo::AmountOfVirtualMemory();
//    uint64_t size_t_max = std::numeric_limits<std::size_t>::max();
//    uint64_t virtual_size = size_t_max - 1024;
//    DCHECK(virtual_size < size_t_max);
//    // If AmountOfVirtualMemory() returns 0, there is no limit on virtual
//    // memory, do not limit the total allocation. Otherwise, Limit the total
//    // allocation to reserve up to 2 GiB virtual memory space for other
//    // components.
//    uint64_t memory_reserve = 2ull * 1024 * 1024 * 1024;  // 2 GiB
//    if (virtual_size > memory_reserve * 2) {
//      max_allocation_ = static_cast<size_t>(virtual_size - memory_reserve);
//    } else {
//      max_allocation_ = static_cast<size_t>(virtual_size / 2);
//    }
//  }
//
//  // Allocate() methods return null to signal allocation failure to V8, which
//  // should respond by throwing a RangeError, per
//  // http://www.ecma-international.org/ecma-262/6.0/#sec-createbytedatablock.
//  void* Allocate(size_t size) override {
//    if (max_allocation_ != 0 &&
//        std::atomic_load(&total_allocation_) > max_allocation_ - size)
//      return nullptr;
//    void* result = ArrayBufferContents::AllocateMemoryOrNull(
//        size, ArrayBufferContents::kZeroInitialize);
//    if (max_allocation_ != 0 && result)
//      total_allocation_.fetch_add(size, std::memory_order_relaxed);
//    return result;
//  }
//
//  void* AllocateUninitialized(size_t size) override {
//    if (max_allocation_ != 0 &&
//        std::atomic_load(&total_allocation_) > max_allocation_ - size)
//      return nullptr;
//    void* result = ArrayBufferContents::AllocateMemoryOrNull(
//        size, ArrayBufferContents::kDontInitialize);
//    if (max_allocation_ != 0 && result)
//      total_allocation_.fetch_add(size, std::memory_order_relaxed);
//    return result;
//  }
//
//  void Free(void* data, size_t size) override {
//    if (max_allocation_ != 0 && data)
//      total_allocation_.fetch_sub(size, std::memory_order_relaxed);
//    ArrayBufferContents::FreeMemory(data);
//  }
//
// private:
//  // Total memory allocated in bytes.
//  std::atomic_size_t total_allocation_;
//  // If |max_allocation_| is 0, skip these atomic operations on
//  // |total_allocation_|.
//  size_t max_allocation_;
//};

V8PerIsolateData::V8ContextSnapshotMode GetV8ContextSnapshotMode() {
#if BUILDFLAG(USE_V8_CONTEXT_SNAPSHOT)
  if (Platform::Current()->IsTakingV8ContextSnapshot())
    return V8PerIsolateData::V8ContextSnapshotMode::kTakeSnapshot;
  if (gin::GetLoadedSnapshotFileType() ==
      gin::V8SnapshotFileType::kWithAdditionalContext) {
    return V8PerIsolateData::V8ContextSnapshotMode::kUseSnapshot;
  }
#endif  // BUILDFLAG(USE_V8_CONTEXT_SNAPSHOT)
  return V8PerIsolateData::V8ContextSnapshotMode::kDontUseSnapshot;
}

}  // namespace

void V8Initializer::InitializeIsolateHolder(
    const intptr_t* reference_table,
    const std::string js_command_line_flags) {
//  DEFINE_STATIC_LOCAL(ArrayBufferAllocator, array_buffer_allocator, ());
//  gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode,
//                                 &array_buffer_allocator, reference_table,
//                                 js_command_line_flags, ReportV8FatalError,
//                                 ReportV8OOMError);
}

v8::Isolate* V8Initializer::InitializeMainThread() {
//  DCHECK(IsMainThread());
//  ThreadScheduler* scheduler = ThreadScheduler::Current();

  V8PerIsolateData::V8ContextSnapshotMode snapshot_mode =
      GetV8ContextSnapshotMode();
  v8::CreateHistogramCallback create_histogram_callback = nullptr;
  v8::AddHistogramSampleCallback add_histogram_sample_callback = nullptr;
  // We don't log histograms when taking a snapshot.
  if (snapshot_mode != V8PerIsolateData::V8ContextSnapshotMode::kTakeSnapshot) {
//    create_histogram_callback = CreateHistogram;
//    add_histogram_sample_callback = AddHistogramSample;
  }
  v8::Isolate* isolate = V8PerIsolateData::Initialize(
//      scheduler->V8TaskRunner(), scheduler->V8LowPriorityTaskRunner(),
      snapshot_mode, create_histogram_callback, add_histogram_sample_callback);
//  scheduler->SetV8Isolate(isolate);

  // ThreadState::isolate_ needs to be set before setting the EmbedderHeapTracer
  // as setting the tracer indicates that a V8 garbage collection should trace
  // over to Blink.
//  DCHECK(ThreadStateStorage::MainThreadStateStorage());

  InitializeV8Common(isolate);

//  isolate->AddMessageListenerWithErrorLevel(
//      MessageHandlerInMainThread,
//      v8::Isolate::kMessageError | v8::Isolate::kMessageWarning |
//          v8::Isolate::kMessageInfo | v8::Isolate::kMessageDebug |
//          v8::Isolate::kMessageLog);
//  isolate->SetFailedAccessCheckCallbackFunction(
//      V8Initializer::FailedAccessCheckCallbackInMainThread);
//  isolate->SetModifyCodeGenerationFromStringsCallback(
//      CodeGenerationCheckCallbackInMainThread);
//  if (RuntimeEnabledFeatures::V8IdleTasksEnabled()) {
//    V8PerIsolateData::EnableIdleTasks(
//        isolate, std::make_unique<V8IdleTaskRunner>(scheduler));
//  }

//  isolate->SetPromiseRejectCallback(PromiseRejectHandlerInMainThread);

//  V8PerIsolateData::From(isolate)->SetThreadDebugger(
//      std::make_unique<MainThreadDebugger>(isolate));

//  if (Platform::Current()->IsolateStartsInBackground()) {
    // If we do not track widget visibility, then assume conservatively that
    // the isolate is in background. This reduces memory usage.
//    isolate->IsolateInBackgroundNotification();
//  }

  return isolate;
}

}  // namespace webf

