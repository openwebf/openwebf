/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "plugin_api/document.h"
#include "plugin_api/exception_state.h"
#include "core/dom/document.h"
#include "core/dom/text.h"
#include "core/dom/document_fragment.h"
#include "core/dom/comment.h"
#include "core/html/html_html_element.h"

namespace webf {

DocumentWebFMethods::DocumentWebFMethods(ContainerNodeWebFMethods* super_method)
    : container_node(super_method) {}

WebFValue<Element, ElementWebFMethods> DocumentWebFMethods::CreateElement(
    webf::Document* ptr,
    const char* tag_name,
    webf::SharedExceptionState* shared_exception_state) {
  auto* document = static_cast<webf::Document*>(ptr);
  MemberMutationScope scope{document->GetExecutingContext()};
  webf::AtomicString tag_name_atomic = webf::AtomicString(document->ctx(), tag_name);
  Element* new_element = document->createElement(tag_name_atomic, shared_exception_state->exception_state);
  if (shared_exception_state->exception_state.HasException()) {
    return {.value = nullptr, .method_pointer = nullptr};
  }

  // Hold the reference until rust side notify this element was released.
  new_element->KeepAlive();
  return {.value = new_element, .method_pointer = To<ElementWebFMethods>(new_element->publicMethodPointer())};
}

WebFValue<Element, ElementWebFMethods> DocumentWebFMethods::CreateElementWithElementCreationOptions(
    webf::Document* ptr,
    const char* tag_name,
    WebFElementCreationOptions& options,
    webf::SharedExceptionState* shared_exception_state) {
  auto* document = static_cast<webf::Document*>(ptr);
  MemberMutationScope scope{document->GetExecutingContext()};
  webf::AtomicString tag_name_atomic = webf::AtomicString(document->ctx(), tag_name);

  std::string value = std::string(R"({"is":")") + options.is + "\"}";
  const char* value_cstr = value.c_str();
  webf::ScriptValue options_value = webf::ScriptValue::CreateJsonObject(document->ctx(), value_cstr, value.length());

  Element* new_element = document->createElement(
    tag_name_atomic,
    options_value,
    shared_exception_state->exception_state
  );
  if (shared_exception_state->exception_state.HasException()) {
    return {.value = nullptr, .method_pointer = nullptr};
  }

  // Hold the reference until rust side notify this element was released.
  new_element->KeepAlive();
  return {.value = new_element, .method_pointer = To<ElementWebFMethods>(new_element->publicMethodPointer())};
}

WebFValue<Element, ElementWebFMethods> DocumentWebFMethods::CreateElementNS(
    webf::Document* ptr,
    const char* uri,
    const char* tag_name,
    webf::SharedExceptionState* shared_exception_state) {
  auto* document = static_cast<webf::Document*>(ptr);
  MemberMutationScope scope{document->GetExecutingContext()};
  webf::AtomicString uri_atomic = webf::AtomicString(document->ctx(), uri);
  webf::AtomicString tag_name_atomic = webf::AtomicString(document->ctx(), tag_name);
  Element* new_element = document->createElementNS(uri_atomic, tag_name_atomic, shared_exception_state->exception_state);
  if (shared_exception_state->exception_state.HasException()) {
    return {.value = nullptr, .method_pointer = nullptr};
  }

  // Hold the reference until rust side notify this element was released.
  new_element->KeepAlive();
  return {.value = new_element, .method_pointer = To<ElementWebFMethods>(new_element->publicMethodPointer())};
}

WebFValue <Element, ElementWebFMethods> DocumentWebFMethods::CreateElementNSWithElementCreationOptions(
    webf::Document* ptr,
    const char* uri,
    const char* tag_name,
    WebFElementCreationOptions& options,
    webf::SharedExceptionState* shared_exception_state) {
  auto* document = static_cast<webf::Document*>(ptr);
  MemberMutationScope scope{document->GetExecutingContext()};
  webf::AtomicString uri_atomic = webf::AtomicString(document->ctx(), uri);
  webf::AtomicString tag_name_atomic = webf::AtomicString(document->ctx(), tag_name);

  std::string value = std::string(R"({"is":")") + options.is + "\"}";
  const char* value_cstr = value.c_str();
  webf::ScriptValue options_value = webf::ScriptValue::CreateJsonObject(document->ctx(), value_cstr, value.length());

  Element* new_element = document->createElementNS(
    uri_atomic,
    tag_name_atomic,
    options_value,
    shared_exception_state->exception_state
  );
  if (shared_exception_state->exception_state.HasException()) {
    return {.value = nullptr, .method_pointer = nullptr};
  }

  // Hold the reference until rust side notify this element was released.
  new_element->KeepAlive();
  return {.value = new_element, .method_pointer = To<ElementWebFMethods>(new_element->publicMethodPointer())};
}

WebFValue<Text, TextNodeWebFMethods> DocumentWebFMethods::CreateTextNode(
    webf::Document* ptr,
    const char* data,
    webf::SharedExceptionState* shared_exception_state) {
  auto* document = static_cast<webf::Document*>(ptr);
  MemberMutationScope scope{document->GetExecutingContext()};
  webf::AtomicString data_atomic = webf::AtomicString(document->ctx(), data);
  Text* text_node = document->createTextNode(data_atomic, shared_exception_state->exception_state);

  if (shared_exception_state->exception_state.HasException()) {
    return {.value = nullptr, .method_pointer = nullptr};
  }

  text_node->KeepAlive();

  return {.value = text_node, .method_pointer = To<TextNodeWebFMethods>(text_node->publicMethodPointer())};
}

WebFValue<DocumentFragment, DocumentFragmentWebFMethods> DocumentWebFMethods::CreateDocumentFragment(
    webf::Document* ptr,
    webf::SharedExceptionState* shared_exception_state) {
  auto* document = static_cast<webf::Document*>(ptr);
  MemberMutationScope scope{document->GetExecutingContext()};
  DocumentFragment* document_fragment = document->createDocumentFragment(shared_exception_state->exception_state);

  if (shared_exception_state->exception_state.HasException()) {
    return {.value = nullptr, .method_pointer = nullptr};
  }

  document_fragment->KeepAlive();

  return {.value = document_fragment,
          .method_pointer = To<DocumentFragmentWebFMethods>(document_fragment->publicMethodPointer())};
}

WebFValue<Comment, CommentWebFMethods> DocumentWebFMethods::CreateComment(
    webf::Document* ptr,
    const char* data,
    webf::SharedExceptionState* shared_exception_state) {
  auto* document = static_cast<webf::Document*>(ptr);
  MemberMutationScope scope{document->GetExecutingContext()};
  webf::AtomicString data_atomic = webf::AtomicString(document->ctx(), data);
  Comment* comment = document->createComment(data_atomic, shared_exception_state->exception_state);

  if (shared_exception_state->exception_state.HasException()) {
    return {.value = nullptr, .method_pointer = nullptr};
  }

  comment->KeepAlive();

  return {.value = comment, .method_pointer = To<CommentWebFMethods>(comment->publicMethodPointer())};
}

WebFValue<Element, ElementWebFMethods> DocumentWebFMethods::DocumentElement(webf::Document* document) {
  return {.value = document->documentElement(),
          .method_pointer = To<ElementWebFMethods>(document->documentElement()->publicMethodPointer())};
}

}  // namespace webf
