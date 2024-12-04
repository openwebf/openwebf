/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_UNION_BASE_H
#define WEBF_UNION_BASE_H

//#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
//#include "third_party/blink/renderer/platform/platform_export.h"
//#include "v8/include/v8.h"
#include <v8/v8.h>
#include "platform/heap/garbage_collected.h"

namespace webf {

class ExceptionState;
class ScriptState;

namespace bindings {

// UnionBase is the common base class of all the IDL union classes.  Most
// importantly this class provides a way of type dispatching (e.g. overload
// resolutions, SFINAE technique, etc.) so that it's possible to distinguish
// IDL unions from anything else.  Also it provides a common implementation of
// IDL unions.
class UnionBase : public GarbageCollected<UnionBase> {
 public:
  virtual ~UnionBase() = default;

  virtual v8::Local<v8::Value> ToV8(ScriptState* script_state) const = 0;

  virtual void Trace(Visitor*) const {}

 protected:
  // Helper function to reduce the binary size of the generated bindings.
  static void ThrowTypeErrorNotOfType(ExceptionState& exception_state,
                                      const char* expected_type);

  UnionBase() = default;
};

}  // namespace bindings

}  // namespace webf

#endif  // WEBF_UNION_BASE_H
