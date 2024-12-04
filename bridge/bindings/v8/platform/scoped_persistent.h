/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_SCOPED_PERSISTENT_H
#define WEBF_SCOPED_PERSISTENT_H


#include <v8/v8.h>

namespace webf {

// Holds a persistent handle to a V8 object; use ScopedPersistent instead of
// directly using v8::Persistent. Introducing a (non-weak) ScopedPersistent
// has a risk of producing memory leaks, ask blink-reviews-bindings@ for a
// review.
template <typename T>
class ScopedPersistent {
//  USING_FAST_MALLOC(ScopedPersistent);

 public:
  ScopedPersistent() = default;
  ScopedPersistent(v8::Isolate* isolate, v8::Local<T> handle)
      : handle_(isolate, handle) {}
  ScopedPersistent(const ScopedPersistent&) = delete;
  ScopedPersistent& operator=(const ScopedPersistent&) = delete;

  ~ScopedPersistent() { Clear(); }

  inline v8::Local<T> NewLocal(v8::Isolate* isolate) const {
    return v8::Local<T>::New(isolate, handle_);
  }

  // If you don't need to get weak callback, use setPhantom instead.
  // setPhantom is faster than setWeak.
  template <typename P>
  void SetWeak(P* parameters,
               void (*callback)(const v8::WeakCallbackInfo<P>&),
               v8::WeakCallbackType type = v8::WeakCallbackType::kParameter) {
    handle_.SetWeak(parameters, callback, type);
  }

  // Turns this handle into a weak phantom handle without
  // finalization callback.
  void SetPhantom() { handle_.SetWeak(); }

  void ClearWeak() { handle_.template ClearWeak<void>(); }

  bool IsEmpty() const { return handle_.IsEmpty(); }
  bool IsWeak() const { return handle_.IsWeak(); }

  void Set(v8::Isolate* isolate, v8::Local<T> handle) {
    handle_.Reset(isolate, handle);
  }

  // Note: This is clear in the std::unique_ptr sense, not the v8::Local sense.
  void Clear() { handle_.Reset(); }

  bool operator==(const ScopedPersistent<T>& other) {
    return handle_ == other.handle_;
  }

  template <class S>
  bool operator==(const v8::Local<S> other) const {
    return handle_ == other;
  }

  inline v8::Persistent<T>& Get() { return handle_; }

 private:
  v8::Persistent<T> handle_;
};

}  // namespace webf

#endif  // WEBF_SCOPED_PERSISTENT_H
