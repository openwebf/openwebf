/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef BRIDGE_NATIVE_STRING_UTILS_H
#define BRIDGE_NATIVE_STRING_UTILS_H

#if WEBF_QUICKJS_JS_ENGINE
#include <quickjs/quickjs.h>
#endif
#include <codecvt>
#include <locale>
#include <memory>
#include <string>

#include "foundation/native_string.h"

namespace webf {

// Convert to string and return a full copy of NativeString from JSValue.
std::unique_ptr<SharedNativeString> jsValueToNativeString(JSContext* ctx, JSValue value);

// Encode utf-8 to utf-16, and return a full copy of NativeString.
std::unique_ptr<SharedNativeString> stringToNativeString(const std::string& string);

std::string nativeStringToStdString(const SharedNativeString* native_string);

template <typename T>
std::string toUTF8(const std::basic_string<T, std::char_traits<T>, std::allocator<T>>& source) {
  std::string result;

  std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
  result = convertor.to_bytes(source);

  return result;
}

template <typename T>
void fromUTF8(const std::string& source, std::basic_string<T, std::char_traits<T>, std::allocator<T>>& result) {
  std::wstring_convert<std::codecvt_utf8_utf16<T>, T> convertor;
  result = convertor.from_bytes(source);
}

}  // namespace webf

#endif  // BRIDGE_NATIVE_STRING_UTILS_H
