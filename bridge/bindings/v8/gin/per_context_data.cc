/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "bindings/v8/gin/per_context_data.h"

#include "bindings/v8/gin/public/context_holder.h"
#include "bindings/v8/gin/public/wrapper_info.h"

namespace gin {

PerContextData::PerContextData(ContextHolder* context_holder,
                               v8::Local<v8::Context> context)
    : context_holder_(context_holder), runner_(nullptr) {
  context->SetAlignedPointerInEmbedderData(
      int{kPerContextDataStartIndex} + kEmbedderWebf, this);
}

// TODO webf
//PerContextData::~PerContextData() {
//  v8::HandleScope handle_scope(context_holder_->isolate());
//  context_holder_->context()->SetAlignedPointerInEmbedderData(
//      int{kPerContextDataStartIndex} + kEmbedderWebf, NULL);
//}

// static
PerContextData* PerContextData::From(v8::Local<v8::Context> context) {
  return static_cast<PerContextData*>(
      context->GetAlignedPointerFromEmbedderData(kEncodedValueIndex));
}

}  // namespace gin

