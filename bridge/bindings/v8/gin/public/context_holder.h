/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef GIN_PUBLIC_CONTEXT_HOLDER_H_
#define GIN_PUBLIC_CONTEXT_HOLDER_H_

#include <memory>

#include "bindings/v8/base/memory/raw_ptr.h"
#include "bindings/v8/gin/gin_export.h"
#include <v8/v8-context.h>
#include <v8/v8-forward.h>
#include <v8/v8-persistent-handle.h>

namespace gin {

// Gin embedder that store embedder data in v8::Contexts must do so in a
// single field with the index kPerContextDataStartIndex + GinEmbedder-enum.
// The field at kDebugIdIndex is treated specially by V8 and is reserved for
// a V8 debugger implementation (not used by gin).
enum ContextEmbedderDataFields {
  kDebugIdIndex = v8::Context::kDebugIdIndex,
  kPerContextDataStartIndex,
};

class PerContextData;

// ContextHolder is a generic class for holding a v8::Context.
class GIN_EXPORT ContextHolder {
 public:
  explicit ContextHolder(v8::Isolate* isolate);
  ContextHolder(const ContextHolder&) = delete;
  ContextHolder& operator=(const ContextHolder&) = delete;
  ~ContextHolder();

  v8::Isolate* isolate() const { return isolate_; }

  v8::Local<v8::Context> context() const {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

  void SetContext(v8::Local<v8::Context> context);

 private:
  v8::Isolate *isolate_;
  v8::UniquePersistent<v8::Context> context_;
  std::unique_ptr<PerContextData> data_;
};

}  // namespace gin

#endif  // GIN_PUBLIC_CONTEXT_HOLDER_H_