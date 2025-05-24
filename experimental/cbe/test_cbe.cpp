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

//!!!!!!!!!!
#include <iostream>
//!!!!!!!!!!
#include <array>

#include <gtest/gtest.h>

#include "jmg/random.h"

#include "cbe.h"

using namespace jmg;
using namespace std;

namespace
{

// NOTE: each element n of the following array contains an unsigned 64
// bit integer value consisting of all 0s except for the first bit in
// the nth octet
constexpr array<uint64_t, 8> kOnesByByteSize = {
  1UL,       // 0th octet -> 1
  1UL << 8,  // 1st octet -> 256
  1UL << 16, // 2nd octet -> 65,536
  1UL << 24, // 3rd octet -> 16,777,216
  1UL << 32, // 4th octet -> 4,294,967,296
  1UL << 40, // 5th octet -> 1,099,511,627,776
  1UL << 48, // 6th octet -> 281,474,976,710,656
  1UL << 56  // 7th octet -> 72,057,594,037,927,936
};

namespace detail
{
template<ArithmeticT T>
struct CheckTypeSelect {
  using type = remove_cvref_t<T>;
};

template<>
struct CheckTypeSelect<float> {
  using type = uint32_t;
};

template<>
struct CheckTypeSelect<double> {
  using type = uint64_t;
};
} // namespace detail

template<ArithmeticT T>
using CheckTypeForT = detail::CheckTypeSelect<T>::type;

} // namespace

/**
 * use bitwise equality to compare floating point types to avoid
 * complexities around non-normal values such as NaN
 */
#define VERIFY_SAME_VALUE(flt_val1, flt_val2)                               \
  do {                                                                      \
    static_assert(decayedSameAs<decltype(flt_val1), decltype(flt_val2)>()); \
    using Decayed = remove_cvref_t<decltype(flt_val1)>;                     \
    using CheckType = CheckTypeForT<Decayed>;                               \
    CheckType check1;                                                       \
    memcpy(&check1, &(flt_val1), sizeof(check1));                           \
    CheckType check2;                                                       \
    memcpy(&check2, &(flt_val1), sizeof(check1));                           \
    EXPECT_EQ(check1, check2);                                              \
  } while (0)

/**
 * verify the results of the encode/decode as well as the number of
 * octets consumed
 */
#define VERIFY_ENCODE_DECODE(arg, expected_consumed)                    \
  do {                                                                  \
    const auto val = (arg);                                             \
    using Decayed = remove_cvref_t<decltype(val)>;                      \
    std::array<uint8_t, 11> buffer = {0};                               \
    const auto consumed_by_encoding = encode(buffer, (val));            \
    EXPECT_EQ(consumed_by_encoding, expected_consumed);                 \
    const auto [decoded, consumed_by_decoding] =                        \
      decode<Decayed>(span(buffer));                                    \
    if (FloatingPointT<Decayed>) { VERIFY_SAME_VALUE((val), decoded); } \
    else { EXPECT_EQ((val), decoded); }                                 \
    EXPECT_EQ(consumed_by_decoding, consumed_by_decoding);              \
  } while (0)

/**
 * verify the results of the encode/decode
 */
#define VERIFY_VAL(arg)                                                 \
  do {                                                                  \
    const auto val = (arg);                                             \
    using Decayed = remove_cvref_t<decltype(val)>;                      \
    std::array<uint8_t, 11> buffer = {0};                               \
    encode(buffer, (val));                                              \
    const auto [decoded, _] = decode<Decayed>(buffer);                  \
    if (FloatingPointT<Decayed>) { VERIFY_SAME_VALUE((val), decoded); } \
    else { EXPECT_EQ((val), decoded); }                                 \
  } while (0)

#if 1
TEST(CbeTest, TestUnsignedInts) {
  size_t expected_octets_consumed = 1;
  {
    const uint64_t zero_val = 0;
    VERIFY_ENCODE_DECODE(zero_val, 1);
  }
  for (const auto raw_val : kOnesByByteSize) {
    VERIFY_ENCODE_DECODE(raw_val, expected_octets_consumed);
    ++expected_octets_consumed;
    if (7 == expected_octets_consumed) {
      // add an extra octet at 7 since each encoded octet holds 7 bits
      // of data and 1 stop bit
      ++expected_octets_consumed;
    }
  }
}
#endif

