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

#include <array>

#include <gmock/gmock.h>

#include "jmg/cbe/cbe.h"
#include "jmg/native/native.h"
#include "jmg/random.h"
#include "jmg/util.h"

using namespace jmg;
using namespace jmg::cbe;
using namespace std;
using ::testing::ElementsAreArray;

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

// buffer large enough to hold any numeric value
using NumericBuf = array<uint8_t, 11>;

} // namespace

/**
 * use bitwise equality to compare floating point types to avoid
 * complexities around non-normal values such as NaN
 */
#define VERIFY_SAME_VALUE(flt_val1, flt_val2)                              \
  do {                                                                     \
    static_assert(DecayedSameAsT<decltype(flt_val1), decltype(flt_val2)>); \
    using Decayed = remove_cvref_t<decltype(flt_val1)>;                    \
    using CheckType = CheckTypeForT<Decayed>;                              \
    CheckType check1;                                                      \
    memcpy(&check1, &(flt_val1), sizeof(check1));                          \
    CheckType check2;                                                      \
    memcpy(&check2, &(flt_val1), sizeof(check1));                          \
    EXPECT_EQ(check1, check2);                                             \
  } while (0)

/**
 * verify the results of the encode/decode as well as the number of
 * octets consumed
 */
#define VERIFY_ENCODE_DECODE(arg, expected_consumed)                 \
  do {                                                               \
    const auto val = (arg);                                          \
    using Decayed = remove_cvref_t<decltype(val)>;                   \
    NumericBuf buf = {0};                                            \
    const auto consumed_by_encoding = cbe::impl::encode(buf, (val)); \
    EXPECT_EQ(consumed_by_encoding, expected_consumed);              \
    const auto [decoded, consumed_by_decoding] =                     \
      cbe::impl::decode<Decayed>(span(buf));                         \
    if constexpr (FloatingPointT<Decayed>) {                         \
      VERIFY_SAME_VALUE((val), decoded);                             \
    }                                                                \
    else { EXPECT_EQ((val), decoded); }                              \
    EXPECT_EQ(consumed_by_decoding, consumed_by_decoding);           \
  } while (0)

/**
 * verify the results of the encode/decode
 */
#define VERIFY_VAL(arg)                                                 \
  do {                                                                  \
    const auto val = (arg);                                             \
    using Decayed = remove_cvref_t<decltype(val)>;                      \
    NumericBuf buf = {0};                                               \
    cbe::impl::encode(buf, (val));                                      \
    const auto [decoded, _] = cbe::impl::decode<Decayed>(buf);          \
    if (FloatingPointT<Decayed>) { VERIFY_SAME_VALUE((val), decoded); } \
    else { EXPECT_EQ((val), decoded); }                                 \
  } while (0)

