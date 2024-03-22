/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'package:flutter/foundation.dart';
import 'package:webf/dom.dart';
import 'package:webf/foundation.dart';
import 'package:webf/src/dom/node_traversal.dart';

typedef InsertNodeHandler = void Function(ContainerNode container, Node child, Node? next);

enum DynamicRestyleFlag {
  ChildrenAffectedByFirstChildRules,
  ChildrenAffectedByLastChildRules,
  ChildrenAffectedByDirectAdjacentRules,
  ChildrenAffectedByForwardPositionalRules,
  ChildrenAffectedByBackwardPositionalRules,
}

extension StructuralRules on DynamicRestyleFlag {
  bool childrenAffectedByStructuralRules() {
    if (this == DynamicRestyleFlag.ChildrenAffectedByFirstChildRules ||
        this == DynamicRestyleFlag.ChildrenAffectedByLastChildRules ||
        this == DynamicRestyleFlag.ChildrenAffectedByDirectAdjacentRules ||
        this == DynamicRestyleFlag.ChildrenAffectedByForwardPositionalRules ||
        this == DynamicRestyleFlag.ChildrenAffectedByBackwardPositionalRules) {
      return true;
    }
    return false;
  }
}

bool collectChildrenAndRemoveFromOldParent(Node node, List<Node> nodes) {
  if (node is DocumentFragment) {
    getChildNodes(node, nodes);
    node.removeChildren();
    return nodes.isNotEmpty;
  }
  nodes.add(node);
  ContainerNode? oldParent = node.parentNode;
  if (oldParent != null && node.isConnected) {
    oldParent.removeChild(node);
  }
  return nodes.isNotEmpty;
}

void getChildNodes(ContainerNode node, List<Node> nodes) {
  assert(nodes.isEmpty);
  for (Node? child = node.firstChild; child != null; child = child.nextSibling) {
    nodes.add(child);
  }
}

abstract class ContainerNode extends Node {
  ContainerNode(NodeType nodeType, [BindingContext? context]) : super(nodeType, context);

  List<DynamicRestyleFlag>? restyleFlags;

  void addFlag(DynamicRestyleFlag flag) {
    restyleFlags ??= [];
    if (restyleFlags?.contains(flag) == false) {
      restyleFlags?.add(flag);
    }
  }

  void _adoptAndAppendChild(ContainerNode container, Node child, Node? next) {
    child.parentOrShadowHostNode = this;
    if (lastChild != null) {
      child.previousSibling = lastChild;
      lastChild!.nextSibling = child;
    } else {
      firstChild = child;
    }
    lastChild = child;
  }

  void _adoptAndInsertBefore(ContainerNode container, Node child, Node? next) {
    assert(next!.parentNode == container);

    // Use insertBefore if you need to handle reparenting (and want DOM mutation
    // events).
    assert(child.parentNode == null);
    assert(child.nextSibling == null);
    assert(child.previousSibling == null);

    Node? prev = next!.previousSibling;
    assert(lastChild != prev);
    next.previousSibling = child;
    if (prev != null) {
      assert(firstChild != next);
      assert(prev.nextSibling == next);
      prev.nextSibling = child;
    } else {
      assert(firstChild == next);
      firstChild = child;
    }
    child.parentOrShadowHostNode = this;
    child.previousSibling = prev;
    child.nextSibling = next;
  }

  @mustCallSuper
  @override
  Node insertBefore(Node newChild, Node referenceNode) {
    // https://dom.spec.whatwg.org/#concept-node-pre-insert

    // insertBefore(node, null) is equivalent to appendChild(node)

    // 1. Ensure pre-insertion validity of node into parent before child.
    // 2. Let reference child be child.
    // 3. If reference child is node, set it to node’s next sibling.
    // Already done at C++ side.

    // 4. Adopt node into parent’s node document.
    List<Node> targets = [];
    if (!collectChildrenAndRemoveFromOldParent(newChild, targets)) {
      return newChild;
    }

    // 5. Insert node into parent before reference child.
    _insertNode(targets, referenceNode, _adoptAndInsertBefore);

    // 6. Mark this element to dirty elements.
    if (this is Element) {
      ownerDocument.markElementStyleDirty(this as Element);
    }

    // 7. Trigger connected callback
    if (newChild.isConnected) {
      newChild.connectedCallback();
    }

    // To insert a node into a parent before a child, run step 9 from the spec:
    // 8. Run the children changed steps for parent when inserting a node into a parent.
    // https://dom.spec.whatwg.org/#concept-node-insert
    didInsertNode(targets, referenceNode);

    return newChild;
  }

