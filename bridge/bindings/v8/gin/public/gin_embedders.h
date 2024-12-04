/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef GIN_PUBLIC_GIN_EMBEDDERS_H_
#define GIN_PUBLIC_GIN_EMBEDDERS_H_

#include <cstdint>

namespace gin {

// The GinEmbedder is used to identify the owner of embedder data stored on
// v8 objects, and is used as in index into the embedder data slots of a
// v8::Isolate.
//
// GinEmbedder is using uint16_t as underlying storage as V8 requires that
// external pointers in embedder fields are at least 2-byte-aligned.
enum GinEmbedder : uint16_t {
  kEmbedderWebf = 1,
};

}  // namespace gin

#endif  // GIN_PUBLIC_GIN_EMBEDDERS_H_