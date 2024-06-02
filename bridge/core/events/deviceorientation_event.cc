/*
 * Copyright (C) 2024-present The WebF authors. All rights reserved.
 */

#include "deviceorientation_event.h"
#include "qjs_deviceorientation_event.h"
#include "qjs_deviceorientation_event_init.h"

namespace webf {

DeviceorientationEvent* DeviceorientationEvent::Create(ExecutingContext* context,
                                   const AtomicString& type,
                                   ExceptionState& exception_state) {
  return MakeGarbageCollected<DeviceorientationEvent>(context, type, exception_state);
}

DeviceorientationEvent* DeviceorientationEvent::Create(ExecutingContext* context,
                                   const AtomicString& type,
                                   const std::shared_ptr<DeviceorientationEventInit>& initializer,
                                   ExceptionState& exception_state) {
  return MakeGarbageCollected<DeviceorientationEvent>(context, type, initializer, exception_state);
}

DeviceorientationEvent::DeviceorientationEvent(ExecutingContext* context, const AtomicString& type, ExceptionState& exception_state)
    : Event(context, type) {}

DeviceorientationEvent::DeviceorientationEvent(ExecutingContext* context,
                           const AtomicString& type,
                           const std::shared_ptr<DeviceorientationEventInit>& initializer,
                           ExceptionState& exception_state)
    : Event(context, type),
      absolute_(initializer->hasAbsolute() ? initializer->absolute() : 0.0),
      alpha_(initializer->hasAlpha() ? initializer->alpha() : 0.0),
      beta_(initializer->hasBeta() ? initializer->beta() : 0.0),
      gamma_(initializer->hasGamma() ? initializer->gamma() : 0.0) {}

DeviceorientationEvent::DeviceorientationEvent(ExecutingContext* context,
                           const AtomicString& type,
                           NativeDeviceorientationEvent* native_orientation_event)
    : Event(context, type, &native_orientation_event->native_event),
      absolute_(native_orientation_event->absolute),
      alpha_(native_orientation_event->alpha),
      beta_(native_orientation_event->beta),
      gamma_(native_orientation_event->gamma) {
}

bool DeviceorientationEvent::IsDeviceorientationEvent() const {
  return true;
}

bool DeviceorientationEvent::absolute() const {
  return absolute_;
}

double DeviceorientationEvent::alpha() const {
  return alpha_;
}

double DeviceorientationEvent::beta() const {
  return beta_;
}

double DeviceorientationEvent::gamma() const {
    return gamma_;
}

}  // namespace webf
