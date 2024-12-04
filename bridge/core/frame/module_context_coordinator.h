/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_MODULE_CALLBACK_COORDINATOR_H
#define BRIDGE_MODULE_CALLBACK_COORDINATOR_H

#include <forward_list>
// Quickjs's linked-list are more efficient than STL forward_list.
#include "module_callback.h"
#include "module_manager.h"

namespace webf {

class ModuleListener;
class ModuleContext;

class ModuleContextCoordinator final {
 public:
  void AddModuleContext(std::shared_ptr<ModuleContext> module_context);

 private:
  std::forward_list<std::shared_ptr<ModuleContext>> module_contexts_;
  friend ModuleListener;
};

}  // namespace webf

#endif  // BRIDGE_MODULE_CALLBACK_COORDINATOR_H
