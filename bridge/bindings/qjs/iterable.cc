/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "iterable.h"

namespace webf {


ScriptValue ESCreateIterResultObject(JSContext* ctx, bool done, const ScriptValue& value) {
  JSValue object = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, object, "done", JS_NewBool(ctx, done));
  JS_SetPropertyStr(ctx, object, "value", JS_DupValue(ctx, value.QJSValue()));
  return ScriptValue(ctx, object);
}

ScriptValue ESCreateIterResultObject(JSContext* ctx, bool done, const ScriptValue& value1, const ScriptValue& value2) {
  JSValue object = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, object, "done", JS_NewBool(ctx, done));
  JSValue array = JS_NewArray(ctx);
  JS_SetPropertyInt64(ctx, array, 0, JS_DupValue(ctx, value1.QJSValue()));
  JS_SetPropertyInt64(ctx, array, 1, JS_DupValue(ctx, value2.QJSValue()));
  JS_SetPropertyStr(ctx, object, "value", array);
  return ScriptValue(ctx, object);
}

}