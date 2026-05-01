#include <array>
#include <ranges>
#include <string_view>

#include <gmock/gmock.h>

#include "jmg/cbe/cbe.h"
#include "jmg/conversion.h"
#include "jmg/fp_util.h"
#include "jmg/meta.h"
#include "jmg/stream_io.h"
#include "jmg/util.h"

using namespace jmg;
using namespace std;
using namespace std::string_view_literals;

namespace vws = std::views;

template<OctetBufferT T>
std::ostream& operator<<(std::ostream& strm, const T& buf) {
  strm << "[";
  bool first = true;
  for (auto octet : buf) {
    static_assert(SameAsDecayedT<Octet, decltype(octet)>);
    // if (Octet(0) == octet) { break; }
    if (first) { first = false; }
    else { strm << " "; }
    strm << octet;
  }
  strm << "]";
  return strm;
}

namespace experimental
{

constexpr uint8_t kMask = 127;
constexpr uint8_t kStopBit = (1 << 7);

////////////////////////////////////////////////////////////////////////////////
// stop bit encode/decode (lowest level, only integer)
////////////////////////////////////////////////////////////////////////////////

/**
 * generate a CBE stop-bit encoding for a single unsigned integer writing to an
 * output stream iterator
 */
template<UnsignedT T>
size_t stopBitEncodeTo(OutputStrmItr& itr, T val) {
  // NOTE: this short-circuit step is required for the algorithm to work
  // correctly in its current form
  if (!val) {
    // short-circuit for value 0
    *itr = octetify(kStopBit);
    ++itr;
    return 1;
  }
  size_t idx = 0;
  bool is_finished = false;
  while (!is_finished) {
    uint8_t octet = val & kMask;
    val >>= 7;
    if (!val) {
      is_finished = true;
      octet |= kStopBit;
    }
    *itr = octetify(octet);
    ++idx;
    ++itr;
  }
  return idx;
}

/**
 * decode stop-bit encoding into a single unsigned integer using an input stream
 * iterator
 */
template<UnsignedT T>
T stopBitDecodeFrom(InputStrmItr& itr) {
  T rslt = 0;
  size_t counter = 0;
  while (InputEnd() != itr) {
    auto octet = unsafe(*itr);
    const auto is_finished = static_cast<bool>(octet & kStopBit);
    octet &= ~kStopBit;
    {
      T decoded = octet;
      decoded <<= (7 * counter);
      rslt += decoded;
    }
    ++counter;
    ++itr;
    if (is_finished) { break; }
  }
  return rslt;
}

////////////////////////////////////////////////////////////////////////////////
// integers
////////////////////////////////////////////////////////////////////////////////

/**
 * serialize an integer value to an output stream iterator
 */
template<IntegralT T>
void serializeTo(OutputStrmItr& itr, T val) {
  // TODO(bd) remove this short-circuit and let the algorithm play out?
  if (!val) {
    // short-circuit for value 0
    *itr = octetify(kStopBit);
    ++itr;
    return;
  }
  if constexpr (SignedT<T>) {
    using Tgt = UnsignifyT<T>;
    Tgt sign_bit = (val < 0) ? 1 : 0;
    if (sign_bit) {
      // this is a negative value, whose range contains one more element than
      // the range of the positive values for this type, so add 1 to the
      // negative value, reducing the required range, and rely on the presence
      // of the sign bit to differentiate the value -1 from the value 0 when
      // decoding
      val += 1;
      val = -val;
    }
    // cast the signed value to an unsigned value to enable use of the same
    // stop-bit encoding algorithm as for unsigned values
    auto tgt = static_cast<Tgt>(val);
    // move the sign bit from the most significant bit to the least significant
    // bit, shifting everything else to the left by 1 bit
    tgt <<= 1;
    // transfer the sign bit to the least significant bit by adding the
    // previously computed value
    tgt += sign_bit;
    stopBitEncodeTo(itr, tgt);
  }
  else { stopBitEncodeTo(itr, val); }
}

/**
 * deserialize an integer value from an input stream iterator
 */
template<IntegralT T>
T deserializeFrom(InputStrmItr& itr) {
  if constexpr (UnsignedT<T>) { return stopBitDecodeFrom<T>(itr); }
  else if constexpr (SignedT<T>) {
    using UType = UnsignifyT<T>;
    auto uval = stopBitDecodeFrom<UType>(itr);
    static constexpr UType kLowSignBitMask = 1;
    T addition_val = (uval & kLowSignBitMask) ? 1 : 0;
    // NOTE: right shift must be performed on the unsigned value to avoid
    // sign-extended shift
    uval >>= 1;
    T rslt = static_cast<T>(uval);
    if (!addition_val) { return rslt; }
    return -rslt - 1;
  }
  else { JMG_NOT_EXHAUSTIVE(T, "decodable integer"); }
}

////////////////////////////////////////////////////////////////////////////////
// floating point numbers
////////////////////////////////////////////////////////////////////////////////

/**
 * serialize a floating point value to an output stream iterator
 *
 * NOTE: no compression is performed
 */
template<FloatingPointT T>
void serializeTo(OutputStrmItr& itr, const T val) {
  const auto buf = octet_buffer_from(val);
  for (const auto octet : buf) {
    *itr = octet;
    ++itr;
  }
}

/**
 * deserialize a floating point value from an input stream iterator
 *
 * NOTE: no decompression is performed
 */
template<FloatingPointT T>
T deserializeFrom(InputStrmItr& itr) {
  T rslt;
  auto buf = octet_buffer_from(rslt);
  for (size_t idx : vws::iota(0U, sizeof(T))) {
    buf[idx] = *itr;
    ++itr;
  }
  return rslt;
}

////////////////////////////////////////////////////////////////////////////////
// strings
////////////////////////////////////////////////////////////////////////////////

/**
 * serialize a string-like value to an output stream iterator
 */
template<StringLikeT T>
void serializeTo(OutputStrmItr& itr, const T val) {
  serializeTo(itr, val.size());
  for (const auto chr : val) {
    *itr = octetify(chr);
    ++itr;
  }
}

/**
 * deserialize a string-like value from an output stream iterator
 */
template<StringLikeT T>
std::string deserializeFrom(InputStrmItr& itr) {
  const auto sz = deserializeFrom<size_t>(itr);
  std::string rslt;
  rslt.reserve(sz);
  for (size_t counter = 0; counter < sz; ++counter) {
    rslt.push_back(unsafe(*itr));
    ++itr;
  }
  return rslt;
}

////////////////////////////////////////////////////////////////////////////////
// safe types
////////////////////////////////////////////////////////////////////////////////

/**
 * serialize a safe-type value to an output stream iterator
 */
template<SafeT T>
void serializeTo(OutputStrmItr& itr, const T val) {
  serializeTo(itr, unsafe(val));
}

/**
 * deserialize a safe-type value from an input stream iterator
 */
template<SafeT T>
T deserializeFrom(InputStrmItr& itr) {
  return T(deserializeFrom<UnsafeTypeFromT<T>>(itr));
}

////////////////////////////////////////////////////////////////////////////////
// object fields
////////////////////////////////////////////////////////////////////////////////

/**
 * serialize a CBE field to an output stream iterator
 */
template<cbe::FieldT Fld, cbe::ObjectT Obj>
void serializeTo(OutputStrmItr& itr, const Obj& obj) {
  constexpr auto id = Fld::kFldId;
  serializeTo(itr, id);
  serializeTo<typename Fld::type>(itr, from(jmg::get<Fld>(obj)));
}

/**
 * deserialize a CBE field from an input stream iterator
 */
template<cbe::FieldT Fld>
Fld::type deserializeFrom(InputStrmItr& itr) {
  return deserializeFrom<typename Fld::type>(itr);
}

////////////////////////////////////////////////////////////////////////////////
// misc helpers
////////////////////////////////////////////////////////////////////////////////

template<typename T, size_t kSz>
constexpr void clear(std::array<T, kSz>& buf) {
  void* const ptr = buf.data();
  size_t clear_size = 0;
  if constexpr (IntegralT<T>) { clear_size = sizeof(T) * kSz; }
  else if constexpr (std::same_as<T, Octet>) { clear_size = kSz; }
  else { JMG_NOT_EXHAUSTIVE(T, "buffer"); }
  memset(ptr, 0, clear_size);
}

} // namespace experimental

