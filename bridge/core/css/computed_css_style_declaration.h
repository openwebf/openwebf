/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_CORE_CSS_COMPUTED_CSS_STYLE_DECLARATION_H_
#define WEBF_CORE_CSS_COMPUTED_CSS_STYLE_DECLARATION_H_

#include "bindings/qjs/cppgc/member.h"
#include "core/binding_object.h"
#include "css_style_declaration.h"
#include "plugin_api/computed_css_style_declaration.h"

namespace webf {

class Element;

class ComputedCssStyleDeclaration : public CSSStyleDeclaration {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using ImplType = ComputedCssStyleDeclaration;
  ComputedCssStyleDeclaration() = delete;

  explicit ComputedCssStyleDeclaration(ExecutingContext* context, NativeBindingObject* native_binding_object);

  ScriptValue item(const AtomicString& key, ExceptionState& exception_state) override;
  bool SetItem(const AtomicString& key, const ScriptValue& value, ExceptionState& exception_state) override;
  bool DeleteItem(const webf::AtomicString& key, webf::ExceptionState& exception_state) override;
  int64_t length() const override;

  AtomicString getPropertyValue(const AtomicString& key, ExceptionState& exception_state) override;
  void setProperty(const AtomicString& key, const ScriptValue& value, ExceptionState& exception_state) override;
  AtomicString removeProperty(const AtomicString& key, ExceptionState& exception_state) override;

  bool NamedPropertyQuery(const AtomicString&, ExceptionState&) override;
  void NamedPropertyEnumerator(std::vector<AtomicString>& names, ExceptionState&) override;

  bool IsComputedCssStyleDeclaration() const override;

  AtomicString cssText() const override;
  void setCssText(const AtomicString& value, ExceptionState& exception_state) override;

  const ComputedCssStyleDeclarationPublicMethods* computedCssStyleDeclarationPublicMethods();

 private:
};

template <>
struct DowncastTraits<ComputedCssStyleDeclaration> {
  static bool AllowFrom(const BindingObject& binding_object) { return binding_object.IsComputedCssStyleDeclaration(); }
};

}  // namespace webf

#endif  // WEBF_CORE_CSS_COMPUTED_CSS_STYLE_DECLARATION_H_
