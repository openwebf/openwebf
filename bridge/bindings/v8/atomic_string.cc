/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "atomic_string.h"
#include <algorithm>
#include <vector>
#include "built_in_string.h"
#include "foundation/native_string.h"
#include "native_string_utils.h"

namespace webf {

AtomicString AtomicString::Empty() {
  return built_in_string::kempty_string;
}

AtomicString AtomicString::Null() {
  return built_in_string::kNULL;
}

namespace {

AtomicString::StringKind GetStringKind(const std::string& string, size_t length) {
  char first_char = string[0];

  if (first_char < 0 || first_char > 255) {
    return AtomicString::StringKind::kUnknown;
  }

  AtomicString::StringKind predictKind =
      std::islower(string[0]) ? AtomicString::StringKind::kIsLowerCase : AtomicString::StringKind::kIsUpperCase;
  for (int i = 0; i < length; i++) {
    char c = string[i];

    if (c < 0 || c > 255) {
      return AtomicString::StringKind::kUnknown;
    }

    if (predictKind == AtomicString::StringKind::kIsUpperCase && !std::isupper(c)) {
      return AtomicString::StringKind::kIsMixed;
    } else if (predictKind == AtomicString::StringKind::kIsLowerCase && !std::islower(c)) {
      return AtomicString::StringKind::kIsMixed;
    }
  }
  return predictKind;
}

AtomicString::StringKind GetStringKind(const SharedNativeString* native_string) {
  if (!native_string->length()) {
    return AtomicString::StringKind::kIsMixed;
  }

  AtomicString::StringKind predictKind = std::islower(native_string->string()[0])
                                             ? AtomicString::StringKind::kIsLowerCase
                                             : AtomicString::StringKind::kIsUpperCase;
  for (int i = 0; i < native_string->length(); i++) {
    uint16_t c = native_string->string()[i];
    if (predictKind == AtomicString::StringKind::kIsUpperCase && !std::isupper(c)) {
      return AtomicString::StringKind::kIsMixed;
    } else if (predictKind == AtomicString::StringKind::kIsLowerCase && !std::islower(c)) {
      return AtomicString::StringKind::kIsMixed;
    }
  }

  return predictKind;
}

}  // namespace

class AtomicStringOneByteResource : public v8::String::ExternalOneByteStringResource {
 public:
  AtomicStringOneByteResource(const std::string& string) : string_(string){};

  const char* data() const override { return string_.data(); };
  size_t length() const override { return string_.length(); };

 private:
  std::string string_;
};

class AtomicStringTwoByteResource : public v8::String::ExternalStringResource {
 public:
  AtomicStringTwoByteResource(std::unique_ptr<AutoFreeNativeString>&& native_string)
      : string_(std::move(native_string)) {}

  const uint16_t* data() const override { return string_->string(); }
  size_t length() const override { return string_->length(); }

