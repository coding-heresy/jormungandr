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

#include <cmath>
#include <cstring>

#include <algorithm>

#include "jmg/cbe/cbe.h"
#include "jmg/meta.h"
#include "jmg/preprocessor.h"

using namespace std;

namespace rng = std::ranges;

namespace jmg::cbe
{

namespace
{

constexpr uint8_t kMask = 127;
constexpr uint8_t kStopBit = (1 << 7);
constexpr uint8_t kSignBit = 1;

} // namespace

////////////////////////////////////////////////////////////////////////////////
// encode/decode for integers
////////////////////////////////////////////////////////////////////////////////

namespace detail
{

/**
 * type metafunction that converts integers to the unsigned type of
 * the equivalent size
 */
template<jmg::IntegralT T>
struct UnsignifyInt {
  using type = T;
};

#define UNSIGNIFY(sz)                \
  template<>                         \
  struct UnsignifyInt<int##sz##_t> { \
    using type = uint##sz##_t;       \
  }

UNSIGNIFY(8);
UNSIGNIFY(16);
UNSIGNIFY(32);
UNSIGNIFY(64);

#undef UNSIGNIFY

} // namespace detail

namespace
{

template<jmg::IntegralT T>
using Unsignify = detail::UnsignifyInt<T>::type;

} // namespace

template<IntegralT T>
size_t encodeInt(BufferProxy tgt,
                 const T src,
                 const bool use_forced_sign_bit = false) {
  JMG_ENFORCE(!tgt.empty(), "attempted to encode output to empty array/vector");
  using Tgt = Unsignify<T>;

  // NOTE: yes, this is a C-style cast
  Tgt val = (Tgt)src;

  // short-circuit for value 0
  if (!val) {
    tgt[0] = kStopBit;
    if (use_forced_sign_bit) { tgt[0] |= kSignBit; }
    return 1;
  }

  // assuming that the architecture is using twos complement encoding
  // for integers, move the sign bit from the end to the beginning to
  // avoid having negative numbers unnecessarily extend the number of
  // non-zero octets

  // convert negative values to positive
  if (src < 0) {
    val = -val;
    // NOTE: it is an invalid state for the forced sign bit to be set
    // when the number to be encoded is negative
    JMG_ENFORCE_USING(logic_error, !use_forced_sign_bit,
                      "detected logic error or memory corruption: forced sign "
                      "bit was set with negative value to encode");
  }
  val <<= 1;

  // NOTE: this handles the case where the fractional portion of a
  // floating point number is being encoded as a signed integer and
  // the fractional portion is 0 due to the following cases:
  //
  // 1. the floating point number is actually negative but the
  //    encoding has an implicit leading 1.
  // 2. the floating point number is some version of -NaN
  if ((src < 0) || use_forced_sign_bit) { val |= kSignBit; }

  // generate stop-bit encoded version of value
  size_t idx = 0;
  while (val) {
    JMG_ENFORCE(idx < tgt.size(), detail::kEncodingFailure,
                " to encode the value [", src, "]");
    uint8_t chunk = val & kMask;
    val >>= 7;
    if (!val) { chunk |= kStopBit; }
    tgt[idx] = chunk;
    ++idx;
  }
  return idx;
}

template<IntegralT T>
tuple<T, size_t, bool> decodeInt(BufferView src,
                                 const bool use_forced_sign_bit = false) {
  T rslt = 0;
  size_t counter = 0;
  for (auto chunk : src) {
    const auto is_finished = static_cast<bool>(chunk & kStopBit);
    chunk &= ~kStopBit;
    T val = chunk;
    val <<= (7 * counter);
    rslt += val;
    ++counter;
    if (is_finished) { break; }
    JMG_ENFORCE(counter < src.size(), detail::kDecodingFailure);
  }
  const bool is_sign_bit_set = rslt & 1;
  rslt >>= 1;
  if (!use_forced_sign_bit && is_sign_bit_set && (0 != rslt)) { rslt = -rslt; }
  const bool has_forced_sign_bit =
    (is_sign_bit_set && ((0 == rslt) || use_forced_sign_bit));
  return make_tuple(rslt, counter, has_forced_sign_bit);
}

////////////////////////////////////////////////////////////////////////////////
// encode/decode for floating point numbers
////////////////////////////////////////////////////////////////////////////////

////////////////////
// IEEE 754

// single precision (32 bits) has 1 sign bit (position 31), 8 exponent
// bits (positions 23 through 30) and 23 fraction bits (positions 0
// through 22)

// double precision (64 bits) has 1 sign bit (position 63), 11
// exponent bits (positions 52 through 62) and 52 fraction bit
// (positions 0 through 51)

