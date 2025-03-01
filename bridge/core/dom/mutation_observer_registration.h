/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_MUTATION_OBSERVER_REGISTRATION_H
#define WEBF_MUTATION_OBSERVER_REGISTRATION_H

#include <set>
#include "bindings/qjs/script_wrappable.h"
#include "mutation_observer.h"
#include "mutation_observer_options.h"
#include "plugin_api/mutation_observer_registration.h"

namespace webf {

class MutationObserver;
class Node;

using NodeSet = std::unordered_set<Member<Node>, Member<Node>::KeyHasher>;

class MutationObserverRegistration : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  MutationObserverRegistration(MutationObserver&,
                               Node*,
                               MutationObserverOptions,
                               const std::unordered_set<AtomicString, AtomicString::KeyHasher>& attribute_filter);
  ~MutationObserverRegistration();

  void ResetObservation(MutationObserverOptions,
                        const std::unordered_set<AtomicString, AtomicString::KeyHasher>& attribute_filter);
  void ObservedSubtreeNodeWillDetach(Node&);
  void ClearTransientRegistrations();
  bool HasTransientRegistrations() const {
    return transient_registration_nodes_.get() != nullptr && !transient_registration_nodes_->empty();
  }
  void Unregister();

  bool ShouldReceiveMutationFrom(Node&, MutationType, const AtomicString* attribute_name) const;
  bool IsSubtree() const { return options_ & MutationObserver::kSubtree; }

  MutationObserver* Observer() const { return observer_; }
  MutationRecordDeliveryOptions DeliveryOptions() const {
    return options_ & (MutationObserver::kAttributeOldValue | MutationObserver::kCharacterDataOldValue);
  }
  MutationType MutationTypes() const { return static_cast<MutationType>(options_ & kMutationTypeAll); }

  void AddRegistrationNodesToSet(NodeSet&) const;

  void Dispose();

  void Trace(GCVisitor*) const override;
  const MutationObserverRegistrationPublicMethods* mutationObserverRegistrationPublicMethods();

 private:
  Member<MutationObserver> observer_;
  Member<Node> registration_node_;
  Member<Node> registration_node_keep_alive_;
  std::unique_ptr<NodeSet> transient_registration_nodes_;

  MutationObserverOptions options_;
  std::unordered_set<AtomicString, AtomicString::KeyHasher> attribute_filter_;
};

}  // namespace webf

#endif  // WEBF_MUTATION_OBSERVER_REGISTRATION_H