using namespace experimental;

using IntBuf = array<Octet, 11>;

#define JMG_CHECK_TYPE(type)                      \
  do {                                            \
    IntBuf buf{};                                 \
    type val = static_cast<type>(0);              \
    {                                             \
      auto itr = OutputStrmItr(buf);              \
      serializeTo(itr, val);                      \
    }                                             \
    {                                             \
      auto itr = InputStrmItr(buf);               \
      EXPECT_EQ(val, deserializeFrom<type>(itr)); \
    }                                             \
    val = numeric_limits<type>::min();            \
    clear(buf);                                   \
    {                                             \
      auto itr = OutputStrmItr(buf);              \
      serializeTo(itr, val);                      \
    }                                             \
    {                                             \
      auto itr = InputStrmItr(buf);               \
      EXPECT_EQ(val, deserializeFrom<type>(itr)); \
    }                                             \
    val = numeric_limits<type>::min() + 1;        \
    clear(buf);                                   \
    {                                             \
      auto itr = OutputStrmItr(buf);              \
      serializeTo(itr, val);                      \
    }                                             \
    {                                             \
      auto itr = InputStrmItr(buf);               \
      EXPECT_EQ(val, deserializeFrom<type>(itr)); \
    }                                             \
    val = numeric_limits<type>::max() - 1;        \
    clear(buf);                                   \
    {                                             \
      auto itr = OutputStrmItr(buf);              \
      serializeTo(itr, val);                      \
    }                                             \
    {                                             \
      auto itr = InputStrmItr(buf);               \
      EXPECT_EQ(val, deserializeFrom<type>(itr)); \
    }                                             \
    val = numeric_limits<type>::max();            \
    clear(buf);                                   \
    {                                             \
      auto itr = OutputStrmItr(buf);              \
      serializeTo(itr, val);                      \
    }                                             \
    {                                             \
      auto itr = InputStrmItr(buf);               \
      EXPECT_EQ(val, deserializeFrom<type>(itr)); \
    }                                             \
  } while (0)

