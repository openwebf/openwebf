/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_V8_BINDING_INITIALIZER_H
#define WEBF_V8_BINDING_INITIALIZER_H

namespace webf {

class ExecutingContext;

void InstallBindings(ExecutingContext* context);

}  // namespace webf

#endif  // WEBF_V8_BINDING_INITIALIZER_H
