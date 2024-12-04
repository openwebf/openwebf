/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "bindings/v8/v8_member_installer.h"
#include "core/executing_context.h"

namespace webf {

void MemberInstaller::InstallFunctions(ExecutingContext* context,
                                       std::initializer_list<FunctionConfig> config) {
  v8::Isolate* isolate = context->ctx();
  v8::Local<v8::Context> v8_context = v8::Context::New(isolate);
  v8::Context::Scope context_scope(v8_context);
  v8::Local<v8::Object> global = v8_context->Global();

  for (const auto& function : config) {
    v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate, function.callback);
    v8::Local<v8::Function> v8_function = function_template->GetFunction(v8_context).ToLocalChecked();

    global->Set(v8_context, v8::String::NewFromUtf8(isolate, function.name).ToLocalChecked(), v8_function).Check();
  }
}

}  // namespace webf