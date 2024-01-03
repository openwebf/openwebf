/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#include "atomic_string.h"
#include <algorithm>
#include <vector>
#include "built_in_string.h"
#include "foundation/native_string.h"
#include "qjs_engine_patch.h"

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

AtomicString::StringKind GetStringKind(JSValue stringValue) {
  JSString* p = JS_VALUE_GET_STRING(stringValue);

  if (p->is_wide_char) {
    return AtomicString::StringKind::kIsMixed;
  }

  return GetStringKind(reinterpret_cast<const char*>(p->u.str8), p->len);
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

AtomicString::AtomicString(JSContext* ctx, const std::string& string)
    : runtime_(JS_GetRuntime(ctx)),
      atom_(JS_NewAtomLen(ctx, string.c_str(), string.size())),
      kind_(GetStringKind(string, string.size())),
      length_(string.size()) {}

AtomicString::AtomicString(JSContext* ctx, const char* str, size_t length)
    : runtime_(JS_GetRuntime(ctx)),
      atom_(JS_NewAtomLen(ctx, str, length)),
      kind_(GetStringKind(str, length)),
      length_(length) {}

AtomicString::AtomicString(JSContext* ctx, const uint16_t* str, size_t length) : runtime_(JS_GetRuntime(ctx)) {
  JSValue string = JS_NewUnicodeString(ctx, str, length);
  atom_ = JS_ValueToAtom(ctx, string);
  kind_ = GetStringKind(string);
  length_ = length;
  JS_FreeValue(ctx, string);
}

AtomicString::AtomicString(JSContext* ctx, const std::unique_ptr<AutoFreeNativeString>& native_string)
    : runtime_(JS_GetRuntime(ctx)),
      atom_(JS_NewUnicodeAtom(ctx, native_string->string(), native_string->length())),
      kind_(GetStringKind(native_string.get())),
      length_(native_string->length()){};

AtomicString::AtomicString(JSContext* ctx, JSValue value)
    : runtime_(JS_GetRuntime(ctx)), atom_(JS_ValueToAtom(ctx, value)) {
  if (JS_IsString(value)) {
    kind_ = GetStringKind(value);
    length_ = JS_VALUE_GET_STRING(value)->len;
  } else {
    initFromAtom(ctx);
  }
}

AtomicString::AtomicString(JSContext* ctx, JSAtom atom) : runtime_(JS_GetRuntime(ctx)), atom_(JS_DupAtom(ctx, atom)) {
  initFromAtom(ctx);
}

void AtomicString::initFromAtom(JSContext* ctx) {
  if (atom_ != JS_ATOM_NULL) {
    auto atom_str = JS_AtomToValue(ctx, atom_);
    kind_ = GetStringKind(atom_str);
    length_ = JS_VALUE_GET_STRING(atom_str)->len;
    JS_FreeValue(ctx, atom_str);
  } else {
    kind_ = StringKind::kIsMixed;
    length_ = 0;
    atom_upper_ = JS_ATOM_NULL;
    atom_lower_ = JS_ATOM_NULL;
  }
}

bool AtomicString::IsEmpty() const {
  return *this == built_in_string::kempty_string || IsNull();
}

bool AtomicString::IsNull() const {
  return atom_ == JS_ATOM_NULL;
}

bool AtomicString::Is8Bit() const {
  return JS_AtomIs8Bit(runtime_, atom_);
}

const uint8_t* AtomicString::Character8() const {
  return JS_AtomRawCharacter8(runtime_, atom_);
}

const uint16_t* AtomicString::Character16() const {
  return JS_AtomRawCharacter16(runtime_, atom_);
}

int AtomicString::Find(bool (*CharacterMatchFunction)(char)) const {
  return JS_FindCharacterInAtom(runtime_, atom_, CharacterMatchFunction);
}

int AtomicString::Find(bool (*CharacterMatchFunction)(uint16_t)) const {
  return JS_FindWCharacterInAtom(runtime_, atom_, CharacterMatchFunction);
}

std::string AtomicString::ToStdString(JSContext* ctx) const {
  if (IsEmpty())
    return "";

  JSValue str = JS_AtomToString(ctx, atom_);
  size_t len;
  const char* buf = JS_ToCStringLen(ctx, &len, str);
  JS_FreeValue(ctx, str);

  // to support \0 in JS string. So must be passing len.
  std::string result = std::string(buf, len);
  JS_FreeCString(ctx, buf);
  return result;
}

std::unique_ptr<SharedNativeString> AtomicString::ToNativeString(JSContext* ctx) const {
  if (IsNull()) {
    // Null string is same like empty string
    return built_in_string::kempty_string.ToNativeString(ctx);
  }
  JSValue stringValue = JS_AtomToValue(ctx, atom_);
  uint32_t length;
  uint16_t* bytes = JS_ToUnicode(ctx, stringValue, &length);
  JS_FreeValue(ctx, stringValue);
  return std::make_unique<SharedNativeString>(bytes, length);
}

StringView AtomicString::ToStringView() const {
  if (IsNull()) {
    return built_in_string::kempty_string.ToStringView();
  }
  return JSAtomToStringView(runtime_, atom_);
}

AtomicString::AtomicString(const AtomicString& value) {
  if (value.IsNull()) {
    atom_ = value.atom_;
    atom_upper_ = value.atom_upper_;
    atom_lower_ = value.atom_lower_;
  } else if (&value != this) {
    atom_ = JS_DupAtomRT(value.runtime_, value.atom_);
  }
  runtime_ = value.runtime_;
  length_ = value.length_;
  kind_ = value.kind_;
}

AtomicString& AtomicString::operator=(const AtomicString& other) {
  if (&other != this && !other.IsNull()) {
    JS_FreeAtomRT(other.runtime_, atom_);
    atom_ = JS_DupAtomRT(other.runtime_, other.atom_);
  }
  runtime_ = other.runtime_;
  length_ = other.length_;
  kind_ = other.kind_;
  return *this;
}

AtomicString::AtomicString(AtomicString&& value) noexcept {
  if (&value != this && !value.IsNull()) {
    atom_ = JS_DupAtomRT(value.runtime_, value.atom_);
  }
  runtime_ = value.runtime_;
  length_ = value.length_;
  kind_ = value.kind_;
}

AtomicString& AtomicString::operator=(AtomicString&& value) noexcept {
  if (&value != this && !value.IsNull()) {
    JS_FreeAtomRT(value.runtime_, atom_);
    atom_ = JS_DupAtomRT(value.runtime_, value.atom_);
  }
  runtime_ = value.runtime_;
  length_ = value.length_;
  kind_ = value.kind_;
  return *this;
}

AtomicString AtomicString::ToUpperIfNecessary(JSContext* ctx) const {
  if (kind_ == StringKind::kIsUpperCase) {
    return *this;
  }
  if (atom_upper_ != JS_ATOM_NULL || IsNull())
    return *this;
  AtomicString upperString = ToUpperSlow(ctx);
  atom_upper_ = upperString.atom_;
  return upperString;
}

AtomicString AtomicString::ToUpperSlow(JSContext* ctx) const {
  const char* cptr = JS_AtomToCString(ctx, atom_);
  std::string str = std::string(cptr);
  std::transform(str.begin(), str.end(), str.begin(), toupper);
  JS_FreeCString(ctx, cptr);
  return AtomicString(ctx, str);
}

AtomicString AtomicString::ToLowerIfNecessary(JSContext* ctx) const {
  if (kind_ == StringKind::kIsLowerCase) {
    return *this;
  }
  if (atom_upper_ != JS_ATOM_NULL || IsNull())
    return *this;
  AtomicString lowerString = ToLowerSlow(ctx);
  atom_lower_ = lowerString.atom_;
  return lowerString;
}

AtomicString AtomicString::ToLowerSlow(JSContext* ctx) const {
  const char* cptr = JS_AtomToCString(ctx, atom_);
  std::string str = std::string(cptr);
  std::transform(str.begin(), str.end(), str.begin(), tolower);
  JS_FreeCString(ctx, cptr);
  return AtomicString(ctx, str);
}

template <typename CharType>
inline AtomicString RemoveCharactersInternal(JSContext* ctx,
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

  auto* to = (CharType*)js_malloc(ctx, len);
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

  auto data = (CharType*)js_malloc(ctx, outc);
  memcpy(data, to, outc);
  js_free(ctx, to);
  if (self.Is8Bit()) {
    str = AtomicString(ctx, reinterpret_cast<const char*>(data), outc);
  } else {
    str = AtomicString(ctx, reinterpret_cast<const uint16_t*>(data), outc);
  }

  js_free(ctx, data);
  return str;
}

AtomicString AtomicString::RemoveCharacters(JSContext* ctx, CharacterMatchFunctionPtr find_match) {
  if (IsEmpty())
    return AtomicString::Empty();
  if (Is8Bit())
    return RemoveCharactersInternal(ctx, *this, Character8(), length_, find_match);
  return RemoveCharactersInternal(ctx, *this, Character16(), length_, find_match);
}

}  // namespace webf