TEST(CbeTest, TestUnsignedInts) {
  size_t expected_octets_consumed = 1;
  VERIFY_ENCODE_DECODE(0, 1);
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

TEST(CbeTest, TestSafeTypes) {
  using SafeId = SafeId32<>;
  const auto id = SafeId(20010911);
  NumericBuf buf = {0};
  {
    const auto consumed = cbe::impl::encode(buf, id);
    EXPECT_EQ(consumed, 4);
  }
  const auto [decoded, consumed] = cbe::impl::decode<SafeId>(span(buf));
  EXPECT_EQ(consumed, 4);
  EXPECT_EQ(decoded, id);
}

TEST(CbeTest, TestBatchEncodeFollowedByDecode) {
  array<uint8_t, 1024> buf = {0};
  auto view = span(buf);
  const uint32_t int32 = 20010911;
  const int64_t int64 = -20070625;
  const float flt32 = 42.0;
  const double flt64 = -1.0;
  {
    size_t idx = 0;
#define DO_ENCODE(val)                                                 \
  do {                                                                 \
    const auto consumed = cbe::impl::encode(view.subspan(idx), (val)); \
    idx += consumed;                                                   \
  } while (0)

    DO_ENCODE(int32);
    DO_ENCODE(int64);
    DO_ENCODE(flt32);
    DO_ENCODE(flt64);

#undef DO_ENCODE
  }
  {
    size_t idx = 0;
    auto view = span(buf);
#define DO_DECODE_CHECK(val)                                               \
  do {                                                                     \
    const auto [decoded, consumed] =                                       \
      cbe::impl::decode<remove_cvref_t<decltype(val)>>(view.subspan(idx)); \
    idx += consumed;                                                       \
    EXPECT_EQ((val), decoded);                                             \
  } while (0)

    DO_DECODE_CHECK(int32);
    DO_DECODE_CHECK(int64);
    DO_DECODE_CHECK(flt32);
    DO_DECODE_CHECK(flt64);

#undef DO_DECODE_CHECK
  }
}

// TODO(bd) add exhaustive tests to show that an exception is thrown if the
// buffer is too short for encoding or decoding

TEST(CbeTest, TestSingleString) {
  array<uint8_t, 1024> buf = {0};
  const auto str = "foo"s;
  cbe::impl::encode(span(buf), str);
  const auto [decoded, consumed] = cbe::impl::decode<string>(span(buf));
  EXPECT_EQ(str, decoded);
}

TEST(CbeTest, TestArray) {
  array<uint8_t, 1024> buf = {0};
  const auto vec = vector{1, 2, 3};
  cbe::impl::encode(span(buf), vec);
  const auto [decoded, consumed] = cbe::impl::decode<vector<int>>(span(buf));
  EXPECT_THAT(decoded, ElementsAreArray(vec));
}

TEST(CbeTest, TestObj) {
  array<uint8_t, 1024> buf = {0};
  using IntFld = cbe::FieldDef<int, "int", Required, 0U /* kFldId */>;
  using SubObject = cbe::Object<IntFld>;
  // NOTE: IntFld is in a different object than SubObjFld so kFldI can be the
  // same for both
  using SubObjFld =
    cbe::FieldDef<SubObject, "sub_obj", Required, 0U /* kFldId */>;
  using TestObject = cbe::Object<SubObjFld>;
  const auto obj = TestObject(SubObject(20010911));
  {
    const auto consumed = cbe::impl::encode(span(buf), obj);
    EXPECT_EQ(14U, consumed);
  }
  const auto [decoded, consumed] = cbe::impl::decode<TestObject>(span(buf));
  EXPECT_EQ(14U, consumed);
  const auto& sub_obj = jmg::get<SubObjFld>(decoded);
  EXPECT_EQ(20010911, jmg::get<IntFld>(sub_obj));
}

TEST(CbeTest, TestSerializerAndDeserializer) {
  using IntFld = cbe::FieldDef<int, "int", Required, 0U /* kFldId */>;
  using DblFld = cbe::FieldDef<double, "dbl", Required, 1U /* kFldId */>;
  using StrFld = cbe::StringField<"str", Required, 2U /* kFldId */>;
  // NOTE: intentionally skipping field ID 3
  using OptFld = cbe::FieldDef<float, "opt", Optional, 4U /* kFldId */>;
  using ArrayFld =
    cbe::ArrayField<unsigned, "unsigned_array", Required, 5U /* kFldId */>;

  using SubObject = cbe::Object<IntFld, DblFld>;
  using SubObjFld =
    cbe::FieldDef<SubObject, "sub_obj", Required, 6U /* kFldId */>;
  using OptSubObjFld =
    cbe::FieldDef<SubObject, "sub_obj", Optional, 7U /* kFldId */>;

  using TestObject = cbe::Object<IntFld, DblFld, StrFld, OptFld, ArrayFld,
                                 SubObjFld, OptSubObjFld>;

  const auto vec = vector{5U, 10U, 20U};
  const auto obj = TestObject(20010911, 42.0, "foo"s, nullopt, vec,
                              SubObject(20070625, -1.0), nullopt);

  array<uint8_t, 1024> buf = {0};
  auto serializer = cbe::Serializer<TestObject>(span(buf));
  serializer.serialize(obj);

  auto serialized_data = span(buf.begin(), serializer.consumed());
  auto deserializer = Deserializer<TestObject>(serialized_data);
  const auto deserialized = deserializer.deserialize();
  EXPECT_EQ(20010911, jmg::get<IntFld>(deserialized));
  EXPECT_EQ(42.0, jmg::get<DblFld>(deserialized));
  EXPECT_EQ("foo"s, jmg::get<StrFld>(deserialized));
  {
    const auto val = jmg::try_get<OptFld>(deserialized);
    EXPECT_FALSE(pred(val));
  }
  {
    const auto view = jmg::get<ArrayFld>(deserialized);
    EXPECT_THAT(view, ElementsAreArray(vec));
  }
  {
    const auto& sub_obj = jmg::get<SubObjFld>(deserialized);
    EXPECT_EQ(20070625, jmg::get<IntFld>(sub_obj));
    EXPECT_EQ(-1.0, jmg::get<DblFld>(sub_obj));
  }
  {
    const auto* opt_sub_obj = jmg::try_get<OptSubObjFld>(deserialized);
    EXPECT_FALSE(pred(opt_sub_obj));
  }
}
