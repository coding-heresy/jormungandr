/** -*- mode: c++ -*-
 *
 * Copyright (C) 2025 Brian Davis
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Brian Davis <brian8702@sbcglobal.net>
 *
 */
#pragma once

/**
 * Compressed Binary Encoding
 *
 * This is a binary encoding format whose design is inspired by
 * protocol buffers and the encoding used by the FIX-FAST market data
 * transmission protocol. As with those two formats, some fixed length
 * binary values are encoded using a variable length to save
 * space. This can be thought of as 'structural compression',
 * i.e. compression based on characteristics of the data, and can be
 * contrasted with 'generic compression', where some algorithm
 * compresses large blocks of binary data without regard to its
 * structure.
 *
 * One goal of this algorithm is to achieve betters compression than
 * the protocol buffers varint and FIX-FAST stop-bit algorithms by
 * also compressing floating point data using its structural
 * characteristics
 */

#include <exception>
#include <string_view>

#include "jmg/meta.h"
#include "jmg/native/native.h"
#include "jmg/object.h"
#include "jmg/preprocessor.h"
#include "jmg/types.h"

namespace jmg::cbe
{
namespace detail
{
struct CbeFieldDefTag {};
} // namespace detail

/**
 * class template for field definitions that are specific to CBE
 * objects
 *
 * TODO(bd) use safe type instead of uint32_t for field ID?
 */
template<typename T, StrLiteral kName, TypeFlagT IsRequired, uint32_t kFieldId>
struct FieldDef : jmg::FieldDef<T, kName, IsRequired>, detail::CbeFieldDefTag {
public:
  static constexpr auto kFldId = kFieldId;
};

namespace detail
{
// TODO(bd) use safe type instead of uint32_t for field ID?
template<typename T>
concept HasCbeFieldId =
  requires { requires std::same_as<Decay<decltype(T::kFldId)>, uint32_t>; };
} // namespace detail

/**
 * concept for CBE field definition
 */
template<typename T>
concept CbeFieldDefT =
  jmg::FieldDefT<T> && std::derived_from<T, detail::CbeFieldDefTag>
  && detail::HasCbeFieldId<T>;

/**
 * class template for object associated with CBE encoded data
 *
 * NOTE: these objects are backed by the native (i.e. tuple-based)
 * encoding
 */
template<CbeFieldDefT... Flds>
class Object : public native::Object<Flds...> {
  using base = native::Object<Flds...>;

  /**
   * compile time helper function template that calculates the maximum
   * value for a field ID
   */
  template<size_t kFldIdx = 0>
  static constexpr size_t getMaxFieldId() {
    using Fld = meta::at<Fields, meta::size_t<kFldIdx>>;
    constexpr auto id = static_cast<size_t>(Fld::kFldId);
    if constexpr ((kFldIdx + 1) < Fields::size()) {
      return std::max(id, getMaxFieldId<kFldIdx + 1>());
    }
    else { return id; }
  }

  /**
   * compile time helper function template that determines whether
   * there are duplicate field IDs in the list of fields
   */
  template<uint32_t kFieldId = 0, size_t kFldIdx = 0>
  static constexpr bool hasDuplicateFieldIds() {
    if constexpr (kFldIdx == Fields::size()) { return false; }
    else {
      using Fld = meta::at<Fields, meta::size_t<kFldIdx>>;
      constexpr auto id = static_cast<size_t>(Fld::kFldId);
      // NOTE: ignore kFieldId for index 0
      if constexpr ((kFldIdx != 0) && (kFieldId == id)) { return true; }
      else {
        return
          // verify that there are no duplicates of kFieldId further
          // down the list of fields
          hasDuplicateFieldIds<kFieldId, kFldIdx + 1>() ||
          // verify that the ID of the current field is not duplicated
          // further down the list of fields
          hasDuplicateFieldIds<id, kFldIdx + 1>();
      }
    }
  }

public:
  using Fields = base::Fields;
  using adapted_type = base::adapted_type;
  static constexpr auto kMaxFieldId = getMaxFieldId();

  Object() = default;
  template<typename... Args>
  explicit Object(Args&&... args) : base(std::forward<Args>(args)...) {
    static_assert(!hasDuplicateFieldIds(),
                  "there are one or more duplicated field IDs in the "
                  "specialization of this object");
  }