TEST(CbeTest, TestSignedInts) {
  size_t expected_octets_consumed = 1;
  for (const auto raw_val : kOnesByByteSize) {
    const auto neg_val = -(static_cast<int64_t>(raw_val));
    VERIFY_ENCODE_DECODE(neg_val, expected_octets_consumed);
    ++expected_octets_consumed;
    if (7 == expected_octets_consumed) {
      // add an extra octet at 7 since each encoded octet holds 7 bits
      // of data and 1 stop bit
      ++expected_octets_consumed;
    }
  }
}

TEST(CbeTest, TestFloat32) {
  VERIFY_ENCODE_DECODE(0.0f, 2);
  // use copysign to force -0.0
  VERIFY_ENCODE_DECODE(copysign(0.0f, -1.0), 2);
  VERIFY_ENCODE_DECODE(numeric_limits<float>::max(), 5);
  VERIFY_ENCODE_DECODE(numeric_limits<float>::lowest(), 5);
  VERIFY_ENCODE_DECODE(numeric_limits<float>::min(), 2);
  VERIFY_ENCODE_DECODE(-numeric_limits<float>::min(), 2);
  VERIFY_ENCODE_DECODE(numeric_limits<float>::denorm_min(), 2);
  VERIFY_ENCODE_DECODE(-numeric_limits<float>::denorm_min(), 2);
  VERIFY_ENCODE_DECODE(numeric_limits<float>::quiet_NaN(), 5);
  VERIFY_ENCODE_DECODE(-numeric_limits<float>::quiet_NaN(), 5);
  VERIFY_ENCODE_DECODE(numeric_limits<float>::infinity(), 2);
  VERIFY_ENCODE_DECODE(-numeric_limits<float>::infinity(), 2);
}

TEST(CbeTest, TestFloat64) {
  VERIFY_ENCODE_DECODE(0.0, 2);
  // use copysign to force -0.0
  VERIFY_ENCODE_DECODE(copysign(0.0, -1.0), 2);
  VERIFY_ENCODE_DECODE(numeric_limits<double>::max(), 10);
  VERIFY_ENCODE_DECODE(numeric_limits<double>::lowest(), 10);
  VERIFY_ENCODE_DECODE(numeric_limits<double>::min(), 2);
  VERIFY_ENCODE_DECODE(-numeric_limits<double>::min(), 2);
  VERIFY_ENCODE_DECODE(numeric_limits<double>::denorm_min(), 2);
  VERIFY_ENCODE_DECODE(-numeric_limits<double>::denorm_min(), 2);
  VERIFY_ENCODE_DECODE(numeric_limits<double>::quiet_NaN(), 10);
  VERIFY_ENCODE_DECODE(-numeric_limits<double>::quiet_NaN(), 10);
  VERIFY_ENCODE_DECODE(numeric_limits<double>::infinity(), 3);
  VERIFY_ENCODE_DECODE(-numeric_limits<double>::infinity(), 3);
}

TEST(CbeTest, TestBatchEncodeFollowedByDecode) {
  std::array<uint8_t, 1024> buffer = {0};
  auto view = span(buffer);
  const uint32_t int32 = 20010911;
  const int64_t int64 = -20070625;
  const float flt32 = 42.0;
  const double flt64 = -1.0;
  {
    size_t idx = 0;
#define DO_ENCODE(val)                                      \
  do {                                                      \
    const auto consumed = encode(view.subspan(idx), (val)); \
    idx += consumed;                                        \
  } while (0)

    DO_ENCODE(int32);
    DO_ENCODE(int64);
    DO_ENCODE(flt32);
    DO_ENCODE(flt64);

#undef DO_ENCODE
  }
  {
    size_t idx = 0;
    auto view = span(buffer);
#define DO_DECODE_CHECK(val)                                    \
  do {                                                          \
    const auto [decoded, consumed] =                            \
      decode<remove_cvref_t<decltype(val)>>(view.subspan(idx)); \
    idx += consumed;                                            \
    EXPECT_EQ((val), decoded);                                  \
  } while (0)

    DO_DECODE_CHECK(int32);
    DO_DECODE_CHECK(int64);
    DO_DECODE_CHECK(flt32);
    DO_DECODE_CHECK(flt64);

#undef DO_DECODE_CHECK
  }
}

TEST(CbeTest, TestSingleString) {
  std::array<uint8_t, 1024> buffer = {0};
  const auto str = "foo"s;
  encode(span(buffer), str);
  const auto [decoded, consumed] = decode<string>(span(buffer));
  EXPECT_EQ(str, decoded);
}
