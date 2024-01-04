/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_CORE_DOM_NODE_DATA_H_
#define BRIDGE_CORE_DOM_NODE_DATA_H_

#include <cinttypes>
#include "bindings/qjs/cppgc/garbage_collected.h"
#include "bindings/qjs/cppgc/gc_visitor.h"
#include "mutation_observer_registration.h"

namespace webf {

class ChildNodeList;
class EmptyNodeList;
class ContainerNode;
class NodeList;
class Node;

class NodeMutationObserverData final {
 public:
  NodeMutationObserverData() = default;
  NodeMutationObserverData(const NodeMutationObserverData&) = delete;
  NodeMutationObserverData& operator=(const NodeMutationObserverData&) = delete;
  ~NodeMutationObserverData();

  const std::vector<Member<MutationObserverRegistration>>& Registry() { return registry_; }
  const std::set<Member<MutationObserverRegistration>>& TransientRegistry() { return transient_registry_; }

  void AddTransientRegistration(MutationObserverRegistration* registration);
  void RemoveTransientRegistration(MutationObserverRegistration* registration);
  void AddRegistration(MutationObserverRegistration* registration);
  void RemoveRegistration(MutationObserverRegistration* registration);

  void Trace(GCVisitor* visitor) const;

 private:
  std::vector<Member<MutationObserverRegistration>> registry_;
  std::set<Member<MutationObserverRegistration>> transient_registry_;
};

class NodeData {
 public:
  enum class ClassType : uint8_t {
    kNodeRareData,
    kElementRareData,
  };

  ChildNodeList* GetChildNodeList(ContainerNode& node);

  ChildNodeList* EnsureChildNodeList(ContainerNode& node);
  NodeList* NodeLists() { return node_list_; }

  NodeMutationObserverData* MutationObserverData() { return mutation_observer_data_.get(); }
  NodeMutationObserverData& EnsureMutationObserverData() {
    if (!mutation_observer_data_) {
      mutation_observer_data_ = std::make_shared<NodeMutationObserverData>();
    }
    return *mutation_observer_data_;
  }

  EmptyNodeList* EnsureEmptyChildNodeList(Node& node);

  void Trace(GCVisitor* visitor) const;

 private:
  Member<NodeList> node_list_;
  std::shared_ptr<NodeMutationObserverData> mutation_observer_data_;
};

}  // namespace webf

#endif  // BRIDGE_CORE_DOM_NODE_DATA_H_
