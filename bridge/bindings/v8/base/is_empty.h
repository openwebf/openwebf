/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef BASE_MACROS_IS_EMPTY_H_
#define BASE_MACROS_IS_EMPTY_H_

// A macro that substitutes with 1 if called without arguments, otherwise 0.
#define BASE_IS_EMPTY(...) BASE_INTERNAL_IS_EMPTY_EXPANDED(__VA_ARGS__)
#define BASE_INTERNAL_IS_EMPTY_EXPANDED(...) \
  BASE_INTERNAL_IS_EMPTY_INNER(_, ##__VA_ARGS__)
#define BASE_INTERNAL_IS_EMPTY_INNER(...) \
  BASE_INTERNAL_IS_EMPTY_INNER_EXPANDED(__VA_ARGS__, 0, 1)
#define BASE_INTERNAL_IS_EMPTY_INNER_EXPANDED(e0, e1, is_empty, ...) is_empty

#endif  // BASE_MACROS_IS_EMPTY_H_
