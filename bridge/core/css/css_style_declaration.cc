/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "css_style_declaration.h"

namespace webf {

CSSStyleDeclaration::CSSStyleDeclaration(JSContext* ctx) : BindingObject(ctx) {}
CSSStyleDeclaration::CSSStyleDeclaration(JSContext* ctx, NativeBindingObject* native_binding_object)
    : BindingObject(ctx, native_binding_object) {}

bool CSSStyleDeclaration::IsComputedCssStyleDeclaration() const {
  return false;
}

bool CSSStyleDeclaration::IsInlineCssStyleDeclaration() const {
  return false;
}

const CSSStyleDeclarationPublicMethods* CSSStyleDeclaration::cssStyleDeclarationPublicMethods() {
  static CSSStyleDeclarationPublicMethods css_style_declaration_public_methods;
  return &css_style_declaration_public_methods;
}

}  // namespace webf