namespace detail
{

template<FloatingPointT T>
struct OctetsTypeSelect {
  using Type = uint32_t;
};

template<>
struct OctetsTypeSelect<double> {
  using Type = uint64_t;
};

template<FloatingPointT T>
struct ExpTypeSelect {
  using Type = uint8_t;
};

template<>
struct ExpTypeSelect<double> {
  using Type = uint16_t;
};

template<FloatingPointT T>
struct FracTypeSelect {
  using Type = int32_t;
};

template<>
struct FracTypeSelect<double> {
  using Type = int64_t;
};

} // namespace detail

namespace
{

template<FloatingPointT T>
using OctetsTypeFor = detail::OctetsTypeSelect<T>::Type;

template<FloatingPointT T>
using ExpTypeFor = detail::ExpTypeSelect<T>::Type;

template<FloatingPointT T>
using FracTypeFor = detail::FracTypeSelect<T>::Type;

template<FloatingPointT T = float>
struct Masks {
  using UType = OctetsTypeFor<T>;
  static constexpr size_t kExpOffset = 23;
  static constexpr UType kSignMask = UType(2147483648U);
  static constexpr UType kExpMask = UType(2139095040U);
  static constexpr UType kFracMask = UType(8388607U);
};

template<>
struct Masks<double> {
  using UType = OctetsTypeFor<double>;
  static constexpr size_t kExpOffset = 52;
  static constexpr UType kSignMask = UType(9223372036854775808UL);
  static constexpr UType kExpMask = UType(9218868437227405312UL);
  static constexpr UType kFracMask = UType(4503599627370495UL);
};

} // namespace

template<FloatingPointT T>
size_t encodeFlt(BufferProxy tgt, const T src) {
  using OctetsType = OctetsTypeFor<T>;
  using ExpType = ExpTypeFor<T>;
  using FracType = FracTypeFor<T>;

  const bool is_abnormal = isnan(src) || isinf(src);

  // TODO(bd) is this type punning safe across all architectures?
  const OctetsType* pun = (const OctetsType*)&src;

  // encode and store the exponent first
  const ExpType exp = [&] {
    OctetsType full = *pun & Masks<T>::kExpMask;
    full >>= Masks<T>::kExpOffset;
    return static_cast<ExpType>(full);
  }();

  size_t idx = 0;
  if constexpr (SameAsDecayedT<float, T>) {
    // exponent is exactly 8 bits, no need for stop bit encoding
    tgt[idx] = static_cast<uint8_t>(exp);
    ++idx;
    JMG_ENFORCE(idx < tgt.size(), detail::kEncodingFailure);
  }
  else {
    // exponent is larger than 8 bits, encode it using the standard
    // encoding for the type equivalent to its size
    idx += impl::encode(tgt.subspan(idx), exp);
  }

  // encode the fraction using the standard encoding its equivalent
  // type
  const FracType fraction = [&] {
    OctetsType full = *pun & Masks<T>::kFracMask;
    auto frac = static_cast<FracType>(full);
    return (src < 0) ? -frac : frac;
  }();
  // NOTE: fraction can be 0 even when overall floating point value is
  // negative due to negative 0 or implicit 1. in the encoding
  // NOTE: copysign handles the case where the value is -0.0
  const auto use_forced_sign_bit =
    ((copysign(1.0, src) < 0) && ((0 == fraction) || is_abnormal));
  idx += encodeInt(tgt.subspan(idx), fraction, use_forced_sign_bit);
  return idx;
}

template<FloatingPointT T>
tuple<T, size_t> decodeFlt(BufferView src) {
  using OctetsType = OctetsTypeFor<T>;
  using ExpType = ExpTypeFor<T>;
  using FracType = FracTypeFor<T>;

  size_t idx = 0;

  // decode the exponent
  const OctetsType exp = [&]() -> OctetsType {
    if constexpr (SameAsDecayedT<float, T>) {
      // exponent is exactly 8 bits, no need for stop bit encoding
      idx += 1;
      return static_cast<OctetsType>(src[0]) << Masks<T>::kExpOffset;
    }
    // exponent is larger than 8 bits, decode it using the standard
    // encoding for the type equivalent to its size
    const auto [raw, consumed] = impl::decode<ExpType>(src);
    idx += consumed;
    OctetsType rslt = static_cast<OctetsType>(raw);
    rslt <<= Masks<T>::kExpOffset;
    return rslt;
  }();

  // NOTE: if all exponent bits are set, then the floating point value
  // is some form of Inf or NaN, in which case the fractional value is
  // always positive even though the trailing sign bit is set and this
  // information must be communicated to the decoder function
  const auto use_forced_sign_bit = (exp == Masks<T>::kExpMask);

  // decode the fraction
  auto [fraction, consumed, has_forced_sign_bit] =
    decodeInt<FracType>(src.subspan(idx), use_forced_sign_bit);
  idx += consumed;

  const auto is_fraction_negative = (fraction < 0);
  // NOTE: fraction should never be negative if forced sign bit is set
  JMG_ENFORCE_USING(
    logic_error, !(is_fraction_negative && has_forced_sign_bit),
    "detected logic error or memory corruption: forced sign bit was set when "
    "floating point fraction value was negative");
  const auto is_result_negative = (is_fraction_negative || has_forced_sign_bit);
  if (is_fraction_negative) { fraction = -fraction; }
  OctetsType pun = exp | static_cast<OctetsType>(fraction);
  T rslt = 0;
  memcpy(&rslt, &pun, sizeof(rslt));
  if (is_result_negative) { rslt = -rslt; }
  return make_tuple(rslt, idx);
}

