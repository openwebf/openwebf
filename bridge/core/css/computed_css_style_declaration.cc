/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "computed_css_style_declaration.h"
#include "binding_call_methods.h"
#include "core/binding_object.h"
#include "core/dom/element.h"
#include "foundation/native_value.h"
#include "foundation/native_value_converter.h"

namespace webf {

ComputedCssStyleDeclaration::ComputedCssStyleDeclaration(ExecutingContext* context,
                                                         NativeBindingObject* native_binding_object)
    : CSSStyleDeclaration(context->ctx(), native_binding_object) {}

ScriptValue ComputedCssStyleDeclaration::item(const AtomicString& key, ExceptionState& exception_state) {
  if (IsPrototypeMethods(key)) {
    return ScriptValue::Undefined(ctx());
  }

  NativeValue arguments[] = {NativeValueConverter<NativeTypeString>::ToNativeValue(ctx(), key)};

  NativeValue result = InvokeBindingMethod(
      binding_call_methods::kgetPropertyValue, 1, arguments,
      FlushUICommandReason::kDependentsOnElement | FlushUICommandReason::kDependentsOnLayout, exception_state);
  return ScriptValue(ctx(), NativeValueConverter<NativeTypeString>::FromNativeValue(ctx(), std::move(result)));
}

bool ComputedCssStyleDeclaration::SetItem(const AtomicString& key,
                                          const ScriptValue& value,
                                          ExceptionState& exception_state) {
  NativeValue arguments[] = {
      NativeValueConverter<NativeTypeString>::ToNativeValue(ctx(), key),
      NativeValueConverter<NativeTypeString>::ToNativeValue(ctx(), value.ToLegacyDOMString(ctx()))};
  InvokeBindingMethod(binding_call_methods::ksetProperty, 2, arguments,
                      FlushUICommandReason::kDependentsOnElement | FlushUICommandReason::kDependentsOnLayout,
                      exception_state);
  return true;
}

bool ComputedCssStyleDeclaration::DeleteItem(const webf::AtomicString& key, webf::ExceptionState& exception_state) {
  return true;
}

int64_t ComputedCssStyleDeclaration::length() const {
  NativeValue result = GetBindingProperty(
      binding_call_methods::klength,
      FlushUICommandReason::kDependentsOnElement | FlushUICommandReason::kDependentsOnLayout, ASSERT_NO_EXCEPTION());
  return NativeValueConverter<NativeTypeInt64>::FromNativeValue(result);
}

AtomicString ComputedCssStyleDeclaration::getPropertyValue(const AtomicString& key, ExceptionState& exception_state) {
  return item(key, exception_state).ToLegacyDOMString(ctx());
}

void ComputedCssStyleDeclaration::setProperty(const AtomicString& key,
                                              const ScriptValue& value,
                                              ExceptionState& exception_state) {
  SetItem(key, value, exception_state);
}

AtomicString ComputedCssStyleDeclaration::removeProperty(const AtomicString& key, ExceptionState& exception_state) {
  NativeValue arguments[] = {NativeValueConverter<NativeTypeString>::ToNativeValue(ctx(), key)};
  NativeValue result = InvokeBindingMethod(
      binding_call_methods::kremoveProperty, 1, arguments,
      FlushUICommandReason::kDependentsOnElement | FlushUICommandReason::kDependentsOnLayout, exception_state);
  return NativeValueConverter<NativeTypeString>::FromNativeValue(ctx(), std::move(result));
}

bool ComputedCssStyleDeclaration::NamedPropertyQuery(const AtomicString& key, ExceptionState& exception_state) {
  NativeValue arguments[] = {NativeValueConverter<NativeTypeString>::ToNativeValue(ctx(), key)};
  NativeValue result = InvokeBindingMethod(
      binding_call_methods::kcheckCSSProperty, 1, arguments,
      FlushUICommandReason::kDependentsOnElement | FlushUICommandReason::kDependentsOnLayout, exception_state);
  return NativeValueConverter<NativeTypeBool>::FromNativeValue(result);
}

void ComputedCssStyleDeclaration::NamedPropertyEnumerator(std::vector<AtomicString>& names,
                                                          ExceptionState& exception_state) {
  NativeValue result = InvokeBindingMethod(
      binding_call_methods::kgetFullCSSPropertyList, 0, nullptr,
      FlushUICommandReason::kDependentsOnElement | FlushUICommandReason::kDependentsOnLayout, exception_state);
  auto&& arr = NativeValueConverter<NativeTypeArray<NativeTypeString>>::FromNativeValue(ctx(), result);
  for (auto& i : arr) {
    names.emplace_back(i);
  }
}

bool ComputedCssStyleDeclaration::IsComputedCssStyleDeclaration() const {
  return true;
}

AtomicString ComputedCssStyleDeclaration::cssText() const {
  return AtomicString::Empty();
}

void ComputedCssStyleDeclaration::setCssText(const webf::AtomicString& value, webf::ExceptionState& exception_state) {}

const ComputedCssStyleDeclarationPublicMethods*
ComputedCssStyleDeclaration::computedCssStyleDeclarationPublicMethods() {
  static ComputedCssStyleDeclarationPublicMethods computed_css_style_declaration_public_methods;
  return &computed_css_style_declaration_public_methods;
}

}  // namespace webf