TEST(SerializationDeserializationTests, TestStopBitEncoding) {
  JMG_CHECK_TYPE(int8_t);
  JMG_CHECK_TYPE(int16_t);
  JMG_CHECK_TYPE(int32_t);
  JMG_CHECK_TYPE(int64_t);

  JMG_CHECK_TYPE(uint8_t);
  JMG_CHECK_TYPE(uint16_t);
  JMG_CHECK_TYPE(uint32_t);
  JMG_CHECK_TYPE(uint64_t);
}

using OctetBuf = array<Octet, 1024>;

TEST(SerializationDeserializationTests, TestFpSerializationDeserialization) {
  OctetBuf buf;
  {
    auto itr = OutputStrmItr(octet_buffer_from(buf));
    serializeTo(itr, 42.0);
  }
  auto itr = InputStrmItr(octet_buffer_from(buf));
  const auto val = deserializeFrom<double>(itr);
  EXPECT_EQ(val, 42.0);
}

TEST(SerializationDeserializationTests, TestValueSerializationDeserialization) {
  OctetBuf buf;
  {
    auto itr = OutputStrmItr(octet_buffer_from(buf));
    serializeTo(itr, static_cast<uint16_t>(42));
    serializeTo(itr, static_cast<int16_t>(-42));
    serializeTo(itr, "bar"s);
  }
  auto itr = InputStrmItr(octet_buffer_from(buf));
  {
    const auto u16 = deserializeFrom<uint16_t>(itr);
    const auto i16 = deserializeFrom<int16_t>(itr);
    EXPECT_EQ(static_cast<int16_t>(u16), -i16);
  }
  EXPECT_EQ("bar"s, deserializeFrom<string>(itr));
}

using Int32Fld = cbe::Field<int32_t, "int32", Required, 0U /* kFldId */>;
using Uint32Fld = cbe::Field<uint32_t, "uint32", Required, 0U /* kFldId */>;
using StrFld = cbe::StringField<"str", Required, 0U /* kFldId */>;
using TestObj = cbe::Object<Int32Fld, Uint32Fld, StrFld>;

TEST(SerializationDeserializationTests, TestObjectSerializationDeserialization) {
  OctetBuf buf;
  auto itr = OutputStrmItr(octet_buffer_from(buf));

  TestObj test_obj;
  jmg::set<Int32Fld>(test_obj, -42);
  jmg::set<Uint32Fld>(test_obj, 42);
  jmg::set<StrFld>(test_obj, "foo"s);

  serializeTo<Int32Fld>(itr, test_obj);
  serializeTo<Uint32Fld>(itr, test_obj);
  serializeTo<StrFld>(itr, test_obj);
}

#undef JMG_CHECK_TYPE