  /**
   * delegate for jmg::get()
   */
  template<RequiredField FldT>
  decltype(auto) get() const {
    return static_cast<const base*>(this)->template get<FldT>();
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField FldT>
  decltype(auto) try_get() const {
    return static_cast<const base*>(this)->template try_get<FldT>();
  }

  // TODO(bd) replace these with set() member functions
  adapted_type& getWrapped() { return static_cast<base*>(this)->getWrapped(); }
  const adapted_type& getWrapped() const {
    return static_cast<const base*>(this)->getWrapped();
  }
};

namespace detail
{
template<typename T>
struct IsCbeObjectDef : std::false_type {};
template<CbeFieldDefT... Flds>
struct IsCbeObjectDef<cbe::Object<Flds...>> : std::true_type {};
} // namespace detail

template<typename T>
concept CbeObjectDefT = ObjectDefT<T> && detail::IsCbeObjectDef<T>{}();

namespace detail
{

template<typename T>
size_t encodePrimitive(BufferProxy tgt, T src);

template<typename T>
std::tuple<T, size_t> decodePrimitive(BufferView src);

} // namespace detail

// TODO(bd) see if this can be moved to the detail namespace
namespace impl
{
template<typename T>
size_t encode(BufferProxy tgt, T src) {
  if constexpr (SafeT<T>) { return encode(tgt, unsafe(src)); }
  if constexpr (OptionalT<T>) {
    // NOTE: std::optional<T> should be handled above this level
    static_assert(always_false<T>, "trying to encode std::optional");
  }
  // TODO(bd) arrays and objects
  else { return detail::encodePrimitive(tgt, src); }
}

template<typename T>
std::tuple<T, size_t> decode(BufferView src) {
  if constexpr (SafeT<T>) {
    const auto& [decoded, consumed] = decode<UnsafeTypeFromT<T>>(src);
    return std::make_tuple(T(decoded), consumed);
  }
  if constexpr (OptionalT<T>) {
    // NOTE: std::optional<T> should be handled above this level
    static_assert(always_false<T>, "trying to decode std::optional");
  }
  // TODO(bd) arrays and objects
  else { return detail::decodePrimitive<T>(src); }
}

} // namespace impl

////////////////////
// serializer and deserializer

/**
 * class that serializes objects to a buffer
 */
template<CbeObjectDefT Obj>
class Serializer {
  template<CbeFieldDefT Fld, typename T>
  void encodeValue(const T& val) {
    // encode the field ID
    constexpr auto id = Fld::kFldId;
    idx_ += impl::encode(buffer_.subspan(idx_), id);
    // encode the value
    idx_ += impl::encode(buffer_.subspan(idx_), val);
  }

  // TODO(bd) ensure that all fields have a cbe field ID and that all
  // IDs are unique
  template<CbeFieldDefT Fld>
  void encodeField(const Obj& object) {
    // get the field ID
    // using Required = typename Fld::required;
    constexpr bool is_required_field = typename Fld::required{};
    if constexpr (is_required_field) {
      encodeValue<Fld>(jmg::get<Fld>(object));
    }
    else {
      decltype(auto) val = jmg::try_get<Fld>(object);
      if (val) { encodeValue<Fld>(*val); }
    }
  }

public:
  explicit Serializer(BufferProxy buffer) : buffer_(buffer) {}

  template<FieldDefT... Flds>
  void serialize(const cbe::Object<Flds...>& object) {
    (encodeField<Flds>(object), ...);
  }

  size_t consumed() const { return idx_; }

private:
  size_t idx_ = 0;
  BufferProxy buffer_;
};

/**
 * class that deserializes objects from a buffer
 */
template<CbeObjectDefT Obj>
class Deserializer {
  static constexpr auto kMaxFieldId = Obj::kMaxFieldId;

  using FieldDecoders =
    std::array<std::optional<std::function<void(Obj&)>>, kMaxFieldId + 1>;
  using RequiredFieldFlags = std::array<bool, kMaxFieldId + 1>;

