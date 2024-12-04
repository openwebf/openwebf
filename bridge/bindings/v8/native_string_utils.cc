/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "native_string_utils.h"

namespace webf {

std::unique_ptr<SharedNativeString> jsValueToNativeString(v8::Local<v8::Context> ctx, v8::Local<v8::Value> value) {
  v8::Local<v8::String> string_value;
  if (value->IsNull()) {
    value = v8::String::NewFromUtf8(ctx->GetIsolate(), "").ToLocalChecked().As<v8::Value>();
  } else if (!value->IsString()) {
    value = value->ToString(ctx).ToLocalChecked();
  } else {
    string_value = value.As<v8::String>();
  }

  uint32_t length = string_value->Length();
  auto* buffer = (uint16_t*) malloc(sizeof(uint16_t) * length);

  string_value->Write(ctx->GetIsolate(), buffer);
  std::unique_ptr<SharedNativeString> ptr = std::make_unique<SharedNativeString>(buffer, length);

  return ptr;
}

std::unique_ptr<SharedNativeString> stringToNativeString(const std::string& string) {
  std::u16string utf16;
  fromUTF8(string, utf16);
  SharedNativeString tmp{reinterpret_cast<const uint16_t*>(utf16.c_str()), static_cast<uint32_t>(utf16.size())};
  return SharedNativeString::FromTemporaryString(tmp.string(), tmp.length());
}

std::string nativeStringToStdString(const SharedNativeString* native_string) {
  std::u16string u16EventType =
      std::u16string(reinterpret_cast<const char16_t*>(native_string->string()), native_string->length());
  return toUTF8(u16EventType);
}


}  // namespace webf
