/** -*- mode: c++ -*-
 *
 * Copyright (C) 2026 Brian Davis
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

#include <cstdint>

#include <array>
#include <optional>
#include <string>
#include <string_view>

////////////////////
// protobuf

#if defined(JMG_TEST_PROTOBUF)
#if defined(JMG_TEST_CBE)
#error "attempting to test both protobuf and cbe"
#endif

#include "test_jmgc.proto.h"
#endif

////////////////////
// CBE

#if defined(JMG_TEST_CBE)
#if defined(JMG_TEST_PROTOBUF)
#error "attempting to test both cbe and protobuf"
#endif

#include "test_jmgc.cbe.h"
#endif

namespace jmg::test_jmgc
{

/**
 * static test values used to construct test cases
 */
struct TestValues {
  static constexpr auto kBoolean = false;
  // 32 bit integers
  static constexpr auto kInt32 = static_cast<int32_t>(20010911);
  static constexpr auto kUInt32 = static_cast<uint32_t>(17760704U);
  static constexpr auto kSFixed32 = static_cast<int32_t>(-1);
  static constexpr auto kFixed32 = static_cast<uint32_t>(19440606);
  // 64 bit integers
  static constexpr auto kInt64 = static_cast<int64_t>(10 * 20010911);
  static constexpr auto kUInt64 = static_cast<uint64_t>(10 * 17760704U);
  static constexpr auto kSFixed64 = static_cast<int64_t>(10 * -1);
  static constexpr auto kFixed64 = static_cast<uint64_t>(10 * 19440606);
  // floating point numbers
  static constexpr auto kFlt = 42.0f;
  static constexpr auto kDbl = 3.14159;
  // strings
  static const std::string kStr;
  static const std::string kBytesStr;
  // safe integer ID
  static const IntId kIntId;
  // enum
  static const Active kActiveState;
  // arrays
  static constexpr auto kInts = std::array{static_cast<int32_t>(2011)};
  static const std::array<std::string, 3> kStrs;
  static constexpr auto kInnerInts =
    std::array{static_cast<int32_t>(1), static_cast<int32_t>(2)};
  static const std::array<std::optional<std::string>, 2> kInnerOptStrs;
};

} // namespace jmg::test_jmgc

////////////////////////////////////////////////////////////////////////////////
// sections specific to each encoding type go below here
////////////////////////////////////////////////////////////////////////////////

////////////////////
// protobuf

#if defined(JMG_TEST_PROTOBUF)

namespace jmg::test_jmgc
{

template<typename T, jmg::StrLiteral kName, jmg::TypeFlagT IsRequired, uint32_t kFldId>
using FldDefType = jmg::protobuf::Field<T, kName, IsRequired, kFldId>;

template<jmg::StrLiteral kName, jmg::TypeFlagT IsRequired, uint32_t kFldId>
using StrFldDefType = jmg::protobuf::StringField<kName, IsRequired, kFldId>;

template<typename T, jmg::StrLiteral kName, jmg::TypeFlagT IsRequired, uint32_t kFldId>
using ArrayDefType = jmg::protobuf::ArrayField<T, kName, IsRequired, kFldId>;

template<typename T, typename... Ts>
using ObjDefType = jmg::protobuf::Object<T, Ts...>;

using NonJmgTestMsg = jmgc::test::TestMsg;
using JmgTestMsg = TestMsgObj;

NonJmgTestMsg makeNonJmgTestMsg(const jmg::TimePoint tp);

using NonJmgTestOptMsg = jmgc::test::TestOptMsg;
using JmgTestOptMsg = TestOptMsgObj;

} // namespace jmg::test_jmgc

using namespace jmg::protobuf;
using namespace jmgc::test;
#endif

////////////////////
// CBE

#if defined(JMG_TEST_CBE)
namespace jmg::test_jmgc
{

template<typename T, jmg::StrLiteral kName, jmg::TypeFlagT IsRequired, uint32_t kFldId>
using FldDefType = jmg::cbe::Field<T, kName, IsRequired, kFldId>;

template<jmg::StrLiteral kName, jmg::TypeFlagT IsRequired, uint32_t kFldId>
using StrFldDefType = jmg::cbe::StringField<kName, IsRequired, kFldId>;

template<typename T, jmg::StrLiteral kName, jmg::TypeFlagT IsRequired, uint32_t kFldId>
using ArrayDefType = jmg::cbe::ArrayField<T, kName, IsRequired, kFldId>;

template<typename T, typename... Ts>
using ObjDefType = jmg::cbe::Object<T, Ts...>;

using NonJmgTestMsg = TestMsg::adapted_type;
using JmgTestMsg = TestMsg;

NonJmgTestMsg makeNonJmgTestMsg(const jmg::TimePoint tp);

using NonJmgTestOptMsg = TestOptMsg::adapted_type;
using JmgTestOptMsg = TestOptMsg;

} // namespace jmg::test_jmgc

using namespace jmg::cbe;

// TODO(bd)

#endif