////////////////////////////////////////////////////////////////////////////////
// encode/decode for strings
////////////////////////////////////////////////////////////////////////////////

namespace detail
{

size_t encodeStr(BufferProxy tgt, const string_view src) {
  auto consumed = encodeInt(tgt.subspan(0), src.size());
  rng::copy(src, tgt.subspan(consumed).begin());
  return consumed + src.size();
}

tuple<string, size_t> decodeStr(BufferView src) {
  const auto [sz, consumed, _] = decodeInt<size_t>(src);
  string rslt;
  rslt.reserve(sz);
  rng::copy(src.subspan(consumed), back_inserter(rslt));
  rslt.push_back('\0');
  rslt.resize(sz);
  return make_tuple(rslt, consumed + sz);
}

} // namespace detail

////////////////////////////////////////////////////////////////////////////////
// dispatch based on type via encodePrimitive/decodePrimitive
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
size_t encodePrimitive(BufferProxy tgt, T src) {
  if constexpr (IntegralT<T>) { return encodeInt(tgt, src); }
  else if constexpr (FloatingPointT<T>) { return encodeFlt(tgt, src); }
  else if constexpr (NonViewStringT<T>) {
    return encodePrimitive(tgt, string_view(src));
  }
  else if constexpr (SameAsDecayedT<std::string_view, T>) {
    return encodeStr(tgt, src);
  }
  else { JMG_NOT_EXHAUSTIVE(T); }
}
template<typename T>
std::tuple<T, size_t> decodePrimitive(BufferView src) {
  if constexpr (IntegralT<T>) {
    const auto [rslt, consumed, has_forced_sign_bit] = decodeInt<T>(src);
    // NOTE: has_forced_sign_bit should never be set when decoding an
    // integer
    JMG_ENFORCE_USING(logic_error, !has_forced_sign_bit,
                      "detected logic error or memory corruption: forced sign "
                      "bit was set when decoding an integer");
    return make_tuple(rslt, consumed);
  }
  else if constexpr (FloatingPointT<T>) { return decodeFlt<T>(src); }
  // NOTE: decode must produce an owning string
  else if constexpr (SameAsDecayedT<string, T>) { return decodeStr(src); }
  else { JMG_NOT_EXHAUSTIVE(T); }
}
} // namespace detail

////////////////////////////////////////////////////////////////////////////////
// explicit instantiation/specialization
////////////////////////////////////////////////////////////////////////////////

// NOTE: there are only a limited set of possible specializations of
// encode and decode for primitive types, so just generate them all
// here. This should reduce compile times

#define INSTANTIATE(type)                                           \
  template size_t detail::encodePrimitive<type>(BufferProxy, type); \
  template tuple<type, size_t> detail::decodePrimitive<type>(BufferView)

INSTANTIATE(int16_t);
INSTANTIATE(uint16_t);
INSTANTIATE(int32_t);
INSTANTIATE(uint32_t);
INSTANTIATE(int64_t);
INSTANTIATE(uint64_t);
INSTANTIATE(float);
INSTANTIATE(double);

#undef INSTANTIATE

// TODO(bd) eliminate requirement for encoding o by-value string
template size_t detail::encodePrimitive<string>(BufferProxy, string);
template size_t detail::encodePrimitive<const string&>(BufferProxy,
                                                       const string&);
template size_t detail::encodePrimitive<string_view>(BufferProxy, string_view);
// TODO(bd) instantiate encoding support for C-style strings?
template tuple<string, size_t> detail::decodePrimitive<string>(BufferView);

} // namespace jmg::cbe
