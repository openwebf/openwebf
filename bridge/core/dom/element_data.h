/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_CORE_DOM_ELEMENT_DATA_H_
#define WEBF_CORE_DOM_ELEMENT_DATA_H_

#if WEBF_V8_JS_ENGINE
#include "bindings/v8/atomic_string.h"
#elif WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/atomic_string.h"
#include "bindings/qjs/cppgc/gc_visitor.h"
#include "bindings/qjs/cppgc/member.h"
#endif
#include "dom_string_map.h"
#include "dom_token_list.h"

namespace webf {

class ElementData {
 public:
  void CopyWith(ElementData* other);
  void Trace(GCVisitor* visitor) const;

  DOMTokenList* GetClassList() const;
  void SetClassList(DOMTokenList* dom_token_lists);

  DOMStringMap* DataSet() const;
  void SetDataSet(DOMStringMap* data_set);

  bool style_attribute_is_dirty() const { return style_attribute_is_dirty_; }
  void SetStyleAttributeIsDirty(bool value) const { style_attribute_is_dirty_ = value; }

 private:
  Member<DOMTokenList> class_lists_;
  Member<DOMStringMap> data_set_;
  AtomicString class_;
  mutable bool style_attribute_is_dirty_;
};

}  // namespace webf

#endif  // WEBF_CORE_DOM_ELEMENT_DATA_H_
