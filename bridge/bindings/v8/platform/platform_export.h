/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

// This header defines the following macros to export component's symbols.
//
// - PLATFORM_EXPORT
//   Exports non-template symbols.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_PLATFORM_EXPORT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_PLATFORM_EXPORT_H_

#include "bindings/v8/for_build/build_config.h"

//
// BLINK_PLATFORM_IMPLEMENTATION
//
#if !defined(BLINK_PLATFORM_IMPLEMENTATION)
#define BLINK_PLATFORM_IMPLEMENTATION 0
#endif

//
// PLATFORM_EXPORT
//
#if !defined(COMPONENT_BUILD)
#define PLATFORM_EXPORT  // No need of export
#else

#if defined(COMPILER_MSVC)
#if BLINK_PLATFORM_IMPLEMENTATION
#define PLATFORM_EXPORT __declspec(dllexport)
#else
#define PLATFORM_EXPORT __declspec(dllimport)
#endif
#endif  // defined(COMPILER_MSVC)

#if defined(COMPILER_GCC)
#if BLINK_PLATFORM_IMPLEMENTATION
#define PLATFORM_EXPORT __attribute__((visibility("default")))
#else
#define PLATFORM_EXPORT
#endif
#endif  // defined(COMPILER_GCC)

#endif  // !defined(COMPONENT_BUILD)

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_PLATFORM_EXPORT_H_