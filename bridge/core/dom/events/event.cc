/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include "event.h"
#include <quickjs/quickjs.h>
#include "bindings/qjs/cppgc/gc_visitor.h"
#include "core/executing_context.h"
#include "event_target.h"
#include "foundation/native_value_converter.h"

namespace webf {

Event::Event(ExecutingContext* context, const AtomicString& event_type)
    : Event(context,
            event_type,
            Bubbles::kNo,
            Cancelable::kNo,
            ComposedMode::kComposed,
            std::chrono::system_clock::now().time_since_epoch().count()) {}

Event::Event(ExecutingContext* context, const AtomicString& type, const std::shared_ptr<EventInit>& init)
    : Event(context,
            type,
            init->bubbles() ? Bubbles::kYes : Bubbles::kNo,
            init->cancelable() ? Cancelable::kYes : Cancelable::kNo,
            init->composed() ? ComposedMode::kComposed : ComposedMode::kScoped,
            std::chrono::system_clock::now().time_since_epoch().count()) {}

Event::Event(ExecutingContext* context,
             const AtomicString& event_type,
             Bubbles bubbles,
             Cancelable cancelable,
             ComposedMode composed_mode,
             double time_stamp)
    : ScriptWrappable(context->ctx()),
      type_(event_type),
      bubbles_(bubbles == Bubbles::kYes),
      cancelable_(cancelable == Cancelable::kYes),
      composed_(composed_mode == ComposedMode::kComposed),
      propagation_stopped_(false),
      immediate_propagation_stopped_(false),
      default_prevented_(false),
      default_handled_(false),
      was_initialized_(true),
      is_trusted_(false),
      handling_passive_(PassiveMode::kNotPassiveDefault),
      prevent_default_called_on_uncancelable_event_(false),
      fire_only_capture_listeners_at_target_(false),
      fire_only_non_capture_listeners_at_target_(false),
      event_phase_(0),
      current_target_(nullptr),
      time_stamp_(time_stamp) {}

Event::Event(ExecutingContext* context, const AtomicString& event_type, NativeEvent* native_event)
    : ScriptWrappable(context->ctx()),
      raw_event_(native_event),
      type_(event_type),
      bubbles_(native_event->bubbles),
      composed_(native_event->composed),
      cancelable_(native_event->cancelable),
      time_stamp_(native_event->timeStamp),
      default_prevented_(native_event->defaultPrevented),
      propagation_stopped_(false),
      immediate_propagation_stopped_(false),
      default_handled_(false),
      was_initialized_(true),
      is_trusted_(false),
      handling_passive_(PassiveMode::kNotPassiveDefault),
      prevent_default_called_on_uncancelable_event_(false),
      fire_only_capture_listeners_at_target_(false),
      fire_only_non_capture_listeners_at_target_(false),
      event_phase_(0),
#if ANDROID_32_BIT
      target_(
          DynamicTo<EventTarget>(BindingObject::From(reinterpret_cast<NativeBindingObject*>(native_event->target)))),
      current_target_(DynamicTo<EventTarget>(
          BindingObject::From(reinterpret_cast<NativeBindingObject*>(native_event->currentTarget)))) {
}
#else
      target_(DynamicTo<EventTarget>(BindingObject::From(native_event->target))),
      current_target_(DynamicTo<EventTarget>(BindingObject::From(native_event->currentTarget))) {
}
#endif

void Event::SetType(const AtomicString& type) {
  type_ = type;
}

EventTarget* Event::target() const {
  return target_;
}

void Event::SetTarget(EventTarget* target) {
  target_ = target;
}

EventTarget* Event::currentTarget() const {
  return current_target_;
}

EventTarget* Event::srcElement() const {
  return target();
}

void Event::SetCurrentTarget(EventTarget* target) {
  current_target_ = target;
}

void Event::NamedPropertyEnumerator(std::vector<AtomicString>& names, webf::ExceptionState&) {
  if (raw_event_ == nullptr)
    return;

#if ANDROID_32_BIT
  auto* raw_event_props = reinterpret_cast<EventProp*>(raw_event_->props);
#else
  auto* raw_event_props = raw_event_->props;
#endif
  for (int i = 0; i < raw_event_->props_len; i++) {
    AtomicString key = AtomicString(ctx(), raw_event_props[i].key_atom);
    names.emplace_back(key);
  }
}

bool Event::NamedPropertyQuery(const AtomicString& key, ExceptionState& exception_state) {
  if (raw_event_ == nullptr)
    return false;

#if ANDROID_32_BIT
  auto* raw_event_props = reinterpret_cast<EventProp*>(raw_event_->props);
#else
  auto* raw_event_props = raw_event_->props;
#endif
  for (int i = 0; i < raw_event_->props_len; i++) {
    if (key.Impl() == raw_event_props[i].key_atom) {
      return true;
    }
  }
  return false;
}

ScriptValue Event::item(const AtomicString& key, ExceptionState& exception_state) {
  if (raw_event_ == nullptr)
    return ScriptValue::Undefined(ctx());

#if ANDROID_32_BIT
  auto* raw_event_props = reinterpret_cast<EventProp*>(raw_event_->props);
#else
  auto* raw_event_props = raw_event_->props;
#endif

  if (raw_event_props != nullptr) {
    for (int i = 0; i < raw_event_->props_len; i++) {
      if (key.Impl() == raw_event_props[i].key_atom) {
        return ScriptValue(ctx(), raw_event_props[i].value, true);
      }
    }
  }

  return ScriptValue::Undefined(ctx());
}

void set_event_prop(JSContext* ctx,
                    EventProp* prop,
                    Event* event,
                    const AtomicString& key,
                    const ScriptValue& value,
                    ExceptionState& exception_state) {
  event->customized_event_props_.emplace_back(value);
  prop->key_atom = key.Impl();
  prop->value = value.ToNative(ctx, exception_state, true);
}

#define DEFAULT_EVENT_PROP_LEN 2

bool Event::SetItem(const AtomicString& key, const ScriptValue& value, webf::ExceptionState& exception_state) {
  if (raw_event_ == nullptr)
    return false;

  if (raw_event_->props == 0x00) {
    auto* event_props = (EventProp*)js_malloc_rt(runtime(), sizeof(EventProp) * DEFAULT_EVENT_PROP_LEN);
    set_event_prop(ctx(), event_props, this, key, value, exception_state);
#if ANDROID_32_BIT
    raw_event_->props = reinterpret_cast<int64_t>(event_props);
#else
    raw_event_->props = event_props;
#endif

    raw_event_->props_len = 1;
    raw_event_->alloc_size = DEFAULT_EVENT_PROP_LEN;
  } else {
    int64_t prop_index = raw_event_->props_len;
#if ANDROID_32_BIT
    auto* raw_event_props = reinterpret_cast<EventProp*>(raw_event_->props);
#else
    auto* raw_event_props = raw_event_->props;
#endif

    for (int i = 0; i < raw_event_->props_len; i++) {
      if (raw_event_props[i].key_atom == key.Impl()) {
        prop_index = i;
        break;
      }
    }

    bool need_expand = prop_index + 1 > raw_event_->alloc_size;
    if (need_expand) {
      size_t new_size = std::max(raw_event_->alloc_size * 9 / 2, raw_event_->props_len + 1);
      raw_event_props = (EventProp*)js_realloc(ctx(), raw_event_props, sizeof(EventProp) * new_size);
#if ANDROID_32_BIT
      raw_event_->props = (int64_t)raw_event_props;
#else
      raw_event_->props = raw_event_props;
#endif
      raw_event_->alloc_size = new_size;
    }
    set_event_prop(ctx(), &(raw_event_props[prop_index]), this, key, value, exception_state);
    if (prop_index == raw_event_->props_len) {
      raw_event_->props_len = raw_event_->props_len + 1;
    }
  }

  return true;
}

bool Event::DeleteItem(const webf::AtomicString& key, webf::ExceptionState& exception_state) {
  if (raw_event_ == nullptr)
    return false;

#if ANDROID_32_BIT
  auto* raw_event_props = reinterpret_cast<EventProp*>(raw_event_->props);
#else
  auto* raw_event_props = raw_event_->props;
#endif
  for (int i = 0; i < raw_event_->props_len; i++) {
    if (raw_event_props[i].key_atom == key.Impl()) {
      for (int j = i + 1; j < raw_event_->props_len; j++) {
        memcpy(&raw_event_props[j - 1], &raw_event_props[j], sizeof(EventProp));
      }
      memset(&raw_event_props[raw_event_->props_len - 1], 0x00, sizeof(EventProp));
      raw_event_->props_len--;
      return true;
    }
  }

  return false;
}

bool Event::IsUiEvent() const {
  return false;
}

bool Event::IsMouseEvent() const {
  return false;
}

bool Event::IsFocusEvent() const {
  return false;
}

bool Event::IsKeyboardEvent() const {
  return false;
}

bool Event::IsTouchEvent() const {
  return false;
}

bool Event::IsGestureEvent() const {
  return false;
}

bool Event::IsPointerEvent() const {
  return false;
}

bool Event::IsInputEvent() const {
  return false;
}

bool Event::IsCloseEvent() const {
  return false;
}

bool Event::IsCustomEvent() const {
  return false;
}

bool Event::IsTransitionEvent() const {
  return false;
}

bool Event::IsAnimationEvent() const {
  return false;
}

bool Event::IsMessageEvent() const {
  return false;
}

bool Event::IsPopstateEvent() const {
  return false;
}

bool Event::IsHashChangeEvent() const {
  return false;
}

bool Event::IsDeviceorientationEvent() const {
  return false;
}

bool Event::IsIntersectionchangeEvent() const {
  return false;
}

bool Event::IsDragEvent() const {
  return false;
}

bool Event::IsBeforeUnloadEvent() const {
  return false;
}

bool Event::IsErrorEvent() const {
  return false;
}

bool Event::IsPromiseRejectionEvent() const {
  return false;
}

void Event::preventDefault(ExceptionState& exception_state) {
  if (handling_passive_ != PassiveMode::kNotPassive && handling_passive_ != PassiveMode::kNotPassiveDefault) {
    return;
  }

  default_prevented_ = true;
}

void Event::initEvent(const AtomicString& event_type, bool bubbles, bool cancelable, ExceptionState& exception_state) {
  if (IsBeingDispatched()) {
    return;
  }

  was_initialized_ = true;
  propagation_stopped_ = false;
  immediate_propagation_stopped_ = false;
  default_prevented_ = false;

  type_ = event_type;
  bubbles_ = bubbles;
  cancelable_ = cancelable;
}

void Event::SetHandlingPassive(PassiveMode mode) {
  handling_passive_ = mode;
}

void Event::Trace(GCVisitor* visitor) const {
  visitor->TraceMember(target_);
  visitor->TraceMember(current_target_);
  for (auto& prop : customized_event_props_) {
    prop.Trace(visitor);
  }
}

}  // namespace webf
