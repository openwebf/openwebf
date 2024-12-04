/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_FOUNDATION_PROFILER_H_
#define WEBF_FOUNDATION_PROFILER_H_

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#if WEBF_V8_JS_ENGINE
//#include "bindings/v8/atomic_string.h"
#elif WEBF_QUICKJS_JS_ENGINE
#include "bindings/qjs/script_value.h"
#endif

#include "foundation/stop_watch.h"

namespace webf {

class WebFProfiler;
class ExecutingContext;
class ProfileOpItem;
class LinkProfileStep;

class ProfileStep {
 public:
  explicit ProfileStep(ProfileOpItem* owner, std::string label);

  virtual ScriptValue ToJSON(JSContext* ctx, const std::string& path, bool should_link);
  void AddChildSteps(std::shared_ptr<ProfileStep> step);
  int64_t id();

 protected:
  ProfileOpItem* owner_;

 private:
  std::vector<std::shared_ptr<ProfileStep>> child_steps_;
  Stopwatch stopwatch_;
  std::string label_;
  int64_t id_;
  friend ProfileOpItem;
  friend LinkProfileStep;
  friend WebFProfiler;
};

class LinkProfileStep : public ProfileStep {
 public:
  explicit LinkProfileStep(ProfileOpItem* owner, std::string label);
  ScriptValue ToJSON(JSContext* ctx, const std::string& path, bool should_link) override;

 private:
  friend ProfileOpItem;
};

class ProfileOpItem {
 public:
  explicit ProfileOpItem(WebFProfiler* owner);

  void RecordStep(const std::string& label, const std::shared_ptr<ProfileStep>& step);
  void FinishStep();

  ScriptValue ToJSON(JSContext* ctx, const std::string& path, bool should_link);
  const std::shared_ptr<ProfileStep>& current_step() { return step_stack_.top(); }
  WebFProfiler* owner() { return owner_; }

 protected:
  WebFProfiler* owner_;

 private:
  Stopwatch stopwatch_;

  std::unordered_map<std::string, std::shared_ptr<ProfileStep>> step_map_;
  std::stack<std::shared_ptr<ProfileStep>> step_stack_;
  std::vector<std::shared_ptr<ProfileStep>> steps_;
  friend WebFProfiler;
  friend ProfileStep;
};

class WebFProfiler {
 public:
  explicit WebFProfiler(bool enable);

  void StartTrackInitialize();
  void FinishTrackInitialize();

  void StartTrackEvaluation(int64_t evaluate_id);
  void FinishTrackEvaluation(int64_t evaluate_id);

  void StartTrackAsyncEvaluation();
  void FinishTrackAsyncEvaluation();

  void StartTrackSteps(const std::string& label);
  void FinishTrackSteps();

  void StartTrackLinkSteps(const std::string& label);
  void FinishTrackLinkSteps();

  int64_t link_id() {
    if (UNLIKELY(enabled_)) {
      return profile_stacks_.top()->current_step()->id();
    }
    return 0;
  }

  std::string ToJSON();
  void clear();

 private:
  bool enabled_{false};
  std::stack<std::shared_ptr<ProfileOpItem>> profile_stacks_;
  std::vector<std::shared_ptr<ProfileOpItem>> initialize_profile_items_;
  std::unordered_map<int64_t, std::string> link_paths_;

  std::vector<std::shared_ptr<ProfileOpItem>> async_evaluate_profile_items;
  std::unordered_map<int64_t, std::shared_ptr<ProfileOpItem>> evaluate_profile_items_;

  friend ProfileOpItem;
  friend LinkProfileStep;
};

}  // namespace webf

#endif  // WEBF_FOUNDATION_PROFILER_H_
