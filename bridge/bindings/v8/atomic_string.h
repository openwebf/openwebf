/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef BRIDGE_BINDINGS_V8_ATOMIC_STRING_H_
#define BRIDGE_BINDINGS_V8_ATOMIC_STRING_H_

#include <v8/v8.h>
#include <cassert>
#include <functional>
#include <memory>
#include "foundation/macros.h"
#include "foundation/native_string.h"
#include "foundation/string_view.h"

namespace webf {

typedef bool (*CharacterMatchFunctionPtr)(char);

// An AtomicString instance represents a string, and multiple AtomicString
// instances can share their string storage if the strings are
// identical. Comparing two AtomicString instances is much faster than comparing
// two String instances because we just check string storage identity.
class AtomicString {
  WEBF_DISALLOW_NEW();

 public:
  enum class StringKind { kIsLowerCase, kIsUpperCase, kIsMixed, kUnknown };

  struct KeyHasher {
    std::size_t operator()(const AtomicString& k) const { return k.string_->GetIdentityHash(); }
  };

  static AtomicString Empty();
  static AtomicString Null();

  AtomicString() = default;
  AtomicString(v8::Isolate* isolate, const std::string& string);
  AtomicString(v8::Isolate* isolate, const char* str, size_t length);
  AtomicString(v8::Isolate* isolate, std::unique_ptr<AutoFreeNativeString>&& native_string);
  AtomicString(v8::Isolate* isolate, const uint16_t* str, size_t length);
  AtomicString(v8::Local<v8::Context> context, v8::Local<v8::Value> v8_value);

  // Return the undefined string value from atom key.
  v8::Local<v8::Value> ToV8(v8::Isolate* isolate) const { return string_.As<v8::Value>(); }

  bool IsEmpty() const;
  bool IsNull() const;

  int64_t length() const { return string_->Length(); }

  bool Is8Bit() const;
  const uint8_t* Character8() const;
  const uint16_t* Character16() const;

  int Find(bool (*CharacterMatchFunction)(char)) const;
  int Find(bool (*CharacterMatchFunction)(uint16_t)) const;

  [[nodiscard]] std::string ToStdString(v8::Isolate* isolate) const;
  [[nodiscard]] std::unique_ptr<SharedNativeString> ToNativeString(v8::Isolate* isolate) const;

  StringView ToStringView() const;

  AtomicString ToUpperIfNecessary(v8::Isolate* isolate) const;
  AtomicString ToUpperSlow(v8::Isolate* isolate) const;

  AtomicString ToLowerIfNecessary(v8::Isolate* isolate) const;
  AtomicString ToLowerSlow(v8::Isolate* isolate) const;

  inline bool ContainsOnlyLatin1OrEmpty() const;
  AtomicString RemoveCharacters(v8::Isolate* isolate, CharacterMatchFunctionPtr find_match);

  // Copy assignment
  AtomicString(AtomicString const& value);
  AtomicString& operator=(const AtomicString& other) noexcept;

  // Move assignment
  AtomicString(AtomicString&& value) noexcept;
  AtomicString& operator=(AtomicString&& value) noexcept;

  bool operator==(const AtomicString& other) const { return other.string_->StringEquals(string_); }
  bool operator!=(const AtomicString& other) const { return !other.string_->StringEquals(string_); };

 protected:
  StringKind kind_;
  v8::Isolate* isolate_;
  v8::Local<v8::String> string_;
  mutable v8::Local<v8::String> string_upper_;
  mutable v8::Local<v8::String> string_lower_;
};

bool AtomicString::ContainsOnlyLatin1OrEmpty() const {
  if (IsEmpty())
    return true;

  if (Is8Bit())
    return true;

  const uint16_t* characters = Character16();
  uint16_t ored = 0;
  for (size_t i = 0; i < string_->Length(); ++i)
    ored |= characters[i];
  return !(ored & 0xFF00);
}

inline v8::Local<v8::String> V8AtomicString(v8::Isolate* isolate,
                                            const char* string) {
  assert(isolate);
  if (!string || string[0] == '\0')
    return v8::String::Empty(isolate);
  return v8::String::NewFromOneByte(
             isolate, reinterpret_cast<const uint8_t*>(string),
             v8::NewStringType::kInternalized, static_cast<int>(strlen(string)))
      .ToLocalChecked();
}

}  // namespace webf

#endif  // BRIDGE_BINDINGS_QJS_ATOMIC_STRING_H_
