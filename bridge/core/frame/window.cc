/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "window.h"
#include <modp_b64/modp_b64.h>
#include "binding_call_methods.h"
#include "bindings/qjs/cppgc/garbage_collected.h"
#include "core/css/computed_css_style_declaration.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/events/message_event.h"
#include "core/executing_context.h"
#include "event_type_names.h"
#include "foundation/native_value_converter.h"

namespace webf {

Window::Window(ExecutingContext* context) : EventTargetWithInlineData(context) {
  context->uiCommandBuffer()->addCommand(UICommand::kCreateWindow, nullptr, (void*)bindingObject(), nullptr);
}

// https://infra.spec.whatwg.org/#ascii-whitespace
// Matches the definition of IsHTMLSpace in html_parser_idioms.h.
template <typename CharType>
bool IsAsciiWhitespace(CharType character) {
  return character <= ' ' &&
         (character == ' ' || character == '\n' || character == '\t' || character == '\r' || character == '\f');
}

AtomicString Window::btoa(const AtomicString& source, ExceptionState& exception_state) {
  if (source.IsEmpty())
    return AtomicString::Empty();
  size_t encode_len = modp_b64_encode_data_len(source.length());
  std::vector<char> buffer;
  buffer.resize(encode_len);

  const size_t output_size = modp_b64_encode_data(reinterpret_cast<char*>(buffer.data()),
                                                  reinterpret_cast<const char*>(source.Character8()), source.length());
  assert(output_size == encode_len);

  return {ctx(), buffer.data(), buffer.size()};
}

// Invokes modp_b64 without stripping whitespace.
bool Base64DecodeRaw(const AtomicString& in, std::vector<uint8_t>& out, ModpDecodePolicy policy) {
  size_t decode_len = modp_b64_decode_len(in.length());
  out.resize(decode_len);

  const size_t output_size = modp_b64_decode(reinterpret_cast<char*>(out.data()),
                                             reinterpret_cast<const char*>(in.Character8()), in.length(), policy);
  if (output_size == MODP_B64_ERROR)
    return false;
  out.resize(output_size);
  return true;
}

bool Base64Decode(JSContext* ctx, AtomicString in, std::vector<uint8_t>& out, ModpDecodePolicy policy) {
  switch (policy) {
    case ModpDecodePolicy::kForgiving: {
      // https://infra.spec.whatwg.org/#forgiving-base64-decode
      // Step 1 is to remove all whitespace. However, checking for whitespace
      // slows down the "happy" path. Since any whitespace will fail normal
      // decoding from modp_b64_decode, just try again if we detect a failure.
      // This shouldn't be much slower for whitespace inputs.
      //
      // TODO(csharrison): Most callers use String inputs so ToString() should
      // be fast. Still, we should add a RemoveCharacters method to StringView
      // to avoid a double allocation for non-String-backed StringViews.
      return Base64DecodeRaw(in, out, policy) ||
             Base64DecodeRaw(in.RemoveCharacters(ctx, &IsAsciiWhitespace), out, policy);
    }
    case ModpDecodePolicy::kNoPaddingValidation: {
      return Base64DecodeRaw(in, out, policy);
    }
    case ModpDecodePolicy::kStrict:
      return false;
  }
}

AtomicString Window::atob(const AtomicString& source, ExceptionState& exception_state) {
  if (source.IsEmpty())
    return AtomicString::Empty();
  if (!source.ContainsOnlyLatin1OrEmpty()) {
    exception_state.ThrowException(ctx(), ErrorType::TypeError,
                                   "The string to be decoded contains \"\n"
                                   "        \"characters outside of the Latin1 range.");
    return AtomicString::Empty();
  }

  std::vector<uint8_t> buffer;
  if (!Base64Decode(ctx(), source, buffer, ModpDecodePolicy::kForgiving)) {
    exception_state.ThrowException(ctx(), ErrorType::TypeError, "The string to be decoded is not correctly encoded.");
    return AtomicString::Empty();
  }

  JSValue str = JS_NewRawUTF8String(ctx(), buffer.data(), buffer.size());
  AtomicString result = {ctx(), str};
  JS_FreeValue(ctx(), str);
  return result;
}

Window* Window::open(ExceptionState& exception_state) {
  return this;
}

Window* Window::open(const AtomicString& url, ExceptionState& exception_state) {
  const NativeValue args[] = {
      NativeValueConverter<NativeTypeString>::ToNativeValue(ctx(), url),
  };
  InvokeBindingMethod(binding_call_methods::kopen, 1, args, exception_state);
  return this;
}

Screen* Window::screen() {
  if (screen_ == nullptr) {
    NativeValue value = GetBindingProperty(binding_call_methods::kscreen, ASSERT_NO_EXCEPTION());
    screen_ = MakeGarbageCollected<Screen>(
        this, NativeValueConverter<NativeTypePointer<NativeBindingObject>>::FromNativeValue(value));
  }
  return screen_;
}

void Window::scroll(ExceptionState& exception_state) {
  return scroll(0, 0, exception_state);
}

void Window::scroll(double x, double y, ExceptionState& exception_state) {
  const NativeValue args[] = {
      NativeValueConverter<NativeTypeDouble>::ToNativeValue(x),
      NativeValueConverter<NativeTypeDouble>::ToNativeValue(y),
  };
  InvokeBindingMethod(binding_call_methods::kscroll, 2, args, exception_state);
}

void Window::scroll(const std::shared_ptr<ScrollToOptions>& options, ExceptionState& exception_state) {
  const NativeValue args[] = {
      NativeValueConverter<NativeTypeDouble>::ToNativeValue(options->hasLeft() ? options->left() : 0.0),
      NativeValueConverter<NativeTypeDouble>::ToNativeValue(options->hasTop() ? options->top() : 0.0),
  };
  InvokeBindingMethod(binding_call_methods::kscroll, 2, args, exception_state);
}

void Window::scrollBy(ExceptionState& exception_state) {
  return scrollBy(0, 0, exception_state);
}

void Window::scrollBy(double x, double y, ExceptionState& exception_state) {
  const NativeValue args[] = {
      NativeValueConverter<NativeTypeDouble>::ToNativeValue(x),
      NativeValueConverter<NativeTypeDouble>::ToNativeValue(y),
  };
  InvokeBindingMethod(binding_call_methods::kscrollBy, 2, args, exception_state);
}

void Window::scrollBy(const std::shared_ptr<ScrollToOptions>& options, ExceptionState& exception_state) {
  const NativeValue args[] = {
      NativeValueConverter<NativeTypeDouble>::ToNativeValue(options->hasLeft() ? options->left() : 0.0),
      NativeValueConverter<NativeTypeDouble>::ToNativeValue(options->hasTop() ? options->top() : 0.0),
  };
  InvokeBindingMethod(binding_call_methods::kscrollBy, 2, args, exception_state);
}

void Window::scrollTo(ExceptionState& exception_state) {
  return scroll(exception_state);
}

void Window::scrollTo(double x, double y, ExceptionState& exception_state) {
  return scroll(x, y, exception_state);
}

void Window::scrollTo(const std::shared_ptr<ScrollToOptions>& options, ExceptionState& exception_state) {
  return scroll(options, exception_state);
}

void Window::postMessage(const ScriptValue& message, ExceptionState& exception_state) {
  auto event_init = MessageEventInit::Create();
  event_init->setData(message);
  auto* message_event =
      MessageEvent::Create(GetExecutingContext(), event_type_names::kmessage, event_init, exception_state);
  dispatchEvent(message_event, exception_state);
}

void Window::postMessage(const ScriptValue& message,
                         const AtomicString& target_origin,
                         ExceptionState& exception_state) {
  auto event_init = MessageEventInit::Create();
  event_init->setData(message);
  event_init->setOrigin(target_origin);
  auto* message_event =
      MessageEvent::Create(GetExecutingContext(), event_type_names::kmessage, event_init, exception_state);
  dispatchEvent(message_event, exception_state);
}

ComputedCssStyleDeclaration* Window::getComputedStyle(Element* element, ExceptionState& exception_state) {
  NativeValue arguments[] = {NativeValueConverter<NativeTypePointer<Element>>::ToNativeValue(element)};
  NativeValue result = InvokeBindingMethod(binding_call_methods::kgetComputedStyle, 1, arguments, exception_state);
  return MakeGarbageCollected<ComputedCssStyleDeclaration>(
      GetExecutingContext(), NativeValueConverter<NativeTypePointer<NativeBindingObject>>::FromNativeValue(result));
}

ComputedCssStyleDeclaration* Window::getComputedStyle(Element* element,
                                                      const AtomicString& pseudo_elt,
                                                      ExceptionState& exception_state) {
  return getComputedStyle(element, exception_state);
}

double Window::requestAnimationFrame(const std::shared_ptr<QJSFunction>& callback, ExceptionState& exceptionState) {
  if (GetExecutingContext()->dartMethodPtr()->flushUICommand == nullptr) {
    exceptionState.ThrowException(ctx(), ErrorType::InternalError,
                                  "Failed to execute 'flushUICommand': dart method (flushUICommand) executed "
                                  "with unexpected error.");
    return 0;
  }

  GetExecutingContext()->FlushUICommand();
  auto frame_callback = FrameCallback::Create(GetExecutingContext(), callback);
  uint32_t request_id = GetExecutingContext()->document()->RequestAnimationFrame(frame_callback, exceptionState);
  // `-1` represents some error occurred.
  if (request_id == -1) {
    exceptionState.ThrowException(
        ctx(), ErrorType::InternalError,
        "Failed to execute 'requestAnimationFrame': dart method (requestAnimationFrame) executed "
        "with unexpected error.");
    return 0;
  }
  return request_id;
}

void Window::cancelAnimationFrame(double request_id, ExceptionState& exception_state) {
  GetExecutingContext()->document()->CancelAnimationFrame(static_cast<uint32_t>(request_id), exception_state);
}

bool Window::IsWindowOrWorkerGlobalScope() const {
  return true;
}

void Window::Trace(GCVisitor* visitor) const {
  visitor->TraceMember(screen_);
  EventTargetWithInlineData::Trace(visitor);
}

JSValue Window::ToQuickJS() const {
  return JS_GetGlobalObject(ctx());
}

}  // namespace webf
