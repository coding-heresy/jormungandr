/** -*- mode: c++ -*-
 *
 * Copyright (C) 2024 Brian Davis
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

#if defined(USE_NATIVE_QUICKFIX_MSG)
#include <quickfix/Message.h>
#else
#include <ranges>
#endif

#include "jmg/conversion.h"
#include "jmg/field.h"
#include "jmg/object.h"
#include "jmg/preprocessor.h"
#include "jmg/types.h"
#include "jmg/util.h"

namespace jmg
{

namespace quickfix
{

constexpr auto kTimeStampFmt = TimePointFmt("%E4Y%m%d-%H:%M:%S");
constexpr auto kTimeStampMillisecFmt = TimePointFmt("%E4Y%m%d-%H:%M:%S.#E3f");
constexpr auto kDateOnlyFmt = TimePointFmt("%E4Y%m%d");

#if defined(USE_NATIVE_QUICKFIX_MSG)
template<typename... Fields>
class Object : public ObjectDef<Fields...> {
public:
  using adapted_type = FIX::Message;

  Object() = delete;
  explicit Object(const adapted_type& adapted) : adapted_(&adapted) {}

  /**
   * delegate for jmg::get()
   */
  template<RequiredField Fld>
  typename Fld::type get() const {
    return getImpl<Fld>();
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField Fld>
  std::optional<typename Fld::type> try_get() const {
    if (!adapted_->isSetField(Fld::kFixTag)) { return std::nullopt; }
    return getImpl<Fld>();
  }

private:
  /**
   * implementation for get/try_get
   */
  template<typename Fld>
  typename Fld::type getImpl() const
    requires ConversionTgtT<typename Fld::type>
  {
    const std::string& fieldRef = adapted_->getField(Fld::kFixTag);
    return static_cast<typename Fld::type>(from(fieldRef));
  }
  const adapted_type* adapted_;
};
#else

template<typename T>
concept HasFixTag =
  requires(T) { std::same_as<decltype(T::kFixTag), uint32_t>; };

template<typename T>
concept RequiredFixFieldT = RequiredField<T> && HasFixTag<T>;

template<typename T>
concept OptionalFixFieldT = OptionalField<T> && HasFixTag<T>;

/**
 * quick and dirty implementation of FIX message object used for
 * testing until the quitckfix library is fully integrated
 */
template<typename... Fields>
class Object : public ObjectDef<Fields...> {
public:
  Object() = delete;
  Object(const std::string_view msg,
         const Dict<unsigned, unsigned>& lengthFields) {
    constexpr std::string_view kFieldDelim = "";
    constexpr std::string_view kFieldSplitter = "=";

    auto stop = msg.find(kFieldDelim);
    decltype(stop) start = 0;
    while (stop != std::string::npos) {
      auto pos = msg.find(kFieldSplitter, start);
      JMG_ENFORCE(pos < stop, "missing field splitter '=' in field");
      unsigned tag = from(msg.substr(start, pos));
      {
        // check for special LENGTH field
        const auto entry = lengthFields.find(tag);
        if (entry != lengthFields.end()) {
          const auto pairedTag = value_of(*entry);
          const size_t nextSz = from(msg.substr(pos + 1, stop - pos - 1));
          // jump to the next field
          start = stop + 1;
          auto pos = msg.find(kFieldSplitter, start);
          JMG_ENFORCE(pos != std::string::npos,
                      "length field with tag ["
                        << tag
                        << "] was followed by data with no splitter "
                           "'='");
          const unsigned checkTag = from(msg.substr(start, pos));
          JMG_ENFORCE(checkTag == pairedTag,
                      "unpaired tag ["
                        << checkTag << "] followed length field with tag ["
                        << tag << "] instead of expected paired tag ["
                        << pairedTag << "]");
          stop = pos + nextSz;
          JMG_ENFORCE(stop < msg.size(),
                      "raw data field with tag ["
                        << pairedTag << "] had length [" << nextSz
                        << "] that was too long for the message");
          // overwrite the length tag so that only the raw data field
          // is stored in the dictionary
          tag = pairedTag;
        }
      }
      // store the data in the dictionary
      const auto [entry, inserted] =
        fields_.try_emplace(tag, msg.substr(pos + 1, stop - pos - 1));
      JMG_ENFORCE(inserted,
                  "encountered duplicate tag [" << tag << "] in message");

      if (msg.size() == stop) {
        // last character of the message should be a field delimiter
        break;
      }
      // find the next field
      start = stop + 1;
      stop = msg.find(kFieldDelim, start);
    }
  }

  /**
   * delegate for jmg::get()
   */
  template<RequiredFixFieldT Fld>
  typename Fld::type get() const {
    using Type = typename Fld::type;
    const auto entry = fields_.find(Fld::kFixTag);
    JMG_ENFORCE(fields_.end() != entry, "message had no value for required "
                                        "FIX field ["
                                          << Fld::name << "] (" << Fld::kFixTag
                                          << ")");
    const auto& str = value_of(*entry);
    if constexpr (std::same_as<Type, std::string>) { return std::string(str); }
    else {
      Type val = from(str);
      return val;
    }
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalFixFieldT Fld>
  std::optional<typename Fld::type> try_get() const {
    using Type = typename Fld::type;
    const auto entry = fields_.find(Fld::kFixTag);
    if (fields_.end() == entry) { return std::nullopt; }
    const auto& str = value_of(*entry);
    if constexpr (std::same_as<Type, std::string>) { return str; }
    else {
      Type val = from(str);
      return val;
    }
  }

private:
  Dict<uint32_t, std::string> fields_;
};
#endif

} // namespace quickfix
} // namespace jmg
