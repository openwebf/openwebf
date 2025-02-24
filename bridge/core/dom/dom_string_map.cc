/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "dom_string_map.h"
#include "core/dom/element.h"
#include "foundation/string_view.h"

namespace webf {

bool startsWith(const char* str, const char* prefix) {
  size_t len_str = strlen(str);
  size_t len_prefix = strlen(prefix);
  if (len_str < len_prefix)
    return false;
  return strncmp(str, prefix, len_prefix) == 0;
}

static bool IsValidAttributeName(const AtomicString& name) {
  if (!name.Is8Bit())
    return false;

  if (!startsWith((const char*)name.Character8(), "data-"))
    return false;

  const int64_t length = name.length();
  for (unsigned i = 5; i < length; ++i) {
    if (IsASCIIUpper(name.Character8()[i]))
      return false;
  }

  return true;
}

static bool IsValidPropertyName(const AtomicString& name) {
  const int64_t length = name.length();
  for (unsigned i = 0; i < length; ++i) {
    if (name.Character8()[i] == '-' && (i + 1 < length) && IsASCIILower(name.Character8()[i + 1]))
      return false;
  }
  return true;
}

static bool PropertyNameMatchesAttributeName(const AtomicString& property_name,
                                             const AtomicString& attribute_name,
                                             unsigned property_length,
                                             unsigned attribute_length) {
  unsigned a = 5;
  unsigned p = 0;
  bool word_boundary = false;
  while (a < attribute_length && p < property_length) {
    if (attribute_name.Character8()[a] == '-' && a + 1 < attribute_length &&
        IsASCIILower(attribute_name.Character8()[a + 1])) {
      word_boundary = true;
    } else {
      if ((word_boundary ? ToASCIIUpper(attribute_name.Character8()[a])
                         : std::tolower(attribute_name.Character8()[a])) != (property_name.Character8()[p]))
        return false;
      p++;
      word_boundary = false;
    }
    a++;
  }

  return (a == attribute_length && p == property_length);
}

// This returns an AtomicString because attribute names are always stored
// as AtomicString types in Element (see setAttribute()).
static std::string ConvertPropertyNameToAttributeName(const std::string& name) {
  std::string result;
  result.reserve(name.size() + 5);  // For performance optimization, reserve enough space beforehand
  result.append("data-");

  unsigned length = name.length();
  for (unsigned i = 0; i < length; ++i) {
    char character = name[i];
    if (std::isupper(character)) {
      result.push_back('-');
      result.push_back(std::tolower(character));
    } else {
      result.push_back(character);
    }
  }

  return result;
}

std::string ConvertAttributeNameToPropertyName(const std::string& name) {
  std::string result;

  unsigned length = name.length();
  for (unsigned i = 5; i < length; ++i) {
    char character = name[i];
    if (character != '-') {
      result.push_back(character);
    } else {
      if ((i + 1 < length) && std::islower(name[i + 1])) {
        result.push_back(std::toupper(name[i + 1]));
        ++i;
      } else {
        result.push_back(character);
      }
    }
  }

  return result;
}

DOMStringMap::DOMStringMap(webf::Element* owner_element)
    : owner_element_(owner_element), ScriptWrappable(owner_element->ctx()) {}

void DOMStringMap::NamedPropertyEnumerator(std::vector<AtomicString>& props, webf::ExceptionState& exception_state) {
  auto attributes = owner_element_->attributes();
  for (auto& attribute : *attributes) {
    auto key = attribute.first;
    if (IsValidAttributeName(key)) {
      auto v = AtomicString(ctx(), ConvertAttributeNameToPropertyName(key.ToStdString(ctx())));
      props.emplace_back(v);
    }
  }
}

bool DOMStringMap::NamedPropertyQuery(const webf::AtomicString& key, webf::ExceptionState& exception_state) {
  for (auto& attribute : *owner_element_->attributes()) {
    if (PropertyNameMatchesAttributeName(key, attribute.first, key.length(), attribute.first.length())) {
      return true;
    }
  }
  return false;
}

AtomicString DOMStringMap::item(const webf::AtomicString& key, webf::ExceptionState& exception_state) {
  for (auto& attribute : *owner_element_->attributes()) {
    if (PropertyNameMatchesAttributeName(key, attribute.first, key.length(), attribute.first.length())) {
      return attribute.second;
    }
  }

  return AtomicString::Empty();
}

bool DOMStringMap::SetItem(const webf::AtomicString& key,
                           const webf::AtomicString& value,
                           webf::ExceptionState& exception_state) {
  if (!IsValidPropertyName(key)) {
    exception_state.ThrowException(ctx(), ErrorType::TypeError,
                                   "'" + key.ToStdString(ctx()) + "' is not a valid property name.");
    return false;
  }

  auto attribute_name = AtomicString(ctx(), ConvertPropertyNameToAttributeName(key.ToStdString(ctx())));
  owner_element_->setAttribute(attribute_name, value, exception_state);
  return true;
}

bool DOMStringMap::DeleteItem(const webf::AtomicString& key, webf::ExceptionState& exception_state) {
  if (IsValidPropertyName(key)) {
    auto attribute_name = AtomicString(ctx(), ConvertPropertyNameToAttributeName(key.ToStdString(ctx())));
    owner_element_->attributes()->removeAttribute(attribute_name, exception_state);
    return true;
  }
  return false;
}

void DOMStringMap::Trace(webf::GCVisitor* visitor) const {
  visitor->TraceMember(owner_element_);
}

}  // namespace webf