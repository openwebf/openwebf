/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */


import {ArrayLikeMethodsMixin} from "../../../bindings/qjs/array_methods";

export interface FormData extends ArrayLikeMethodsMixin {
  new(): FormData;
  append(name: string, value: (string | Blob), fileName?: string): void;
  delete(name: string): ImplementedAs<void, "deleteEntry">;
  get(name: string): (string | Blob);
  getAll(name: string): (string | Blob)[];
  has(name: string): boolean;
  set(name: string, value: string | Blob, fileName?: string): void;
}
