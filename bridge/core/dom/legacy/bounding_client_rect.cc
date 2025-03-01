/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "bounding_client_rect.h"
#include "core/executing_context.h"

namespace webf {

BoundingClientRect* BoundingClientRect::Create(ExecutingContext* context, NativeBindingObject* native_binding_object) {
  return MakeGarbageCollected<BoundingClientRect>(context, native_binding_object);
}

BoundingClientRect::BoundingClientRect(ExecutingContext* context, NativeBindingObject* native_binding_object)
    : BindingObject(context->ctx(), native_binding_object),
      extra_(static_cast<BoundingClientRectData*>(native_binding_object->extra)) {}

NativeValue BoundingClientRect::HandleCallFromDartSide(const AtomicString& method,
                                                       int32_t argc,
                                                       const NativeValue* argv,
                                                       Dart_Handle dart_object) {
  return Native_NewNull();
}

const BoundingClientRectPublicMethods* BoundingClientRect::boundingClientRectPublicMethods() {
  static BoundingClientRectPublicMethods bounding_client_rect_public_methods;
  return &bounding_client_rect_public_methods;
}

}  // namespace webf
