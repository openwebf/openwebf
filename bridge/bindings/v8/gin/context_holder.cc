/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "bindings/v8/gin/public/context_holder.h"

#include <memory>
#include <assert.h>

//#include "bindings/v8/base/check.h"
#include "bindings/v8/gin/per_context_data.h"

namespace gin {

ContextHolder::ContextHolder(v8::Isolate* isolate)
    : isolate_(isolate) {
}

ContextHolder::~ContextHolder() {
  // PerContextData needs to be destroyed before the context.
  data_.reset();
}

void ContextHolder::SetContext(v8::Local<v8::Context> context) {
  assert(context_.IsEmpty());
  context_.Reset(isolate_, context);
  context_.AnnotateStrongRetainer("gin::ContextHolder::context_");
  data_ = std::make_unique<PerContextData>(this, context);
}

}  // namespace gin
