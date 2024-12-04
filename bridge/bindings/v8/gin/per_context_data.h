/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef GIN_PER_CONTEXT_DATA_H_
#define GIN_PER_CONTEXT_DATA_H_

#include "bindings/v8/base/memory/raw_ptr.h"
//#include "base/supports_user_data.h"
#include "bindings/v8/gin/gin_export.h"
#include <v8/v8-forward.h>

namespace gin {

class ContextHolder;
class Runner;

// There is one instance of PerContextData per v8::Context managed by Gin. This
// class stores all the Gin-related data that varies per context. Arbitrary data
// can be associated with this class by way of the SupportsUserData methods.
// Instances of this class (and any associated user data) are destroyed before
// the associated v8::Context.
// TODO webf not include SupportsUserData for now
class GIN_EXPORT PerContextData {
 public:
  PerContextData(ContextHolder* context_holder,
                 v8::Local<v8::Context> context);
  PerContextData(const PerContextData&) = delete;
  PerContextData& operator=(const PerContextData&) = delete;
//  ~PerContextData() override;

  // Can return NULL after the ContextHolder has detached from context.
  static PerContextData* From(v8::Local<v8::Context> context);

  // The Runner associated with this context. To execute script in this context,
  // please use the appropriate API on Runner.
  Runner* runner() const { return runner_; }
  void set_runner(Runner* runner) { runner_ = runner; }

  ContextHolder* context_holder() { return context_holder_; }

 private:
  ContextHolder *context_holder_;
  Runner *runner_;
};

}  // namespace gin

#endif  // GIN_PER_CONTEXT_DATA_H_

