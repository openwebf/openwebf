// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// https://wicg.github.io/IntersectionObserver/#intersection-observer-init

// Copyright (C) 2024-present The WebF authors. All rights reserved.

import {Node} from "./node";

// @ts-ignore
@Dictionary()
export interface IntersectionObserverInit {
  root?: Node | null;
  // TODO(pengfei12.guo): Just definition, no implementation.
  rootMargin?: string;
  threshold?: number[];
  // scrollMargin?: string;
  // delay?: number;
  // trackVisibility?: boolean;
}
