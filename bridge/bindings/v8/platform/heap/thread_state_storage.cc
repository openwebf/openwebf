/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "bindings/v8/platform/heap/thread_state_storage.h"
#include <new>
//#include "base/check_op.h"

namespace webf {

constinit thread_local ThreadStateStorage* g_thread_specific_ = nullptr;

// static
ThreadStateStorage ThreadStateStorage::main_thread_state_storage_;

// static
void ThreadStateStorage::AttachMainThread(
    ThreadState& thread_state,
    cppgc::AllocationHandle& allocation_handle,
    cppgc::HeapHandle& heap_handle) {
  g_thread_specific_ = new (&main_thread_state_storage_)
      ThreadStateStorage(thread_state, allocation_handle, heap_handle);
}

// static
void ThreadStateStorage::AttachNonMainThread(
    ThreadState& thread_state,
    cppgc::AllocationHandle& allocation_handle,
    cppgc::HeapHandle& heap_handle) {
  g_thread_specific_ =
      new ThreadStateStorage(thread_state, allocation_handle, heap_handle);
}

// static
void ThreadStateStorage::DetachNonMainThread(
    ThreadStateStorage& thread_state_storage) {
//  CHECK_NE(MainThreadStateStorage(), &thread_state_storage);
//  CHECK_EQ(g_thread_specific_, &thread_state_storage);
  delete &thread_state_storage;
  g_thread_specific_ = nullptr;
}

ThreadStateStorage::ThreadStateStorage(
    ThreadState& thread_state,
    cppgc::AllocationHandle& allocation_handle,
    cppgc::HeapHandle& heap_handle)
    : allocation_handle_(&allocation_handle),
      heap_handle_(&heap_handle),
      thread_state_(&thread_state) {}

}  // namespace webf
