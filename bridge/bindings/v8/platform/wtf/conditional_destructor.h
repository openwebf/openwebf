/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef WEBF_CONDITIONAL_DESTRUCTOR_H_
#define WEBF_CONDITIONAL_DESTRUCTOR_H_

namespace webf {

// `ConditionalDestructor` defines the destructor of the derived object.
// This base is used in order to completely avoid creating a destructor
// for an object that does not need to be destructed. By doing so,
// the clang compiler will have correct information about whether or not
// the object has a trivial destructor.
template <typename Derived, bool needsDestructor>
class ConditionalDestructor;

template <typename Derived>
class ConditionalDestructor<Derived, true> {
 public:
  ~ConditionalDestructor() { static_cast<Derived*>(this)->Finalize(); }
};

template <typename Derived>
class ConditionalDestructor<Derived, false> {};

}  // namespace webf

#endif  // WEBF_CONDITIONAL_DESTRUCTOR_H_