  @override
  Node? replaceChild(Node newChild, Node oldChild) {
    // https://dom.spec.whatwg.org/#concept-node-replace
    // Step 2 to 6 are already done at C++ side.

    bool isOldChildConnected = oldChild.isConnected;

    // 7. Let reference child be child’s next sibling.
    Node? next = oldChild.nextSibling;

    // 8. If reference child is node, set it to node’s next sibling.
    if (next == newChild) next = newChild.nextSibling;

    // 10. Adopt node into parent’s node document.
    // Though the following CollectChildrenAndRemoveFromOldParent() also calls
    // RemoveChild(), we'd like to call RemoveChild() here to make a separated
    // MutationRecord.
    ContainerNode? newChildParent = newChild.parentNode;
    if (newChildParent != null) {
      newChildParent.removeChild(newChild);
    }

    // 9. Let previousSibling be child’s previous sibling.
    // 11. Let removedNodes be the empty list.
    // 15. Queue a mutation record of "childList" for target parent with
    // addedNodes nodes, removedNodes removedNodes, nextSibling reference child,
    // and previousSibling previousSibling.

    // 12. If child’s parent is not null, run these substeps:
    //    1. Set removedNodes to a list solely containing child.
    //    2. Remove child from its parent with the suppress observers flag set.
    ContainerNode? oldChildParent = oldChild.parentNode;
    if (oldChildParent != null) {
      oldChildParent.removeChild(oldChild);
    }

    List<Node> targets = [];

    // 13. Let nodes be node’s children if node is a DocumentFragment node, and
    // a list containing solely node otherwise.
    if (!collectChildrenAndRemoveFromOldParent(newChild, targets)) return oldChild;
    // 10. Adopt node into parent’s node document.
    // 14. Insert node into parent before reference child with the suppress
    // observers flag set.
    if (next != null) {
      _insertNode(targets, next, _adoptAndInsertBefore);
    } else {
      _insertNode(targets, null, _adoptAndAppendChild);
    }

    if (isOldChildConnected) {
      oldChild.disconnectedCallback();
      newChild.connectedCallback();
    }

    didInsertNode(targets, next);

    return oldChild;
  }

  @override
  Node? removeChild(Node oldChild) {
    Node child = oldChild;

    // Not remove node type which is not present in RenderObject tree such as Comment
    // Only append node types which is visible in RenderObject tree
    // Only remove childNode when it has parent
    if (child.isRendererAttachedToSegmentTree) {
      child.unmountRenderObject();
    }

    if (this is Element) {
      ownerDocument.markElementStyleDirty(this as Element);
    }

    bool isOldChildConnected = child.isConnected;
    assert(child.parentNode == this);

    Node? prev = child.previousSibling;
    Node? next = child.nextSibling;
    removeBetween(prev, next, oldChild);
    notifyNodeRemoved(child);

    if (isOldChildConnected) {
      child.disconnectedCallback();
    }

    // 21. Run the children changed steps for parent.
    // https://dom.spec.whatwg.org/#concept-node-remove
    childrenChanged(ChildrenChange.forRemoval(child, prev, next, ChildrenChangeSource.API));

    return child;
  }

  void _insertNode(
      List<Node> targets, Node? next, InsertNodeHandler mutator) {
    for (final targetNode in targets) {
      assert(targetNode.parentNode == null);
      mutator(this, targetNode, next);
      notifyNodeInsertedInternal(targetNode);
    }
  }

  @mustCallSuper
  @override
  Node appendChild(Node newChild) {
    List<Node> targets = [];
    if (!collectChildrenAndRemoveFromOldParent(newChild, targets)) {
      return newChild;
    }

    _insertNode(targets, null, _adoptAndAppendChild);

    if (this is Element) {
      ownerDocument.markElementStyleDirty(this as Element);
    }

    if (newChild.isConnected) {
      newChild.connectedCallback();
    }

    didInsertNode(targets, null);

    return newChild;
  }

  void removeChildren() {
    if (firstChild == null) {
      return;
    }

    Node? child = firstChild;
    while (child != null) {
      removeBetween(null, child.nextSibling, child);
      notifyNodeRemoved(child);
    }

    ContainerNode thisNode = this;
    thisNode.ensureNodeData().ensureChildNodeList(thisNode).invalidateCache();
  }

  void notifyNodeRemoved(Node node) {
    for (Node node in NodeTraversal.inclusiveDescendantsOf(node)) {
      // As an optimization we skip notifying Text nodes and other leaf nodes
      // of removal when they're not in the Document tree and not in a shadow root
      // since the virtual call to removedFrom is not needed.
      if (node is! ContainerNode && !node.isInTreeScope()) {
        continue;
      }
      node.removedFrom(this);
    }
  }

  void notifyNodeInsertedInternal(Node root) {
    for (final node in NodeTraversal.inclusiveDescendantsOf(root)) {
      if (!isConnected && !node.isContainerNode()) {
        continue;
      }
      node.insertedInto(this);
    }
  }

  void removeBetween(Node? previousChild, Node? nextChild, Node oldChild) {
    assert(oldChild.parentNode == this);

    if (nextChild != null) {
      nextChild..previousSibling = previousChild;
    }
    if (previousChild != null) {
      previousChild.nextSibling = nextChild;
    }
    if (firstChild == oldChild) {
      firstChild = nextChild;
    }
    if (lastChild == oldChild) {
      lastChild = previousChild;
    }

    oldChild.previousSibling = null;
    oldChild.nextSibling = null;
    oldChild.parentOrShadowHostNode = null;
  }

