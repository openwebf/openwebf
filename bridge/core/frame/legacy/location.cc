/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#include "location.h"
#include "core/executing_context.h"
#include "core/frame/window.h"

namespace webf {

void Location::__webf_location_reload__(ExecutingContext* context, ExceptionState& exception_state) {
  context->FlushUICommand(context->window(), FlushUICommandReason::kDependentsOnElement);
  context->dartMethodPtr()->reloadApp(context->isDedicated(), context->contextId());
}

}  // namespace webf