  /**
   * construct a set of field decoder objects, which decode the field
   * based on its FieldId
   *
   * NOTE: this function is complicated by the fact that the index of
   * the field in the underlying tuple is currently needed to
   * determine the location to store the value to, but the decoder
   * itself is stored using the field ID as an index into the array of
   * decoders to avoid the need for a lookup from the field ID to the
   * field index
   *
   * TODO(bd) recode this with a jmg::set() operation against the
   * object to avoid the need to embed the mapping to the index here
   */
  template<size_t kFldIdx>
  void makeFieldDecoders(FieldDecoders& decoders) {
    using Fields = typename Obj::Fields;
    using Fld = meta::at<Fields, meta::size_t<kFldIdx>>;
    constexpr auto id = static_cast<size_t>(Fld::kFldId);
    decoders[id] = [this](Obj& obj) {
      using Tgt = RemoveOptionalT<Decay<decltype(std::get<kFldIdx>(
        std::declval<typename Obj::adapted_type>()))>>;
      auto& tgt = std::get<kFldIdx>(obj.getWrapped());
      const auto [decoded, consumed] = impl::decode<Tgt>(buffer_.subspan(idx_));
      idx_ += consumed;
      tgt = decoded;
    };
    if constexpr (kFldIdx + 1 < Fields::size()) {
      makeFieldDecoders<kFldIdx + 1>(decoders);
    }
  }

  /**
   * populate an a array of flags based on field indices where each
   * flag is set if the corresponding field is required and cleared
   * otherwise
   */
  template<size_t kFldIdx = 0>
  static void setRequiredFieldFlags(RequiredFieldFlags& required) {
    using Fields = typename Obj::Fields;
    using Fld = meta::at<Fields, meta::size_t<kFldIdx>>;
    constexpr auto id = static_cast<size_t>(Fld::kFldId);
    constexpr bool is_required = RequiredField<Fld>;
    required[id] = is_required;
    if constexpr (kFldIdx + 1 < Fields::size()) {
      setRequiredFieldFlags<kFldIdx + 1>(required);
    }
  }

  /**
   * decoding work actually happens here
   */
  template<CbeFieldDefT... Flds>
  void decodeFields(cbe::Object<Flds...>& object) {
    //////////
    // set up function scope static helper objects
    static const auto kFieldDecoders = [&] {
      FieldDecoders rslt;
      makeFieldDecoders<0>(rslt);
      return rslt;
    }();
    static const auto kRequiredFields = []() -> RequiredFieldFlags {
      RequiredFieldFlags rslt = {false};
      setRequiredFieldFlags(rslt);
      return rslt;
    }();
    static const auto kRequiredCount = [&]() -> size_t {
      // TODO(bd) update to c++23 and use std::ranges::fold_left
      size_t rslt = 0;
      for (const auto required : kRequiredFields) {
        if (required) { ++rslt; }
      }
      return rslt;
    }();
    //////////
    // done setting up function scope static helper objects

    //////////
    // decoding logic
    size_t required_fields_deserialized = 0;
    while (!isBufferEmpty()) {
      const auto field_id = [&]() -> size_t {
        const auto [decoded, consumed] =
          impl::decode<uint32_t>(buffer_.subspan(idx_));
        idx_ += consumed;
        return static_cast<size_t>(decoded);
      }();
      JMG_ENFORCE(
        field_id < kMaxFieldId,
        "decoded field ID ["
          << field_id
          << "] is not in the set of valid IDs for the type being decoded");
      JMG_ENFORCE(!isBufferEmpty(), "ran out of data after deserializing field "
                                    "ID and before deserializing type");
      auto& decoder = kFieldDecoders[field_id];
      JMG_ENFORCE(
        pred(decoder),
        "decoded field ID ["
          << field_id
          << "] is not in the set of valid IDs for the type being decoded");
      (*decoder)(object);
      if (kRequiredFields[field_id]) { ++required_fields_deserialized; }
    }
    JMG_ENFORCE(required_fields_deserialized == kRequiredCount,
                "ran out of data after deserializing only ["
                  << required_fields_deserialized << "] of [" << kRequiredCount
                  << "] required fields");
  }

public:
  explicit Deserializer(BufferProxy buffer) : buffer_(buffer) {}

  /**
   * deserialize a single object from the buffer
   */
  Obj deserialize() {
    Obj rslt;
    decodeFields(rslt);
    return rslt;
  }

  size_t consumed() const { return idx_; }

private:
  bool isBufferEmpty() const { return idx_ >= buffer_.size(); }

  size_t idx_ = 0;
  BufferProxy buffer_;
};

} // namespace jmg::cbe
