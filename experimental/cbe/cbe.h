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

#include <string_view>

#include "jmg/meta.h"
#include "jmg/preprocessor.h"
#include "jmg/types.h"

// TODO(bd) rework this to avoid function names with embedded type
// specifiers

namespace jmg
{

namespace detail
{
size_t encode(BufferProxy tgt, const std::string_view src);
// TODO(bd) this should support traditional and polymorphic allocator
// flavors for std::basic_string
std::tuple<std::string, size_t> decodeStr(BufferView src);
} // namespace detail

////////////////////
// numeric/arithmetic values

/**
 * encode numeric/arithmetic value
 *
 * @return the number of octets of the buffer that were consumed
 */
template<ArithmeticT T>
size_t encode(BufferProxy tgt, T src);

/**
 * decode numeric/arithmetic value
 *
 * @return a 2-tuple consisting of the decoded value and the number of
 *         octets consumed
 */
template<StringLikeT T>
std::tuple<T, size_t> decode(BufferView src) {
  return detail::decodeStr(src);
}

////////////////////
// string values

/**
 * encode string value
 *
 * @return the number of octets of the buffer that were consumed
 */
template<StringLikeT T>
size_t encode(BufferProxy tgt, T src) {
  if constexpr (sameAsDecayed<std::string_view, T>) {
    return detail::encode(tgt, src);
  }
  else { return encode(tgt, std::string_view(src)); }
}

/**
 * decode numeric/arithmetic value
 *
 * @return a 2-tuple consisting of the decoded value and the number of
 *         octets consumed
 */
template<ArithmeticT T>
std::tuple<T, size_t> decode(BufferView src);

////////////////////
// decode directly to variable

/**
 * decode any supported value directly to variable
 *
 * @return a 2-tuple consisting of the decoded value and the number of
 *         octets consumed
 */
template<typename T>
size_t decode_to(BufferView src, T& tgt) {
  const auto [decoded, consumed] = decode<std::remove_cvref_t<T>>(src);
  tgt = decoded;
  return consumed;
}

////////////////////
// encoder/decoder objects

/**
 * simple encoder class that manages the index into a fixed-size
 * buffer as items are encoded into it
 */
class Encoder {
public:
  JMG_NON_COPYABLE(Encoder);
  JMG_NON_MOVEABLE(Encoder);
  Encoder() = delete;
  ~Encoder() = default;

  Encoder(BufferProxy buffer) : buffer_(std::move(buffer)) {}

  template<ArithmeticT T>
  void encode(const T src) {
    idx_ += encode(buffer_, src);
  }

  template<StringLikeT T>
  void encode(const T& src) {
    if constexpr (decayedSameAs<std::string_view, T>) {
      idx_ += detail::encode(buffer_, src);
    }
    else { idx_ += detail::encode(buffer_, std::string_view(src)); }
  }

private:
  size_t idx_ = 0;
  BufferProxy buffer_;
};

/**
 * simple encoder class that manages the index into a fixed-size
 * buffer as items are decoded from it
 */
class Decoder {
public:
  JMG_NON_COPYABLE(Decoder);
  JMG_NON_MOVEABLE(Decoder);
  Decoder() = delete;
  ~Decoder() = default;

  Decoder(BufferView buffer) : buffer_(std::move(buffer)) {}

  template<typename T>
  T decode() {
    auto [decoded, consumed] = decode<T>(buffer_);
    idx_ += consumed;
    return decoded;
  }

  template<typename T>
  void decode_to(T& tgt) {
    idx_ += decode_to(buffer_, tgt);
  }

private:
  size_t idx_ = 0;
  BufferView buffer_;
};

} // namespace jmg
