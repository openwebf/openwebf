/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "core/html/html_image_element.h"
#include "core/html/canvas/html_canvas_element.h"
#include "core/html/canvas/canvas_gradient.h"
#include "<%= generateUnionTypeFileName(unionType) %>.h"
#include "bindings/qjs/converter_impl.h"
#include "core/fileapi/blob.h"
#include "core/html/html_image_element.h"
#include "core/html/canvas/html_canvas_element.h"

namespace webf {

std::shared_ptr<<%= generateUnionTypeClassName(unionType) %>> <%= generateUnionTypeClassName(unionType) %>::Create(JSContext* ctx, JSValue value, ExceptionState& exception_state) {
  <% _.forEach(unionType, (type) => { %>
    <% if (!isTypeHaveNull(type)) { %>
    if (<%= generateTypeRawChecker(type) %>) {
      auto&& v = Converter<<%= generateIDLTypeConverter(type) %>>::FromValue(ctx, value, exception_state);
      if (UNLIKELY(exception_state.HasException())) {
        return nullptr;
      }
      return std::make_shared<<%= generateUnionTypeClassName(unionType) %>>(v);
    }
    <% } %>
  <% }) %>
<% if(isTypeHaveString(unionType)) { %>
  auto&& v = Converter<IDLDOMString>::FromValue(ctx, value, exception_state);
  if (UNLIKELY(exception_state.HasException())) {
    return nullptr;
  }
  return std::make_shared<<%= generateUnionTypeClassName(unionType) %>>(v);
<% } else { %>
  return nullptr;
<% } %>
}

<% _.forEach(unionType, (type) => { %>
<%= generateUnionConstructorImpl(generateUnionTypeClassName(unionType), type) %>
<%= generateUnionTypeSetter(generateUnionTypeClassName(unionType), type) %>
<% }) %>

<%= generateUnionTypeClassName(unionType) %> ::~<%= generateUnionTypeClassName(unionType) %> () {

}

JSValue <%= generateUnionTypeClassName(unionType) %>::ToQuickJSValue(JSContext* ctx, ExceptionState& exception_state) const {
  switch(content_type_) {
    <% _.forEach(unionType, (type) => { %>
    <% if (!isTypeHaveNull(type)) { %>
   case ContentType::k<%= getUnionTypeName(type) %>: {
    return Converter<<%= generateIDLTypeConverter(type) %>>::ToValue(ctx, member_<%= generateUnionMemberName(type) %>_);
   }
    <% } %>
    <% }) %>
  }
  return JS_NULL;
}

void <%= generateUnionTypeClassName(unionType) %>::Trace(GCVisitor* visitor) const {
<% _.forEach(unionType, (type) => { %>
<% if (!isTypeHaveNull(type)) { %>
  TraceIfNeeded<<%= generateIDLTypeConverter(type) %>>::Trace(visitor, member_<%= generateUnionMemberName(type) %>_);
<% } %>
<% }) %>
}

void <%= generateUnionTypeClassName(unionType) %>::Clear() {
<% _.forEach(unionType, (type) => { %>
<% if (!isTypeHaveNull(type)) { %>
  member_<%= generateUnionMemberName(type) %>_<%= generateUnionTypeClear(type) %>;
<% } %>
<% }) %>
}

}