/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_CORE_INPUT_TOUCH_H_
#define BRIDGE_CORE_INPUT_TOUCH_H_

#include "bindings/qjs/cppgc/member.h"
#include "bindings/qjs/script_wrappable.h"
#include "core/dom/events/event_target.h"
#include "plugin_api/touch.h"
#include "qjs_touch_init.h"

namespace webf {

struct NativeTouch {
  int64_t identifier;
  NativeBindingObject* target;
  double clientX;
  double clientY;
  double screenX;
  double screenY;
  double pageX;
  double pageY;
  double radiusX;
  double radiusY;
  double rotationAngle;
  double force;
  double altitudeAngle;
  double azimuthAngle;
};

class Touch : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using ImplType = Touch*;
  static Touch* Create(ExecutingContext* context, ExceptionState& exception_state);
  static Touch* Create(ExecutingContext* context,
                       const std::shared_ptr<TouchInit>& initializer,
                       ExceptionState& exception_state);
  static Touch* Create(ExecutingContext* context, NativeTouch* native_touch);

  explicit Touch(ExecutingContext* context, ExceptionState& exception_state);
  explicit Touch(ExecutingContext* context,
                 const std::shared_ptr<TouchInit>& initializer,
                 ExceptionState& exception_state);
  explicit Touch(ExecutingContext* context, NativeTouch* native_touch);

  double altitudeAngle() const;
  double azimuthAngle() const;
  double clientX() const;
  double clientY() const;
  double force() const;
  double identifier() const;
  double pageX() const;
  double pageY() const;
  double radiusX() const;
  double radiusY() const;
  double rotationAngle() const;
  double screenX() const;
  double screenY() const;
  EventTarget* target() const;

  void Trace(GCVisitor* visitor) const override;
  const TouchPublicMethods* touchPublicMethods();

 private:
  double altitude_angle_;
  double azimuth_angle_;
  double clientX_;
  double clientY_;
  double force_;
  double identifier_;
  double pageX_;
  double pageY_;
  double radiusX_;
  double radiusY_;
  double rotationAngle_;
  double screenX_;
  double screenY_;
  Member<EventTarget> target_;
};

}  // namespace webf

#endif  // BRIDGE_CORE_INPUT_TOUCH_H_
