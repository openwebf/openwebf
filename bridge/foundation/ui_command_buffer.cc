/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "ui_command_buffer.h"
#include "core/dart_methods.h"
#include "core/executing_context.h"
#include "foundation/logging.h"
#include "include/webf_bridge.h"

namespace webf {

UICommandKind GetKindFromUICommand(UICommand command) {
  switch (command) {
    case UICommand::kCreateElement:
    case UICommand::kCreateTextNode:
    case UICommand::kCreateComment:
    case UICommand::kCreateDocument:
    case UICommand::kCreateWindow:
    case UICommand::kRemoveNode:
    case UICommand::kCreateDocumentFragment:
    case UICommand::kCreateSVGElement:
    case UICommand::kCreateElementNS:
    case UICommand::kCloneNode:
      return UICommandKind::kNodeCreation;
    case UICommand::kInsertAdjacentNode:
      return UICommandKind::kNodeMutation;
    case UICommand::kAddEvent:
    case UICommand::kRemoveEvent:
      return UICommandKind::kEvent;
    case UICommand::kSetStyle:
    case UICommand::kClearStyle:
      return UICommandKind::kStyleUpdate;
    case UICommand::kSetAttribute:
    case UICommand::kRemoveAttribute:
      return UICommandKind::kAttributeUpdate;
    case UICommand::kDisposeBindingObject:
      return UICommandKind::kDisposeBindingObject;
    case UICommand::kStartRecordingCommand:
    case UICommand::kFinishRecordingCommand:
      return UICommandKind::kOperation;
    case UICommand::kAddIntersectionObserver:
    case UICommand::kRemoveIntersectionObserver:
    case UICommand::kDisconnectIntersectionObserver:
      return UICommandKind::kIntersectionObserver;
    default:
      return UICommandKind::kUknownCommand;
  }
}

UICommandBuffer::UICommandBuffer(ExecutingContext* context)
    : context_(context), buffer_((UICommandItem*)malloc(sizeof(UICommandItem) * MAXIMUM_UI_COMMAND_SIZE)) {}

UICommandBuffer::~UICommandBuffer() {
  free(buffer_);
}

void UICommandBuffer::addCommand(UICommand command,
                                 std::unique_ptr<SharedNativeString>&& args_01,
                                 void* nativePtr,
                                 void* nativePtr2,
                                 bool request_ui_update) {
  if (command == UICommand::kFinishRecordingCommand) {
    return;
  }

  UICommandItem item{static_cast<int32_t>(command), args_01.get(), nativePtr, nativePtr2};
  updateFlags(command);
  addCommand(item, request_ui_update);
}

void UICommandBuffer::updateFlags(UICommand command) {
  UICommandKind type = GetKindFromUICommand(command);
  kind_flag = kind_flag | type;
}

void UICommandBuffer::addCommand(const UICommandItem& item, bool request_ui_update) {
  if (UNLIKELY(!context_->dartIsolateContext()->valid())) {
    return;
  }

  if (size_ >= max_size_) {
    buffer_ = (UICommandItem*)realloc(buffer_, sizeof(UICommandItem) * max_size_ * 2);
    max_size_ = max_size_ * 2;
  }

  buffer_[size_] = item;
  size_++;
}

void UICommandBuffer::addCommands(const webf::UICommandItem* items, int64_t item_size, bool request_ui_update) {
  if (UNLIKELY(!context_->dartIsolateContext()->valid())) {
    return;
  }

  int64_t target_size = size_ + item_size;
  if (target_size > max_size_) {
    buffer_ = (UICommandItem*)realloc(buffer_, sizeof(UICommandItem) * target_size * 2);
    max_size_ = target_size * 2;
  }

  std::memcpy(buffer_ + size_, items, sizeof(UICommandItem) * item_size);
  size_ = target_size;
}

UICommandItem* UICommandBuffer::data() {
  return buffer_;
}

uint32_t UICommandBuffer::kindFlag() {
  return kind_flag;
}

int64_t UICommandBuffer::size() {
  return size_;
}

bool UICommandBuffer::empty() {
  return size_ == 0;
}

void UICommandBuffer::clear() {
  if (buffer_ == nullptr)
    return;
  memset(buffer_, 0, sizeof(UICommandItem) * size_);
  size_ = 0;
  kind_flag = 0;
  update_batched_ = false;
}

}  // namespace webf
