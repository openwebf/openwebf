/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_V8_MEMBER_INSTALLER_H
#define WEBF_V8_MEMBER_INSTALLER_H

#include <initializer_list>
#include <v8/v8.h>

namespace webf {

class ExecutingContext;

// A set of utility functions to define attributes members as ES properties.
class MemberInstaller {
 public:
  struct FunctionConfig {
    FunctionConfig& operator=(const FunctionConfig&) = delete;
    const char* name;
    v8::FunctionCallback callback;
  };

  static void InstallFunctions(ExecutingContext* context, std::initializer_list<FunctionConfig> config);
};

}  // namespace webf

#endif  // WEBF_V8_MEMBER_INSTALLER_H
