/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_WRAPPER_TYPE_INFO_H
#define BRIDGE_WRAPPER_TYPE_INFO_H

#include <quickjs/quickjs.h>
#include <cassert>
#include "bindings/qjs/qjs_engine_patch.h"

namespace webf {

class EventTarget;
class TouchList;

// Define all built-in wrapper class id.
enum {
  JS_CLASS_GC_TRACKER = JS_CLASS_INIT_COUNT + 1,
  JS_CLASS_SYNC_ITERATOR,
  JS_CLASS_BLOB,
  JS_CLASS_EVENT,
  JS_CLASS_ERROR_EVENT,
  JS_CLASS_MESSAGE_EVENT,
  JS_CLASS_UI_EVENT,
  JS_CLASS_CLOSE_EVENT,
  JS_CLASS_TOUCH_EVENT,
  JS_CLASS_POINTER_EVENT,
  JS_CLASS_MOUSE_EVENT,
  JS_CLASS_CUSTOM_EVENT,
  JS_CLASS_TRANSITION_EVENT,
  JS_CLASS_INPUT_EVENT,
  JS_CLASS_HYBRID_ROUTER_CHANGE_EVENT,
  JS_CLASS_ANIMATION_EVENT,
  JS_CLASS_FOCUS_EVENT,
  JS_CLASS_GESTURE_EVENT,
  JS_CLASS_HASHCHANGE_EVENT,
  JS_CLASS_POP_STATE_EVENT,
  JS_CLASS_INTERSECTION_CHANGE_EVENT,
  JS_CLASS_KEYBOARD_EVENT,
  JS_CLASS_PROMISE_REJECTION_EVENT,
  JS_CLASS_EVENT_TARGET,
  JS_CLASS_TOUCH,
  JS_CLASS_TOUCH_LIST,
  JS_CLASS_WINDOW,
  JS_CLASS_NODE,
  JS_CLASS_ELEMENT,
  JS_CLASS_SCREEN,
  JS_CLASS_PERFORMANCE,
  JS_CLASS_PERFORMANCE_MARK,
  JS_CLASS_PERFORMANCE_ENTRY,
  JS_CLASS_PERFORMANCE_MEASURE,
  JS_CLASS_DOCUMENT,
  JS_CLASS_CHARACTER_DATA,
  JS_CLASS_TEXT,
  JS_CLASS_COMMENT,
  JS_CLASS_NODE_LIST,
  JS_CLASS_DOCUMENT_FRAGMENT,
  JS_CLASS_BOUNDING_CLIENT_RECT,
  JS_CLASS_ELEMENT_ATTRIBUTES,
  JS_CLASS_HTML_ALL_COLLECTION,
  JS_CLASS_HTML_COLLECTION,
  JS_CLASS_HTML_ELEMENT,
  JS_CLASS_WIDGET_ELEMENT,
  JS_CLASS_HTML_DIV_ELEMENT,
  JS_CLASS_HTML_BODY_ELEMENT,
  JS_CLASS_HTML_HEAD_ELEMENT,
  JS_CLASS_HTML_HTML_ELEMENT,
  JS_CLASS_HTML_IMAGE_ELEMENT,
  JS_CLASS_HTML_SCRIPT_ELEMENT,
  JS_CLASS_HTMLI_FRAME_ELEMENT,
  JS_CLASS_HTML_ANCHOR_ELEMENT,
  JS_CLASS_HTML_LINK_ELEMENT,
  JS_CLASS_HTML_CANVAS_ELEMENT,
  JS_CLASS_IMAGE,
  JS_CLASS_MUTATION_OBSERVER,
  JS_CLASS_MUTATION_RECORD,
  JS_CLASS_MUTATION_OBSERVER_REGISTRATION,
  JS_CLASS_CANVAS_RENDERING_CONTEXT,
  JS_CLASS_CANVAS_RENDERING_CONTEXT_2_D,
  JS_CLASS_CANVAS_GRADIENT,
  JS_CLASS_CANVAS_PATTERN,
  JS_CLASS_DOM_MATRIX,
  JS_CLASS_DOM_MATRIX_READONLY,
  JS_CLASS_HTML_TEMPLATE_ELEMENT,
  JS_CLASS_HTML_UNKNOWN_ELEMENT,
  JS_CLASS_HTML_INPUT_ELEMENT,
  JS_CLASS_HTML_BUTTON_ELEMENT,
  JS_CLASS_HTML_FORM_ELEMENT,
  JS_CLASS_HTML_TEXTAREA_ELEMENT,
  JS_CLASS_CSS_STYLE_DECLARATION,
  JS_CLASS_INLINE_CSS_STYLE_DECLARATION,
  JS_CLASS_COMPUTED_CSS_STYLE_DECLARATION,

