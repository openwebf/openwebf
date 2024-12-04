/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
#ifndef BRIDGE_CONTEXT_DATA_H
#define BRIDGE_CONTEXT_DATA_H

#if WEBF_V8_JS_ENGINE
#include <v8/v8.h>
#elif WEBF_QUICKJS_JS_ENGINE
#include <quickjs/quickjs.h>
#include "bindings/qjs/wrapper_type_info.h"
#endif
#include <unordered_map>

namespace webf {

class ExecutingContext;

// Used to hold data that is associated with a single ExecutionContext object, and
// has a 1:1 relationship with ExecutionContext.
class ExecutionContextData final {
 public:
  explicit ExecutionContextData(ExecutingContext* context) : m_context(context){};
  ExecutionContextData(const ExecutionContextData&) = delete;
  ExecutionContextData& operator=(const ExecutionContextData&) = delete;

#if WEBF_QUICKJS_JS_ENGINE
  // Returns the constructor object that is appropriately initialized.
  JSValue constructorForType(const WrapperTypeInfo* type);
  // Returns the prototype object that is appropriately initialized.
  JSValue prototypeForType(const WrapperTypeInfo* type);
#endif

  void Dispose();

 private:
#if WEBF_QUICKJS_JS_ENGINE
  JSValue constructorForIdSlowCase(const WrapperTypeInfo* type);
  std::unordered_map<const WrapperTypeInfo*, JSValue> constructor_map_;
  std::unordered_map<const WrapperTypeInfo*, JSValue> prototype_map_;
#endif

  ExecutingContext* m_context;
};

}  // namespace webf

#endif  // BRIDGE_CONTEXT_DATA_H
