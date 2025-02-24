// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (C) 2024-present The WebF authors. All rights reserved.

#include "core/dom/intersection_observer.h"

#include <algorithm>
#include <limits>

#include <native_value_converter.h>
#include "bindings/qjs/converter_impl.h"
#include "core/dom/element.h"
#include "core/dom/intersection_observer_entry.h"
#include "core/dom/node.h"
#include "core/executing_context.h"
#include "foundation/logging.h"
#include "qjs_intersection_observer_init.h"

namespace webf {

IntersectionObserver* IntersectionObserver::Create(ExecutingContext* context,
                                                   const std::shared_ptr<QJSFunction>& function,
                                                   ExceptionState& exception_state) {
  return MakeGarbageCollected<IntersectionObserver>(context, function);
}

IntersectionObserver* IntersectionObserver::Create(ExecutingContext* context,
                                                   const std::shared_ptr<QJSFunction>& function,
                                                   const std::shared_ptr<IntersectionObserverInit>& observer_init,
                                                   ExceptionState& exception_state) {
  return MakeGarbageCollected<IntersectionObserver>(context, function, observer_init);
}

IntersectionObserver::IntersectionObserver(ExecutingContext* context, const std::shared_ptr<QJSFunction>& function)
    : BindingObject(context->ctx()), function_(function) {
  GetExecutingContext()->dartMethodPtr()->createBindingObject(
      GetExecutingContext()->isDedicated(), GetExecutingContext()->contextId(), bindingObject(),
      CreateBindingObjectType::kCreateIntersectionObserver, nullptr, 0);
}

IntersectionObserver::IntersectionObserver(ExecutingContext* context,
                                           const std::shared_ptr<QJSFunction>& function,
                                           const std::shared_ptr<IntersectionObserverInit>& observer_init)
    : BindingObject(context->ctx()), function_(function) {
  if (observer_init && observer_init->hasRoot()) {
    root_ = observer_init->root();
  }
  NativeValue arguments[1];
  if (observer_init && observer_init->hasThreshold()) {
#if ENABLE_LOG
    WEBF_LOG(DEBUG) << "[IntersectionObserver]: Constructor threshold.size = " << observer_init->threshold().size()
                    << std::endl;
#endif
    thresholds_ = std::move(observer_init->threshold());
    std::sort(thresholds_.begin(), thresholds_.end());
    arguments[0] = NativeValueConverter<NativeTypeArray<NativeTypeDouble>>::ToNativeValue(thresholds_);
  }
  GetExecutingContext()->dartMethodPtr()->createBindingObject(
      GetExecutingContext()->isDedicated(), GetExecutingContext()->contextId(), bindingObject(),
      CreateBindingObjectType::kCreateIntersectionObserver, arguments, 1);
}

bool IntersectionObserver::RootIsValid() const {
  return RootIsImplicit() || root();
}

void IntersectionObserver::observe(Element* target, ExceptionState& exception_state) {
  if (!RootIsValid() || !target) {
    WEBF_LOG(ERROR) << "[IntersectionObserver]: observe valid:" << std::endl;
    return;
  }

#if ENABLE_LOG
  WEBF_LOG(DEBUG) << "[IntersectionObserver]: observe target=" << target << "，tagName=" << target->nodeName()
                  << std::endl;
#endif
  GetExecutingContext()->uiCommandBuffer()->AddCommand(UICommand::kAddIntersectionObserver, nullptr, bindingObject(),
                                                       target->bindingObject());
}

void IntersectionObserver::unobserve(Element* target, ExceptionState& exception_state) {
  if (!target) {
    WEBF_LOG(ERROR) << "[IntersectionObserver]: unobserve valid:" << std::endl;
    return;
  }

  GetExecutingContext()->uiCommandBuffer()->AddCommand(UICommand::kRemoveIntersectionObserver, nullptr, bindingObject(),
                                                       target->bindingObject());
}

void IntersectionObserver::disconnect(ExceptionState& exception_state) {
  GetExecutingContext()->uiCommandBuffer()->AddCommand(UICommand::kDisconnectIntersectionObserver, nullptr,
                                                       bindingObject(), nullptr);
}

NativeValue IntersectionObserver::HandleCallFromDartSide(const AtomicString& method,
                                                         int32_t argc,
                                                         const NativeValue* argv,
                                                         Dart_Handle dart_object) {
  if (!GetExecutingContext() || !GetExecutingContext()->IsContextValid()) {
    WEBF_LOG(ERROR) << "[IntersectionObserver]: HandleCallFromDartSide Context Valid" << std::endl;
    return Native_NewNull();
  }

  MemberMutationScope scope{GetExecutingContext()};

  NativeIntersectionObserverEntry* native_entry =
      NativeValueConverter<NativeTypePointer<NativeIntersectionObserverEntry>>::FromNativeValue(argv[0]);
  size_t length = NativeValueConverter<NativeTypeInt64>::FromNativeValue(argv[1]);

  if (length > 0) {
    assert(function_ != nullptr);
    JSValue js_array = JS_NewArray(ctx());
    for (int i = 0; i < length; i++) {
      auto* entry = MakeGarbageCollected<IntersectionObserverEntry>(
          GetExecutingContext(), native_entry[i].is_intersecting, native_entry[i].intersectionRatio,
          DynamicTo<Element>(BindingObject::From(native_entry[i].target)));
      JS_SetPropertyUint32(ctx(), js_array, i, entry->ToQuickJS());
    }
    ScriptValue arguments[] = {ScriptValue(ctx(), js_array), ToValue()};

#if ENABLE_LOG
    WEBF_LOG(DEBUG) << "[IntersectionObserver]: HandleCallFromDartSide length=" << length << "，JS function_ Invoke"
                    << std::endl;
#endif

    function_->Invoke(ctx(), ToValue(), 2, arguments);

    JS_FreeValue(ctx(), js_array);
  } else {
    WEBF_LOG(ERROR) << "[IntersectionObserver]: HandleCallFromDartSide entries is empty";
  }

  return Native_NewNull();
}

void IntersectionObserver::Trace(GCVisitor* visitor) const {
  BindingObject::Trace(visitor);

  function_->Trace(visitor);
}

}  // namespace webf
