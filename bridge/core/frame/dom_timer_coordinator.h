/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_BINDINGS_QJS_BOM_DOM_TIMER_COORDINATOR_H_
#define BRIDGE_BINDINGS_QJS_BOM_DOM_TIMER_COORDINATOR_H_

#include <memory>
#include <unordered_map>
#include <vector>

namespace webf {

class DOMTimer;
class ExecutingContext;

// Maintains a set of DOMTimers for a given page
// DOMTimerCoordinator assigns IDs to timers; these IDs are
// the ones returned to web authors from setTimeout or setInterval. It
// also tracks recursive creation or iterative scheduling of timers,
// which is used as a signal for throttling repetitive timers.
class DOMTimerCoordinator {
 public:
  // Creates and installs a new timer. Returns the assigned ID.
  void installNewTimer(ExecutingContext* context, int32_t timer_id, std::shared_ptr<DOMTimer> timer);

  // Then timer are going to be finished, remove them from active_timers_ list.
  void removeTimeoutById(int32_t timer_id);
  // Force stop and remove a timer, even if it's still executing.
  void forceStopTimeoutById(int32_t timer_id);

  std::shared_ptr<DOMTimer> getTimerById(int32_t timer_id);

 private:
  std::unordered_map<int, std::shared_ptr<DOMTimer>> active_timers_;
  std::unordered_map<int, std::shared_ptr<DOMTimer>> terminated_timers;
};

}  // namespace webf

#endif  // BRIDGE_BINDINGS_QJS_BOM_DOM_TIMER_COORDINATOR_H_
