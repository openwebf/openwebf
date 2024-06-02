/*
 * Copyright (C) 2024-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_CORE_EVENTS_DEVICE_ORIENTATION_EVENT_H_
#define BRIDGE_CORE_EVENTS_DEVICE_ORIENTATION_EVENT_H_

#include "bindings/qjs/dictionary_base.h"
#include "bindings/qjs/source_location.h"
#include "core/dom/events/event.h"
#include "qjs_deviceorientation_event_init.h"

namespace webf {

struct NativeDeviceorientationEvent;
class DeviceorientationEventInit;

class DeviceorientationEvent : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using ImplType = DeviceorientationEvent*;

  static DeviceorientationEvent* Create(ExecutingContext* context, const AtomicString& type, ExceptionState& exception_state);

  static DeviceorientationEvent* Create(ExecutingContext* context,
                              const AtomicString& type,
                              const std::shared_ptr<DeviceorientationEventInit>& initializer,
                              ExceptionState& exception_state);

  explicit DeviceorientationEvent(ExecutingContext* context, const AtomicString& type, ExceptionState& exception_state);

  explicit DeviceorientationEvent(ExecutingContext* context,
                        const AtomicString& type,
                        const std::shared_ptr<DeviceorientationEventInit>& initializer,
                        ExceptionState& exception_state);

  explicit DeviceorientationEvent(ExecutingContext* context, const AtomicString& type, NativeDeviceorientationEvent* native_orientation_event);

  bool absolute() const;
  double alpha() const;
  double beta() const;
  double gamma() const;

  bool IsDeviceorientationEvent() const override;

 private:
  bool absolute_;
  double alpha_;
  double beta_;
  double gamma_;
};

}  // namespace webf

#endif  // BRIDGE_CORE_EVENTS_DEVICE_ORIENTATION_EVENT_H_