  void didInsertNode(List<Node> targets, Node? next) {
    Node? unchangedPrevious = targets.isNotEmpty ? targets[0].previousSibling : null;

    for (final targetNode in targets) {
      childrenChanged(ChildrenChange.forInsertion(targetNode, unchangedPrevious, next, ChildrenChangeSource.API));
    }
  }

  @override
  void childrenChanged(ChildrenChange change) {
    super.childrenChanged(change);
    invalidateNodeListCachesInAncestors(change);
  }

  void invalidateNodeListCachesInAncestors(ChildrenChange? change) {
    // This is a performance optimization, NodeList cache invalidation is
    // not necessary for a text change.
    if (change != null && change.type == ChildrenChangeType.TEXT_CHANGE) {
      return;
    }

    if (nodeData != null) {
      if (nodeData!.nodeList is ChildNodeList) {
        if (change != null) {
          (nodeData!.nodeList as ChildNodeList).childrenChanged(change);
        } else {
          (nodeData!.nodeList as ChildNodeList).invalidateCache();
        }
      }
      if (parentNode != null) {
        ownerDocument.nthIndexCache.invalidateWithParentNode(parentNode!);
      }
    }

    // This is a performance optimization, NodeList cache invalidation is
    // not necessary for non-element nodes.
    if (change != null &&
        change.affectsElements == ChildrenChangeAffectsElements.NO) {
      return;
    }

    ContainerNode? node = this;
    while (node != null) {
      NodeList lists = node.childNodes;
      if (lists is ChildNodeList) {
        lists.invalidateCache();
      }
      node = node.parentNode;
    }
    if (parentNode != null) {
      ownerDocument.nthIndexCache.invalidateWithParentNode(parentNode!);
    }
  }

  void checkForSiblingStyleChanges(Element parent, bool isRemoved, Node? nodeBeforeChange, Node? nodeAfterChange) {

    if (!isRendererAttached) {
      return;
    }

    final elementBeforeChange = nodeBeforeChange as Element?;
    final elementAfterChange = nodeAfterChange as Element?;

    // :first-child.  In the parser callback case, we don't have to check anything, since we were right the first time.
    // In the DOM case, we only need to do something if |afterChange| is not 0.
    // |afterChange| is 0 in the parser case, so it works out that we'll skip this block.
    if (elementAfterChange != null &&
        restyleFlags?.contains(DynamicRestyleFlag.ChildrenAffectedByFirstChildRules) == true) {
      // Find our new first child.
      final newFirstElement = parent.firstChild as Element?;

      // This is the insert/append case.
      if (newFirstElement != elementAfterChange && elementAfterChange.isRendererAttached) {
        elementAfterChange.recalculateStyle();
      }

      if (newFirstElement != null && isRemoved && newFirstElement == elementAfterChange) {
        newFirstElement.recalculateStyle();
      }
    }

    if (elementBeforeChange != null &&
        restyleFlags?.contains(DynamicRestyleFlag.ChildrenAffectedByLastChildRules) == true) {
      // Find our new first child.
      final newLastElement = parent.lastChild as Element?;

      // This is the insert/append case.
      if (newLastElement != elementBeforeChange && elementBeforeChange.isRendererAttached) {
        elementBeforeChange.recalculateStyle();
      }

      if (newLastElement != null && isRemoved && newLastElement == elementBeforeChange) {
        newLastElement.recalculateStyle();
      }
    }

    // The + selector.  We need to invalidate the first element following the insertion point.  It is the only possible element
    // that could be affected by this DOM change.
    if (restyleFlags?.contains(DynamicRestyleFlag.ChildrenAffectedByDirectAdjacentRules) == true && elementAfterChange != null) {
       elementAfterChange.recalculateStyle();
    }

    // Forward positional selectors include the ~ selector, nth-child, nth-of-type, first-of-type and only-of-type.
    // Backward positional selectors include nth-last-child, nth-last-of-type, last-of-type and only-of-type.
    // We have to invalidate everything following the insertion point in the forward case, and everything before the insertion point in the
    // backward case.
    // |afterChange| is 0 in the parser callback case, so we won't do any work for the forward case if we don't have to.
    // For performance reasons we just mark the parent node as changed, since we don't want to make childrenChanged O(n^2) by crawling all our kids
    // here. recalcStyle will then force a walk of the children when it sees that this has happened.
    if (elementAfterChange != null &&
        restyleFlags?.contains(DynamicRestyleFlag.ChildrenAffectedByForwardPositionalRules) == true) {
      parent.recalculateStyle();
    } else if (elementBeforeChange != null &&
        restyleFlags?.contains(DynamicRestyleFlag.ChildrenAffectedByBackwardPositionalRules) == true) {
      parent.recalculateStyle();
    }
  }

  Node? _firstChild;

  @override
  Node? get firstChild => _firstChild;

  set firstChild(Node? value) {
    _firstChild = value;
  }

  Node? _lastChild;

  @override
  Node? get lastChild => _lastChild;

  set lastChild(Node? value) {
    _lastChild = value;
  }
}
