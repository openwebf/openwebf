/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_FOUNDATION_UI_COMMAND_BUFFER_H_
#define BRIDGE_FOUNDATION_UI_COMMAND_BUFFER_H_

#include <cinttypes>
#include "foundation/native_string.h"

namespace webf {

class ExecutingContext;

enum UICommandKind : uint32_t {
  kNodeCreation = 1,
  kNodeMutation = 1 << 2,
  kStyleUpdate = 1 << 3,
  kEvent = 1 << 4,
  kAttributeUpdate = 1 << 5,
  kDisposeBindingObject = 1 << 6,
  kOperation = 1 << 7,
  kUknownCommand = 1 << 8
};

enum class UICommand {
  kStartRecordingCommand,
  kCreateElement,
  kCreateTextNode,
  kCreateComment,
  kCreateDocument,
  kCreateWindow,
  kDisposeBindingObject,
  kAddEvent,
  kRemoveNode,
  kInsertAdjacentNode,
  kSetStyle,
  kClearStyle,
  kSetAttribute,
  kRemoveAttribute,
  kCloneNode,
  kRemoveEvent,
  kCreateDocumentFragment,
  kCreateSVGElement,
  kCreateElementNS,
  kFinishRecordingCommand,
};

#define MAXIMUM_UI_COMMAND_SIZE 2048

struct UICommandItem {
  UICommandItem() = default;
  explicit UICommandItem(int32_t type, SharedNativeString* args_01, void* nativePtr, void* nativePtr2)
      : type(type),
        string_01(reinterpret_cast<int64_t>(args_01 != nullptr ? args_01->string() : nullptr)),
        args_01_length(args_01 != nullptr ? args_01->length() : 0),
        nativePtr(reinterpret_cast<int64_t>(nativePtr)),
        nativePtr2(reinterpret_cast<int64_t>(nativePtr2)){};
  int32_t type{0};
  int32_t args_01_length{0};
  int64_t string_01{0};
  int64_t nativePtr{0};
  int64_t nativePtr2{0};
};

UICommandKind GetKindFromUICommand(UICommand type);

class UICommandBuffer {
 public:
  UICommandBuffer() = delete;
  explicit UICommandBuffer(ExecutingContext* context);
  ~UICommandBuffer();
  void addCommand(UICommand type,
                  std::unique_ptr<SharedNativeString>&& args_01,
                  void* nativePtr,
                  void* nativePtr2,
                  bool request_ui_update = true);
  UICommandItem* data();
  uint32_t kindFlag();
  int64_t size();
  bool empty();
  void clear();

 private:
  void addCommand(const UICommandItem& item, bool request_ui_update = true);
  void addCommands(const UICommandItem* items, int64_t item_size, bool request_ui_update = true);
  void updateFlags(UICommand command);

  ExecutingContext* context_{nullptr};
  UICommandItem* buffer_{nullptr};
  uint32_t kind_flag{0};
  bool update_batched_{false};
  int64_t size_{0};
  int64_t max_size_{MAXIMUM_UI_COMMAND_SIZE};
  friend class SharedUICommand;
};

}  // namespace webf

#endif  // BRIDGE_FOUNDATION_UI_COMMAND_BUFFER_H_
