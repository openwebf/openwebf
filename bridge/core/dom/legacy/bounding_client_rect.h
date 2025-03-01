/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_CORE_DOM_LEGACY_BOUNDING_CLIENT_RECT_H_
#define BRIDGE_CORE_DOM_LEGACY_BOUNDING_CLIENT_RECT_H_

#include "bindings/qjs/exception_state.h"
#include "bindings/qjs/script_wrappable.h"
#include "core/binding_object.h"
#include "plugin_api/bounding_client_rect.h"

namespace webf {

class ExecutingContext;

struct BoundingClientRectData {
  double x;
  double y;
  double width;
  double height;
  double top;
  double right;
  double bottom;
  double left;
};

class BoundingClientRect : public BindingObject {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using ImplType = BoundingClientRect*;
  BoundingClientRect() = delete;
  static BoundingClientRect* Create(ExecutingContext* context, NativeBindingObject* native_binding_object);
  explicit BoundingClientRect(ExecutingContext* context, NativeBindingObject* native_binding_object);

  NativeValue HandleCallFromDartSide(const AtomicString& method,
                                     int32_t argc,
                                     const NativeValue* argv,
                                     Dart_Handle dart_object) override;

  double x() const { return extra_->x; }
  double y() const { return extra_->y; }
  double width() const { return extra_->width; }
  double height() const { return extra_->height; }
  double top() const { return extra_->top; }
  double right() const { return extra_->right; }
  double bottom() const { return extra_->bottom; }
  double left() const { return extra_->left; }
  const BoundingClientRectPublicMethods* boundingClientRectPublicMethods();

 private:
  BoundingClientRectData* extra_ = nullptr;
};

}  // namespace webf

#endif  // BRIDGE_CORE_DOM_LEGACY_BOUNDING_CLIENT_RECT_H_
