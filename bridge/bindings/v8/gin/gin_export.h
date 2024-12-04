/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef GIN_GIN_EXPORT_H_
#define GIN_GIN_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(GIN_IMPLEMENTATION)
#define GIN_EXPORT __declspec(dllexport)
#else
#define GIN_EXPORT __declspec(dllimport)
#endif  // defined(GIN_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(GIN_IMPLEMENTATION)
#define GIN_EXPORT __attribute__((visibility("default")))
#else
#define GIN_EXPORT
#endif  // defined(GIN_IMPLEMENTATION)
#endif

#else  // defined(COMPONENT_BUILD)
#define GIN_EXPORT
#endif

#endif  // GIN_GIN_EXPORT_H_