  JS_CLASS_DOM_TOKEN_LIST,
  JS_CLASS_DOM_STRING_MAP,

  // SVG
  JS_CLASS_SVG_ELEMENT,
  JS_CLASS_SVG_GRAPHICS_ELEMENT,
  JS_CLASS_SVG_GEOMETRY_ELEMENT,
  JS_CLASS_SVG_TEXT_CONTENT_ELEMENT,
  JS_CLASS_SVG_TEXT_POSITIONING_ELEMENT,

  JS_CLASS_SVG_RECT_ELEMENT,
  JS_CLASS_SVG_SVG_ELEMENT,
  JS_CLASS_SVG_PATH_ELEMENT,
  JS_CLASS_SVG_TEXT_ELEMENT,
  JS_CLASS_SVG_G_ELEMENT,
  JS_CLASS_SVG_CIRCLE_ELEMENT,
  JS_CLASS_SVG_ELLIPSE_ELEMENT,
  JS_CLASS_SVG_STYLE_ELEMENT,
  JS_CLASS_SVG_LINE_ELEMENT,

  // SVG unit
  JS_CLASS_SVG_LENGTH,
  JS_CLASS_SVG_ANIMATED_LENGTH,

  //
  JS_CLASS_FORM_DATA,

  JS_CLASS_CUSTOM_CLASS_INIT_COUNT /* last entry for predefined classes */

  
};

// Callback when get property using index.
// exp: obj[0]
using IndexedPropertyGetterHandler = JSValue (*)(JSContext* ctx, JSValue obj, uint32_t index);

// Callback when get property using string or symbol.
// exp: obj['hello']
using StringPropertyGetterHandler = JSValue (*)(JSContext* ctx, JSValue obj, JSAtom atom);

// Callback when set property using index.
// exp: obj[0] = value;
using IndexedPropertySetterHandler = bool (*)(JSContext* ctx, JSValueConst obj, uint32_t index, JSValueConst value);

// Callback when set property using string or symbol.
// exp: obj['hello'] = value;
using StringPropertySetterHandler = bool (*)(JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst value);

// Callback when delete property using string or symbol.
// exp: delete obj['hello']
using StringPropertyDeleteHandler = bool (*)(JSContext* ctx, JSValueConst obj, JSAtom prop);

// Callback when check property exist on object.
// exp: 'hello' in obj;
using PropertyCheckerHandler = bool (*)(JSContext* ctx, JSValueConst obj, JSAtom atom);

// Callback when enums all property on object.
// exp: Object.keys(obj);
using PropertyEnumerateHandler = int (*)(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj);

// This struct provides a way to store a bunch of information that is helpful
// when creating quickjs objects. Each quickjs bindings class has exactly one static
// WrapperTypeInfo member, so comparing pointers is a safe way to determine if
// types match.
class WrapperTypeInfo final {
 public:
  bool equals(const WrapperTypeInfo* that) const { return this == that; }

  bool isSubclass(const WrapperTypeInfo* that) const {
    for (const WrapperTypeInfo* current = this; current; current = current->parent_class) {
      if (current == that)
        return true;
    }
    return false;
  }

  JSClassID classId{0};
  const char* className{nullptr};
  const WrapperTypeInfo* parent_class{nullptr};
  JSClassCall* callFunc{nullptr};
  IndexedPropertyGetterHandler indexed_property_getter_handler_{nullptr};
  IndexedPropertySetterHandler indexed_property_setter_handler_{nullptr};
  StringPropertyGetterHandler string_property_getter_handler_{nullptr};
  StringPropertySetterHandler string_property_setter_handler_{nullptr};
  PropertyCheckerHandler property_checker_handler_{nullptr};
  PropertyEnumerateHandler property_enumerate_handler_{nullptr};
  StringPropertyDeleteHandler property_delete_handler_{nullptr};
};

}  // namespace webf

#endif  // BRIDGE_WRAPPER_TYPE_INFO_H
