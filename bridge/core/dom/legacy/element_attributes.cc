/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "element_attributes.h"
#include "bindings/qjs/exception_state.h"
#include "core/dom/element.h"
#include "core/html/custom/widget_element.h"
#include "foundation/native_value_converter.h"
#include "html_names.h"

namespace webf {

static inline bool IsNumberIndex(const StringView& name) {
  if (name.Empty())
    return false;
  char f = name.Characters8()[0];
  return f >= '0' && f <= '9';
}

ElementAttributes::ElementAttributes(Element* element) : ScriptWrappable(element->ctx()), element_(element) {}

AtomicString ElementAttributes::getAttribute(const AtomicString& name, ExceptionState& exception_state) {
  bool numberIndex = IsNumberIndex(name.ToStringView());

  if (numberIndex) {
    return AtomicString::Null();
  }

  if (attributes_.count(name) == 0) {
    if (element_->IsWidgetElement()) {
      // Fallback to directly FFI access to dart.
      NativeValue dart_result =
          element_->GetBindingProperty(name, FlushUICommandReason::kDependentsOnElement, exception_state);
      if (dart_result.tag == NativeTag::TAG_STRING) {
        return NativeValueConverter<NativeTypeString>::FromNativeValue(element_->ctx(), std::move(dart_result));
      }
    }
    return AtomicString::Null();
  }

  AtomicString value = attributes_[name];
  return value;
}

bool ElementAttributes::setAttribute(const AtomicString& name,
                                     const AtomicString& value,
                                     ExceptionState& exception_state) {
  bool numberIndex = IsNumberIndex(name.ToStringView());

  if (numberIndex) {
    exception_state.ThrowException(ctx(), ErrorType::TypeError,
                                   "Failed to execute 'kSetAttribute' on 'Element': '" + name.ToStdString(ctx()) +
                                       "' is not a valid attribute name.");
    return false;
  }

  AtomicString existing_attribute = attributes_[name];

  attributes_[name] = value;

  // Style attribute will be parsed and separated into multiple setStyle command.
  if (name == html_names::kStyleAttr)
    return true;

  std::unique_ptr<SharedNativeString> args_01 = value.ToNativeString(ctx());
  std::unique_ptr<SharedNativeString> args_02 = name.ToNativeString(ctx());

  GetExecutingContext()->uiCommandBuffer()->AddCommand(UICommand::kSetAttribute, std::move(args_01),
                                                       element_->bindingObject(), args_02.release());

  return true;
}

bool ElementAttributes::hasAttribute(const AtomicString& name, ExceptionState& exception_state) {
  bool numberIndex = IsNumberIndex(name.ToStringView());

  if (numberIndex) {
    return false;
  }

  bool has_attribute = attributes_.count(name) > 0;

  if (!has_attribute && element_->IsWidgetElement()) {
    // Fallback to directly FFI access to dart.
    NativeValue dart_result =
        element_->GetBindingProperty(name, FlushUICommandReason::kDependentsOnElement, exception_state);
    return dart_result.tag != NativeTag::TAG_NULL;
  }

  return has_attribute;
}

void ElementAttributes::removeAttribute(const AtomicString& name, ExceptionState& exception_state) {
  if (!hasAttribute(name, exception_state))
    return;

  AtomicString old_value = getAttribute(name, exception_state);
  element_->WillModifyAttribute(name, old_value, AtomicString::Null());

  attributes_.erase(name);

  std::unique_ptr<SharedNativeString> args_01 = name.ToNativeString(ctx());
  GetExecutingContext()->uiCommandBuffer()->AddCommand(UICommand::kRemoveAttribute, std::move(args_01),
                                                       element_->bindingObject(), nullptr);
}

void ElementAttributes::CopyWith(ElementAttributes* attributes) {
  for (auto& attr : attributes->attributes_) {
    attributes_[attr.first] = attr.second;
  }
}

std::string ElementAttributes::ToString() {
  std::string s;

  for (auto& attr : attributes_) {
    s += attr.first.ToStdString(ctx()) + "=";
    s += "\"" + attr.second.ToStdString(ctx()) + "\"";
  }

  return s;
}

bool ElementAttributes::IsEquivalent(const ElementAttributes& other) const {
  if (attributes_.size() != other.attributes_.size())
    return false;
  for (auto& entry : attributes_) {
    auto it = other.attributes_.find(entry.first);
    if (it == other.attributes_.end()) {
      return false;
    }
  }
  return true;
}

std::unordered_map<AtomicString, AtomicString>::iterator ElementAttributes::begin() {
  return attributes_.begin();
}

std::unordered_map<AtomicString, AtomicString>::iterator ElementAttributes::end() {
  return attributes_.end();
}

void ElementAttributes::Trace(GCVisitor* visitor) const {
  visitor->TraceMember(element_);
}

const ElementAttributesPublicMethods* ElementAttributes::elementAttributesPublicMethods() {
  static ElementAttributesPublicMethods element_attributes_public_methods;
  return &element_attributes_public_methods;
}

}  // namespace webf
