/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_CORE_DOM_LEGACY_ELEMENT_ATTRIBUTES_H_
#define BRIDGE_CORE_DOM_LEGACY_ELEMENT_ATTRIBUTES_H_

#include <unordered_map>
#include "bindings/qjs/atomic_string.h"
#include "bindings/qjs/cppgc/member.h"
#include "bindings/qjs/script_wrappable.h"
#include "plugin_api/element_attributes.h"

namespace webf {

class ExceptionState;
class Element;

// TODO: refactor for better W3C standard support and higher performance.
class ElementAttributes : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using ImplType = ElementAttributes*;

  static ElementAttributes* Create(Element* element) { return MakeGarbageCollected<ElementAttributes>(element); }

  explicit ElementAttributes(Element* element);

  AtomicString getAttribute(const AtomicString& name, ExceptionState& exception_state);
  bool setAttribute(const AtomicString& name, const AtomicString& value, ExceptionState& exception_state);
  bool hasAttribute(const AtomicString& name, ExceptionState& exception_state);
  void removeAttribute(const AtomicString& name, ExceptionState& exception_state);
  void CopyWith(ElementAttributes* attributes);
  std::string ToString();

  bool IsEquivalent(const ElementAttributes& other) const;
  std::unordered_map<AtomicString, AtomicString>::iterator begin();
  std::unordered_map<AtomicString, AtomicString>::iterator end();

  void Trace(GCVisitor* visitor) const override;
  const ElementAttributesPublicMethods* elementAttributesPublicMethods();

 private:
  Member<Element> element_;
  std::unordered_map<AtomicString, AtomicString, AtomicString::KeyHasher> attributes_;
};

}  // namespace webf

#endif  // BRIDGE_CORE_DOM_LEGACY_ELEMENT_ATTRIBUTES_H_
