/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "touch.h"
#include "bindings/qjs/cppgc/gc_visitor.h"

namespace webf {

Touch* Touch::Create(ExecutingContext* context, ExceptionState& exception_state) {
  return MakeGarbageCollected<Touch>(context, exception_state);
}

Touch* Touch::Create(ExecutingContext* context,
                     const std::shared_ptr<TouchInit>& initializer,
                     ExceptionState& exception_state) {
  return MakeGarbageCollected<Touch>(context, initializer, exception_state);
}

Touch* Touch::Create(ExecutingContext* context, NativeTouch* native_touch) {
  return MakeGarbageCollected<Touch>(context, native_touch);
}

Touch::Touch(ExecutingContext* context, ExceptionState& exception_state) : ScriptWrappable(context->ctx()) {}

Touch::Touch(ExecutingContext* context, const std::shared_ptr<TouchInit>& initializer, ExceptionState& exception_state)
    : ScriptWrappable(context->ctx()),
      identifier_(initializer->identifier()),
      target_(initializer->target()),
      clientX_(initializer->clientX()),
      clientY_(initializer->clientY()),
      screenX_(initializer->screenX()),
      screenY_(initializer->screenY()),
      pageX_(initializer->pageX()),
      pageY_(initializer->pageY()),
      radiusX_(initializer->radiusX()),
      radiusY_(initializer->radiusY()),
      rotationAngle_(initializer->rotationAngle()),
      force_(initializer->force()) {}

Touch::Touch(ExecutingContext* context, NativeTouch* native_touch)
    : ScriptWrappable(context->ctx()),
      target_(DynamicTo<EventTarget>(BindingObject::From(native_touch->target))),
      identifier_(native_touch->identifier),
      clientX_(native_touch->clientX),
      clientY_(native_touch->clientY),
      screenX_(native_touch->screenX),
      screenY_(native_touch->screenY),
      pageX_(native_touch->pageX),
      pageY_(native_touch->pageY),
      radiusX_(native_touch->radiusX),
      radiusY_(native_touch->radiusY),
      rotationAngle_(native_touch->rotationAngle),
      force_(native_touch->force),
      altitude_angle_(native_touch->altitudeAngle),
      azimuth_angle_(native_touch->azimuthAngle) {
  assert(target_ != nullptr);
}

double Touch::altitudeAngle() const {
  return altitude_angle_;
}

double Touch::azimuthAngle() const {
  return azimuth_angle_;
}

double Touch::clientX() const {
  return clientX_;
}

double Touch::clientY() const {
  return clientY_;
}

double Touch::force() const {
  return force_;
}

double Touch::identifier() const {
  return identifier_;
}

double Touch::pageX() const {
  return pageX_;
}

double Touch::pageY() const {
  return pageY_;
}

double Touch::radiusX() const {
  return radiusX_;
}

double Touch::radiusY() const {
  return radiusY_;
}

double Touch::rotationAngle() const {
  return rotationAngle_;
}

double Touch::screenX() const {
  return screenX_;
}

double Touch::screenY() const {
  return screenY_;
}

EventTarget* Touch::target() const {
  return target_;
}

void Touch::Trace(GCVisitor* visitor) const {
  visitor->TraceMember(target_);
}

const TouchPublicMethods* Touch::touchPublicMethods() {
  static TouchPublicMethods touch_public_methods;
  return &touch_public_methods;
}

}  // namespace webf
