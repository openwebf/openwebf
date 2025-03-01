/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include "inline_css_style_declaration.h"
#include <vector>
#include "core/dom/element.h"
#include "core/dom/mutation_observer_interest_group.h"
#include "core/executing_context.h"
#include "core/html/parser/html_parser.h"
#include "css_property_list.h"
#include "element_namespace_uris.h"
#include "html_names.h"

namespace webf {

static std::string parseJavaScriptCSSPropertyName(std::string& propertyName) {
  static std::unordered_map<std::string, std::string> propertyCache{};

  if (propertyCache.count(propertyName) > 0) {
    return propertyCache[propertyName];
  }

  std::vector<char> buffer(propertyName.size() + 1);

  if (propertyName.size() > 2 && propertyName[0] == '-' && propertyName[1] == '-') {
    propertyCache[propertyName] = propertyName;
    return propertyName;
  }

  size_t hyphen = 0;
  bool toCamelCase = false;
  for (size_t i = 0; i < propertyName.size(); ++i) {
    char c = propertyName[i];
    if (!c)
      break;
    if (c == '-' && (i > 0 && propertyName[i - 1] != '-')) {
      toCamelCase = true;
      hyphen++;
      continue;
    }
    if (toCamelCase) {
      buffer[i - hyphen] = ToASCIIUpper(c);
      toCamelCase = false;
    } else {
      buffer[i - hyphen] = c;
    }
  }

  buffer.emplace_back('\0');

  std::string result = std::string(buffer.data());

  propertyCache[propertyName] = result;
  return result;
}

static std::string convertCamelCaseToKebabCase(const std::string& propertyName) {
  static std::unordered_map<std::string, std::string> propertyCache{};

  if (propertyCache.count(propertyName) > 0) {
    return propertyCache[propertyName];
  }

  std::string result;
  for (char c : propertyName) {
    if (std::isupper(c)) {
      result += '-';
      result += std::tolower(c);
    } else {
      result += c;
    }
  }

  propertyCache[propertyName] = result;
  return result;
}

InlineCssStyleDeclaration* InlineCssStyleDeclaration::Create(ExecutingContext* context,
                                                             ExceptionState& exception_state) {
  exception_state.ThrowException(context->ctx(), ErrorType::TypeError, "Illegal constructor.");
  return nullptr;
}

InlineCssStyleDeclaration::InlineCssStyleDeclaration(ExecutingContext* context, Element* owner_element_)
    : CSSStyleDeclaration(context->ctx()), owner_element_(owner_element_) {}

ScriptValue InlineCssStyleDeclaration::item(const AtomicString& key, ExceptionState& exception_state) {
  if (IsPrototypeMethods(key)) {
    return ScriptValue::Undefined(ctx());
  }

  std::string property_name = key.ToStdString(ctx());
  AtomicString property_value = InternalGetPropertyValue(property_name);
  return ScriptValue(ctx(), property_value);
}

bool InlineCssStyleDeclaration::SetItem(const AtomicString& key,
                                        const ScriptValue& value,
                                        ExceptionState& exception_state) {
  if (IsPrototypeMethods(key)) {
    return false;
  }

  std::string propertyName = key.ToStdString(ctx());
  bool success = InternalSetProperty(propertyName, value.ToLegacyDOMString(ctx()));
  if (success)
    InlineStyleChanged();
  return success;
}

bool InlineCssStyleDeclaration::DeleteItem(const webf::AtomicString& key, webf::ExceptionState& exception_state) {
  return true;
}

int64_t InlineCssStyleDeclaration::length() const {
  return properties_.size();
}

void InlineCssStyleDeclaration::Clear() {
  InternalClearProperty();
}

AtomicString InlineCssStyleDeclaration::getPropertyValue(const AtomicString& key, ExceptionState& exception_state) {
  std::string propertyName = key.ToStdString(ctx());
  return InternalGetPropertyValue(propertyName);
}

void InlineCssStyleDeclaration::setProperty(const AtomicString& key,
                                            const ScriptValue& value,
                                            ExceptionState& exception_state) {
  std::string propertyName = key.ToStdString(ctx());
  bool success = InternalSetProperty(propertyName, value.ToLegacyDOMString(ctx()));
  if (success)
    InlineStyleChanged();
}

AtomicString InlineCssStyleDeclaration::removeProperty(const AtomicString& key, ExceptionState& exception_state) {
  std::string propertyName = key.ToStdString(ctx());
  return InternalRemoveProperty(propertyName);
}

void InlineCssStyleDeclaration::CopyWith(InlineCssStyleDeclaration* inline_style) {
  for (auto& attr : inline_style->properties_) {
    properties_[attr.first] = attr.second;
  }
}

AtomicString InlineCssStyleDeclaration::cssText() const {
  std::string result;
  size_t index = 0;
  for (auto& attr : properties_) {
    result += convertCamelCaseToKebabCase(attr.first) + ": " + attr.second.ToStdString(ctx()) + ";";
    index++;
    if (index < properties_.size()) {
      result += " ";
    }
  }
  return AtomicString(ctx(), result);
}

void InlineCssStyleDeclaration::setCssText(const webf::AtomicString& value, webf::ExceptionState& exception_state) {
  SetCSSTextInternal(value);
  InlineStyleChanged();
}

void InlineCssStyleDeclaration::SetCSSTextInternal(const AtomicString& value) {
  const std::string css_text = value.ToStdString(ctx());
  InternalClearProperty();

  std::vector<std::string> styles;
  std::string::size_type prev_pos = 0, pos = 0;

  while ((pos = css_text.find(';', pos)) != std::string::npos) {
    styles.push_back(css_text.substr(prev_pos, pos - prev_pos));
    prev_pos = ++pos;
  }
  styles.push_back(css_text.substr(prev_pos, pos - prev_pos));

  for (auto& s : styles) {
    std::string::size_type position = s.find(':');
    if (position != std::basic_string<char>::npos) {
      std::string css_key = s.substr(0, position);
      css_key = trim(css_key);
      std::string css_value = s.substr(position + 1, s.length());
      css_value = trim(css_value);
      InternalSetProperty(css_key, AtomicString(ctx(), css_value));
    }
  }
}

void InlineCssStyleDeclaration::Trace(GCVisitor* visitor) const {
  visitor->TraceMember(owner_element_);
}

std::string InlineCssStyleDeclaration::ToString() const {
  if (properties_.empty())
    return "";

  std::string s;

  for (auto& attr : properties_) {
    s += attr.first + ": " + attr.second.ToStdString(ctx()) + ";";
  }

  s += "\"";
  return s;
}

void InlineCssStyleDeclaration::InlineStyleChanged() {
  assert(owner_element_->IsStyledElement());

  owner_element_->InvalidateStyleAttribute();

  if (std::shared_ptr<MutationObserverInterestGroup> recipients =
          MutationObserverInterestGroup::CreateForAttributesMutation(*owner_element_, html_names::kStyleAttr)) {
    AtomicString old_value = AtomicString::Null();
    if (owner_element_->attributes()->hasAttribute(html_names::kStyleAttr, ASSERT_NO_EXCEPTION())) {
      old_value = owner_element_->attributes()->getAttribute(html_names::kStyleAttr, ASSERT_NO_EXCEPTION());
    }

    recipients->EnqueueMutationRecord(
        MutationRecord::CreateAttributes(owner_element_, html_names::kStyleAttr, AtomicString::Null(), old_value));
    owner_element_->SynchronizeStyleAttributeInternal();
  }
}

bool InlineCssStyleDeclaration::NamedPropertyQuery(const AtomicString& key, ExceptionState&) {
  return cssPropertyList.count(key.ToStdString(ctx())) > 0;
}

void InlineCssStyleDeclaration::NamedPropertyEnumerator(std::vector<AtomicString>& names, ExceptionState&) {
  for (auto& entry : cssPropertyList) {
    names.emplace_back(AtomicString(ctx(), entry.first));
  }
}

AtomicString InlineCssStyleDeclaration::InternalGetPropertyValue(std::string& name) {
  name = parseJavaScriptCSSPropertyName(name);

  if (LIKELY(properties_.count(name) > 0)) {
    return properties_[name];
  }

  return AtomicString::Null();
}

bool InlineCssStyleDeclaration::InternalSetProperty(std::string& name, const AtomicString& value) {
  name = parseJavaScriptCSSPropertyName(name);
  if (properties_[name] == value) {
    return false;
  }

  AtomicString old_value = properties_[name];

  properties_[name] = value;

  std::unique_ptr<SharedNativeString> args_01 = stringToNativeString(name);
  GetExecutingContext()->uiCommandBuffer()->AddCommand(
      UICommand::kSetStyle, std::move(args_01), owner_element_->bindingObject(), value.ToNativeString(ctx()).release());

  return true;
}

AtomicString InlineCssStyleDeclaration::InternalRemoveProperty(std::string& name) {
  name = parseJavaScriptCSSPropertyName(name);

  if (UNLIKELY(properties_.count(name) == 0)) {
    return AtomicString::Empty();
  }

  AtomicString return_value = properties_[name];
  properties_.erase(name);

  InlineStyleChanged();

  std::unique_ptr<SharedNativeString> args_01 = stringToNativeString(name);
  GetExecutingContext()->uiCommandBuffer()->AddCommand(UICommand::kSetStyle, std::move(args_01),
                                                       owner_element_->bindingObject(), nullptr);

  return return_value;
}

void InlineCssStyleDeclaration::InternalClearProperty() {
  if (properties_.empty())
    return;
  properties_.clear();
  GetExecutingContext()->uiCommandBuffer()->AddCommand(UICommand::kClearStyle, nullptr, owner_element_->bindingObject(),
                                                       nullptr);
}

bool InlineCssStyleDeclaration::IsInlineCssStyleDeclaration() const {
  return true;
}

const InlineCssStyleDeclarationPublicMethods* InlineCssStyleDeclaration::inlineCssStyleDeclarationPublicMethods() {
  static InlineCssStyleDeclarationPublicMethods inline_css_style_declaration_public_methods;
  return &inline_css_style_declaration_public_methods;
}

}  // namespace webf
