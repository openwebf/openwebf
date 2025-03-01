/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_CORE_DOM_DOM_STRING_MAP_H_
#define WEBF_CORE_DOM_DOM_STRING_MAP_H_

#include "bindings/qjs/atomic_string.h"
#include "bindings/qjs/cppgc/member.h"
#include "bindings/qjs/exception_state.h"
#include "bindings/qjs/script_wrappable.h"
#include "plugin_api/dom_string_map.h"

namespace webf {

class Element;

class DOMStringMap : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using ImplType = DOMStringMap*;
  DOMStringMap() = delete;
  explicit DOMStringMap(Element* owner_element);

  bool NamedPropertyQuery(const AtomicString& key, ExceptionState& exception_state);
  void NamedPropertyEnumerator(std::vector<AtomicString>& props, ExceptionState& exception_state);
  AtomicString item(const AtomicString& key, ExceptionState& exception_state);
  bool SetItem(const AtomicString& key, const AtomicString& value, ExceptionState& exception_state);
  bool DeleteItem(const AtomicString& key, ExceptionState& exception_state);

  void Trace(webf::GCVisitor* visitor) const override;
  const DOMStringMapPublicMethods* domStringMapPublicMethods();

 private:
  Member<Element> owner_element_;
};

}  // namespace webf

#endif  // WEBF_CORE_DOM_DOM_STRING_MAP_H_
