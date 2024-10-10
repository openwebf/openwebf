/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

#ifndef WEBF_CORE_API_FORM_DATA_H_
#define WEBF_CORE_API_FORM_DATA_H_

#include <string>
#include <vector>
#include "bindings/qjs/atomic_string.h"
#include "bindings/qjs/cppgc/member.h"
#include "bindings/qjs/iterable.h"
#include "bindings/qjs/script_wrappable.h"
#include "core/binding_object.h"
#include "core/fileapi/blob.h"
#include "qjs_union_dom_stringblob.h"

namespace webf {

class FormData;

// Exchange data struct designed for reading the bytes data from Dart Side.
struct NativeFormData : public DartReadable {

};

class FormData : public BindingObject, public PairSyncIterable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  class Entry;
  using ImplType = FormData*;

  FormData() = delete;
  static FormData* Create(ExecutingContext* context, ExceptionState& exception_state);
  explicit FormData(JSContext* ctx);
  NativeValue HandleCallFromDartSide(const AtomicString& method,
                                     int32_t argc,
                                     const NativeValue* argv,
                                     Dart_Handle dart_object) override;
  bool IsFormData() const override;
  void append(const AtomicString& name,
              const std::shared_ptr<QJSUnionDomStringBlob>& value,
              ExceptionState& exception_state);
  void append(const AtomicString& name,
              const std::shared_ptr<QJSUnionDomStringBlob>& value,
              const AtomicString& file_name,
              ExceptionState& exception_state);
  void deleteEntry(const AtomicString& name, ExceptionState& exception_state);
  std::shared_ptr<QJSUnionDomStringBlob> get(const AtomicString& name, ExceptionState& exception_state);
  std::vector<std::shared_ptr<QJSUnionDomStringBlob>> getAll(const AtomicString& name, ExceptionState& exception_state);
  bool has(const AtomicString& name, ExceptionState& exception_state);
  void set(const AtomicString& name,
           const std::shared_ptr<QJSUnionDomStringBlob>& blob_part,
           ExceptionState& exception_state);
  void set(const AtomicString& name,
           const std::shared_ptr<QJSUnionDomStringBlob>& blob_part,
           const AtomicString& file_name,
           ExceptionState& exception_state);

  void SetEntry(std::shared_ptr<Entry> entry);
  const std::vector<std::shared_ptr<const Entry>>& Entries() const { return entries_; }

  void Trace(webf::GCVisitor* visitor) const override;

  void forEach(const std::shared_ptr<QJSFunction>& callback,
               const webf::ScriptValue& this_arg,
               webf::ExceptionState& exception_state) override;
  void forEach(const std::shared_ptr<QJSFunction>& callback, webf::ExceptionState& exception_state) override;

 private:
  std::shared_ptr<PairSyncIterationSource> CreateIterationSource(webf::ExceptionState& exception_state) override;

  void append(const AtomicString& name, const AtomicString& value);
  void append(const AtomicString& name, Blob* blob, const AtomicString& file_name);

  std::vector<std::shared_ptr<const Entry>> entries_;
};

// Represents entry, which is a pair of a name and a value.
// https://xhr.spec.whatwg.org/#concept-formdata-entry
// Entry objects are immutable.
class FormData::Entry final {
 public:
  Entry(const AtomicString& name, const AtomicString& value);
  Entry(const AtomicString& name, Blob* blob, const AtomicString& filename);
  void Trace(GCVisitor*) const;

  bool IsString() const { return !blob_; }
  bool isFile() const { return blob_ != nullptr; }
  const AtomicString& name() const { return name_; }
  const AtomicString& Value() const { return value_; }
  Blob* GetBlob() const { return blob_.Get(); }
  const AtomicString& Filename() const { return filename_; }

 private:
  AtomicString name_;
  AtomicString value_;
  Member<Blob> blob_;
  AtomicString filename_;
};

template <>
struct DowncastTraits<FormData> {
  static bool AllowFrom(const BindingObject& binding_object) { return binding_object.IsFormData(); }
};

}  // namespace webf
#endif