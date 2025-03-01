/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_CORE_CSS_CSS_STYLE_DECLARATION_H_
#define WEBF_CORE_CSS_CSS_STYLE_DECLARATION_H_

#include "bindings/qjs/script_wrappable.h"
#include "core/binding_object.h"
#include "defined_properties.h"
#include "plugin_api/css_style_declaration.h"

namespace webf {

static bool IsPrototypeMethods(const AtomicString& key) {
  return key == defined_properties::kgetPropertyValue || key == defined_properties::kremoveProperty ||
         key == defined_properties::ksetProperty || key == defined_properties::kcssText ||
         key == defined_properties::klength;
}

class CSSStyleDeclaration : public BindingObject {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using ImplType = CSSStyleDeclaration*;
  explicit CSSStyleDeclaration(JSContext* ctx);
  explicit CSSStyleDeclaration(JSContext* ctx, NativeBindingObject* native_binding_object);

  virtual ScriptValue item(const AtomicString& key, ExceptionState& exception_state) = 0;
  virtual bool SetItem(const AtomicString& key, const ScriptValue& value, ExceptionState& exception_state) = 0;
  virtual bool DeleteItem(const AtomicString& key, ExceptionState& exception_state) = 0;
  virtual int64_t length() const = 0;
  virtual AtomicString cssText() const = 0;
  virtual void setCssText(const AtomicString& value, ExceptionState& exception_state) = 0;

  virtual AtomicString getPropertyValue(const AtomicString& key, ExceptionState& exception_state) = 0;
  virtual void setProperty(const AtomicString& key, const ScriptValue& value, ExceptionState& exception_state) = 0;
  virtual AtomicString removeProperty(const AtomicString& key, ExceptionState& exception_state) = 0;

  virtual bool NamedPropertyQuery(const AtomicString&, ExceptionState&) = 0;
  virtual void NamedPropertyEnumerator(std::vector<AtomicString>& names, ExceptionState&) = 0;

  //  virtual AtomicString cssText() const = 0;
  //  virtual void setCssText(const AtomicString& value, ExceptionState& exception_state) = 0;

  virtual bool IsComputedCssStyleDeclaration() const override;
  virtual bool IsInlineCssStyleDeclaration() const;

  const CSSStyleDeclarationPublicMethods* cssStyleDeclarationPublicMethods();

 private:
};

}  // namespace webf

#endif  // WEBF_CORE_CSS_CSS_STYLE_DECLARATION_H_