 private:
  std::unique_ptr<AutoFreeNativeString> string_;
};

AtomicString::AtomicString(v8::Isolate* isolate, const std::string& string)
    : kind_(GetStringKind(string, string.size())), isolate_(isolate) {
  auto* external_string_resource = new AtomicStringOneByteResource(string);
  string_ = v8::String::NewExternalOneByte(isolate, external_string_resource).ToLocalChecked();
}

AtomicString::AtomicString(v8::Isolate* isolate, const char* str, size_t length)
    : kind_(GetStringKind(str, length)), isolate_(isolate) {
  auto* external_string_resource = new AtomicStringOneByteResource(std::string(str, length));
  string_ = v8::String::NewExternalOneByte(isolate, external_string_resource).ToLocalChecked();
}

AtomicString::AtomicString(v8::Isolate* isolate, std::unique_ptr<AutoFreeNativeString>&& native_string)
    : isolate_(isolate) {
  kind_ = GetStringKind(native_string.get());
  auto* external_resource = new AtomicStringTwoByteResource(std::move(native_string));
  string_ = v8::String::NewExternalTwoByte(isolate, external_resource).ToLocalChecked();
}

AtomicString::AtomicString(v8::Isolate* isolate, const uint16_t* str, size_t length) : isolate_(isolate) {
  auto native_string = std::unique_ptr<AutoFreeNativeString>(
      reinterpret_cast<AutoFreeNativeString*>(new SharedNativeString(str, length)));
  kind_ = GetStringKind(native_string.get());
  auto* external_resource = new AtomicStringTwoByteResource(std::move(native_string));
  string_ = v8::String::NewExternalTwoByte(isolate, external_resource).ToLocalChecked();
}

AtomicString::AtomicString(v8::Local<v8::Context> context, v8::Local<v8::Value> v8_value) {
  auto&& raw_native_string = jsValueToNativeString(context, v8_value);
  auto native_string =
      std::unique_ptr<AutoFreeNativeString>(reinterpret_cast<AutoFreeNativeString*>(raw_native_string.release()));
  kind_ = GetStringKind(native_string.get());
  auto* external_resource = new AtomicStringTwoByteResource(std::move(native_string));
  string_ = v8::String::NewExternalTwoByte(context->GetIsolate(), external_resource).ToLocalChecked();
}

bool AtomicString::IsEmpty() const {
  return *this == built_in_string::kempty_string || IsNull();
}

bool AtomicString::IsNull() const {
  return string_->IsNull();
}

bool AtomicString::Is8Bit() const {
  return string_->IsExternalOneByte();
}

const uint8_t* AtomicString::Character8() const {
  assert(string_->IsExternal());
  return reinterpret_cast<const uint8_t*>(string_->GetExternalOneByteStringResource()->data());
}

const uint16_t* AtomicString::Character16() const {
  assert(string_->IsExternal());
  return string_->GetExternalStringResource()->data();
}

int AtomicString::Find(bool (*CharacterMatchFunction)(char)) const {
  //  return JS_FindCharacterInAtom(runtime_, atom_, CharacterMatchFunction);
}

int AtomicString::Find(bool (*CharacterMatchFunction)(uint16_t)) const {
  //  return JS_FindWCharacterInAtom(runtime_, atom_, CharacterMatchFunction);
}

std::string AtomicString::ToStdString(v8::Isolate* isolate) const {
  if (IsEmpty())
    return "";

  if (string_->IsExternalOneByte()) {
    return {string_->GetExternalOneByteStringResource()->data()};
  }

  std::string result;
  size_t length = string_->Utf8Length(isolate);
  result.reserve(length);

  string_->WriteUtf8(isolate, result.data());
  return result;
}

std::unique_ptr<SharedNativeString> AtomicString::ToNativeString(v8::Isolate* isolate) const {
  if (IsNull()) {
    // Null string is same like empty string
    return built_in_string::kempty_string.ToNativeString(isolate);
  }

  if (string_->IsExternalTwoByte()) {
    auto* resource = string_->GetExternalStringResource();
    return SharedNativeString::FromTemporaryString(resource->data(), resource->length());
  }

  size_t length = string_->Length();
  std::vector<uint16_t> buffer;
  buffer.reserve(length);
  string_->Write(isolate, buffer.data());
  return SharedNativeString::FromTemporaryString(buffer.data(), length);
}

StringView AtomicString::ToStringView() const {
  if (IsNull()) {
    return built_in_string::kempty_string.ToStringView();
  }

  if (string_->IsExternalOneByte()) {
    auto* resource = string_->GetExternalOneByteStringResource();
    return StringView((void*)(resource->data()), resource->length(), false);
  }

  auto* resource = string_->GetExternalStringResource();

  return StringView((void*)(resource->data()), resource->length(), true);
}

AtomicString AtomicString::ToUpperIfNecessary(v8::Isolate* isolate) const {
  if (kind_ == StringKind::kIsUpperCase) {
    return *this;
  }
  if (!string_upper_->IsNull() || IsNull())
    return *this;
  AtomicString upperString = ToUpperSlow(isolate);
  string_upper_ = v8::Local<v8::String>(upperString.string_);
  return upperString;
}

AtomicString AtomicString::ToUpperSlow(v8::Isolate* isolate) const {
  std::string str = ToStdString(isolate);
  std::transform(str.begin(), str.end(), str.begin(), toupper);
  return {isolate, str};
}

AtomicString AtomicString::ToLowerIfNecessary(v8::Isolate* isolate) const {
  if (kind_ == StringKind::kIsLowerCase) {
    return *this;
  }
  if (!string_lower_->IsNull() || IsNull())
    return *this;
  AtomicString lowerString = ToLowerSlow(isolate);
  string_lower_ = lowerString.string_;
  return lowerString;
}

AtomicString AtomicString::ToLowerSlow(v8::Isolate* isolate) const {
  std::string str = ToStdString(isolate);
  std::transform(str.begin(), str.end(), str.begin(), tolower);
  return {isolate, str};
}

template <typename CharType>
inline AtomicString RemoveCharactersInternal(v8::Isolate* isolate,
                                             const AtomicString& self,
                                             const CharType* characters,
                                             size_t len,
                                             CharacterMatchFunctionPtr find_match) {
  const CharType* from = characters;
  const CharType* fromend = from + len;

  // Assume the common case will not remove any characters
  while (from != fromend && !find_match(*from))
    ++from;
  if (from == fromend)
    return self;

  auto* to = (CharType*)malloc(len);
  size_t outc = static_cast<size_t>(from - characters);

  if (outc)
    memcpy(to, characters, outc * sizeof(CharType));

  while (true) {
    while (from != fromend && find_match(*from))
      ++from;
    while (from != fromend && !find_match(*from))
      to[outc++] = *from++;
    if (from == fromend)
      break;
  }

  AtomicString str;

  if (outc == 0) {
    return AtomicString::Empty();
  }

  auto data = (CharType*)malloc(outc);
  memcpy(data, to, outc);
  free(to);
  if (self.Is8Bit()) {
    str = AtomicString(isolate, reinterpret_cast<const char*>(data), outc);
  } else {
    str = AtomicString(isolate, reinterpret_cast<const uint16_t*>(data), outc);
  }

  free(data);
  return str;
}

AtomicString AtomicString::RemoveCharacters(v8::Isolate* isolate, CharacterMatchFunctionPtr find_match) {
  if (IsEmpty())
    return AtomicString::Empty();
  if (Is8Bit())
    return RemoveCharactersInternal(isolate, *this, Character8(), string_->Utf8Length(isolate), find_match);
  return RemoveCharactersInternal(isolate, *this, Character16(), string_->Length(), find_match);
}

AtomicString::AtomicString(const webf::AtomicString& value) {
  string_ = v8::Local<v8::String>(value.string_);
}
AtomicString& AtomicString::operator=(const webf::AtomicString& other) noexcept {
  string_ = other.string_;
  return *this;
}

AtomicString::AtomicString(webf::AtomicString&& value) noexcept {
  string_ = v8::Local<v8::String>(value.string_);
}
AtomicString& AtomicString::operator=(webf::AtomicString&& value) noexcept {
  string_ = v8::Local<v8::String>(value.string_);
  return *this;
}

}  // namespace webf
