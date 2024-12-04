/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_BUILDFLAG_H
#define WEBF_BUILDFLAG_H

// These macros un-mangle the names of the build flags in a way that looks
// natural, and gives errors if the flag is not defined. Normally in the
// preprocessor it's easy to make mistakes that interpret "you haven't done
// the setup to know what the flag is" as "flag is off". Normally you would
// include the generated header rather than include this file directly.
//
// This is for use with generated headers. See build/buildflag_header.gni.

// This dance of two macros does a concatenation of two preprocessor args using
// ## doubly indirectly because using ## directly prevents macros in that
// parameter from being expanded.
#define BUILDFLAG_CAT_INDIRECT(a, b) a ## b
#define BUILDFLAG_CAT(a, b) BUILDFLAG_CAT_INDIRECT(a, b)

// Accessor for build flags.
//
// To test for a value, if the build file specifies:
//
//   ENABLE_FOO=true
//
// Then you would check at build-time in source code with:
//
//   #include "foo_flags.h"  // The header the build file specified.
//
//   #if BUILDFLAG(ENABLE_FOO)
//     ...
//   #endif
//
// There will no #define called ENABLE_FOO so if you accidentally test for
// whether that is defined, it will always be negative. You can also use
// the value in expressions:
//
//   const char kSpamServerName[] = BUILDFLAG(SPAM_SERVER_NAME);
//
// Because the flag is accessed as a preprocessor macro with (), an error
// will be thrown if the proper header defining the internal flag value has
// not been included.
#define BUILDFLAG(flag) (BUILDFLAG_CAT(BUILDFLAG_INTERNAL_, flag)())

#endif  // WEBF_BUILDFLAG_H
