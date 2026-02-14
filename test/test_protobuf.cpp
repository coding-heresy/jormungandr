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

#include "jmg/protobuf/protobuf.h"

#include <concepts>
#include <ranges>

#include <gmock/gmock.h>
#include <google/protobuf/message.h>

#include "test/data/test.pb.h"

using namespace jmg;
using namespace std;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

namespace rng = std::ranges;
namespace vws = std::views;

////////////////////
// aliases for protobuf types

using InnerMsg = jmg::test::InnerMsg;
using TestMsg = jmg::test::TestMsg;
using TestOptMsg = jmg::test::TestOptMsg;

////////////////////
// fields for InnerMsg

using InnerInt32 = protobuf::Field<int32_t, "inner_int_32", Required, 1U>;
using OptInnerStr = protobuf::StringField<"opt_inner_str", Optional, 2U>;

using InnerMsgObj = protobuf::Object<InnerMsg, InnerInt32, OptInnerStr>;

////////////////////
// fields for TestMsg

using Boolean = protobuf::Field<bool, "boolean", Required, 1U>;

using Int32 = protobuf::Field<int32_t, "int_32", Required, 2U>;
using UInt32 = protobuf::Field<uint32_t, "uint_32", Required, 3U>;
using SFixed32 = protobuf::Field<int32_t, "sfixed_32", Required, 4U>;
using Fixed32 = protobuf::Field<uint32_t, "fixed_32", Required, 5U>;

using Int64 = protobuf::Field<int64_t, "int_64", Required, 6U>;
using UInt64 = protobuf::Field<uint64_t, "uint_64", Required, 7U>;
using SFixed64 = protobuf::Field<int64_t, "sfixed_64", Required, 8U>;
using Fixed64 = protobuf::Field<uint64_t, "fixed_64", Required, 9U>;

using Flt = protobuf::Field<float, "flt", Required, 10U>;
using Dbl = protobuf::Field<double, "dbl", Required, 11U>;

using Str = protobuf::StringField<"str", Required, 12U>;
using BytesStr = protobuf::StringField<"bytes_str", Required, 13U>;

using Ts = protobuf::Field<TimePoint, "ts", Required, 14U>;

using Ints = protobuf::ArrayField<int32_t, "ints", Required, 15U>;

using Strs = protobuf::ArrayField<std::string, "strs", Required, 16U>;

using InnerMsgFld = protobuf::Field<InnerMsgObj, "inner_msg", Required, 17U>;

// TODO(bd) handle repeated string fields

// TODO(bd) handle repeated non-primitive, non-string fields

using TestMsgObj = protobuf::Object<TestMsg,
                                    Boolean,
                                    Int32,
                                    UInt32,
                                    SFixed32,
                                    Fixed32,
                                    Int64,
                                    UInt64,
                                    SFixed64,
                                    Fixed64,
                                    Flt,
                                    Dbl,
                                    Str,
                                    BytesStr,
                                    Ts,
                                    Ints,
                                    Strs,
                                    InnerMsgFld>;

using OptBool = protobuf::Field<bool, "opt_bool", Optional, 1U>;

using OptInt32 = protobuf::Field<int32_t, "opt_int_32", Optional, 2U>;
using OptUInt32 = protobuf::Field<uint32_t, "opt_uint_32", Optional, 3U>;
using OptSFixed32 = protobuf::Field<int32_t, "opt_sfixed_32", Optional, 4U>;
using OptFixed32 = protobuf::Field<uint32_t, "opt_fixed_32", Optional, 5U>;

using OptInt64 = protobuf::Field<int64_t, "opt_int_64", Optional, 6U>;
using OptUInt64 = protobuf::Field<uint64_t, "opt_uint_64", Optional, 7U>;
using OptSFixed64 = protobuf::Field<int64_t, "opt_sfixed_64", Optional, 8U>;
using OptFixed64 = protobuf::Field<uint64_t, "opt_fixed_64", Optional, 9U>;

using OptFlt = protobuf::Field<float, "opt_flt", Optional, 10U>;
using OptDbl = protobuf::Field<double, "opt_dbl", Optional, 11U>;

using OptStr = protobuf::StringField<"opt_str", Optional, 12U>;
using OptBytesStr = protobuf::StringField<"opt_bytes_str", Optional, 13U>;

using OptTs = protobuf::Field<TimePoint, "opt_ts", Optional, 14U>;

using OptInnerMsgFld =
  protobuf::Field<InnerMsgObj, "opt_inner_msg", Optional, 15U>;

using TestOptMsgObj = protobuf::Object<TestOptMsg,
                                       OptBool,
                                       OptInt32,
                                       OptUInt32,
                                       OptSFixed32,
                                       OptFixed32,
                                       OptInt64,
                                       OptUInt64,
                                       OptSFixed64,
                                       OptFixed64,
                                       OptFlt,
                                       OptDbl,
                                       OptStr,
                                       OptBytesStr,
                                       OptTs,
                                       OptInnerMsgFld>;

////////////////////////////////////////////////////////////////////////////////
// test fixture
////////////////////////////////////////////////////////////////////////////////

class ProtoTests : public ::testing::Test {
protected:
  void SetUp() override {
    tp_ = getCurrentTime();
    const google::protobuf::Timestamp proto_ts = from(tp_);

    // initialize msg_
    msg_.set_boolean(false);

    msg_.set_int_32(20010911);
    msg_.set_uint_32(20070625);
    msg_.set_sfixed_32(-1);
    msg_.set_fixed_32(19701204);

    msg_.set_int_64(10 * 20010911);
    msg_.set_uint_64(10 * 20070625);
    msg_.set_sfixed_64(10 * -1);
    msg_.set_fixed_64(10 * 19701204);

    msg_.set_flt(42.0f);
    msg_.set_dbl(3.14159);

    msg_.set_str("foo");
    msg_.set_bytes_str("bar");

    *(msg_.mutable_ts()) = from(tp_);

    msg_.add_ints(2011);

    *(msg_.add_strs()) = "foo"s;
    *(msg_.add_strs()) = "bar"s;
    *(msg_.add_strs()) = "blub"s;

    auto& inner_msg = *(msg_.mutable_inner_msg());
    inner_msg.set_inner_int_32(1989);

    // initialize all_full_
    all_full_.set_opt_bool(false);

    all_full_.set_opt_int_32(20010911);
    all_full_.set_opt_uint_32(20070625);
    all_full_.set_opt_sfixed_32(-1);
    all_full_.set_opt_fixed_32(19701204);

    all_full_.set_opt_int_64(10 * 20010911);
    all_full_.set_opt_uint_64(10 * 20070625);
    all_full_.set_opt_sfixed_64(10 * -1);
    all_full_.set_opt_fixed_64(10 * 19701204);

    all_full_.set_opt_flt(42.0f);
    all_full_.set_opt_dbl(3.14159);

    all_full_.set_opt_str("foo");
    all_full_.set_opt_bytes_str("bar");

    *(all_full_.mutable_opt_ts()) = proto_ts;

    auto& opt_inner_msg = *(all_full_.mutable_opt_inner_msg());
    opt_inner_msg.set_inner_int_32(2011);
    opt_inner_msg.set_opt_inner_str("blub");
  }

  TimePoint tp_;
  TestMsg msg_;
  TestOptMsg all_empty_;
  TestOptMsg all_full_;
};

TEST_F(ProtoTests, TestConcepts) {
  EXPECT_TRUE(protobuf::detail::HasProtoFieldId<Int32>);
  EXPECT_TRUE(protobuf::FieldT<Str>);
  EXPECT_TRUE(jmg::OptionalFieldT<OptBool>);
  EXPECT_TRUE(protobuf::detail::NonSpecializedTypeT<int>);
  EXPECT_TRUE(protobuf::detail::NonSpecializedTypeT<InnerMsgObj>);
  EXPECT_TRUE(protobuf::detail::NonSpecializedTypeT<TimePoint>);
  EXPECT_TRUE((same_as<string_view, ArgTypeForFieldT<Str>>));
  using OptTsReturn = decltype(jmg::try_get<OptTs>(declval<TestOptMsgObj>()));
  EXPECT_TRUE((same_as<optional<TimePoint>, OptTsReturn>));
}

TEST_F(ProtoTests, TestGet) {
  const auto obj = TestMsgObj(msg_);

  EXPECT_EQ(msg_.boolean(), jmg::get<Boolean>(obj));

  EXPECT_EQ(msg_.int_32(), jmg::get<Int32>(obj));
  EXPECT_EQ(msg_.uint_32(), jmg::get<UInt32>(obj));
  EXPECT_EQ(msg_.sfixed_32(), jmg::get<SFixed32>(obj));
  EXPECT_EQ(msg_.fixed_32(), jmg::get<Fixed32>(obj));

  EXPECT_EQ(msg_.int_64(), jmg::get<Int64>(obj));
  EXPECT_EQ(msg_.uint_64(), jmg::get<UInt64>(obj));
  EXPECT_EQ(msg_.sfixed_64(), jmg::get<SFixed64>(obj));
  EXPECT_EQ(msg_.fixed_64(), jmg::get<Fixed64>(obj));

  EXPECT_EQ(msg_.flt(), jmg::get<Flt>(obj));
  EXPECT_EQ(msg_.dbl(), jmg::get<Dbl>(obj));

  EXPECT_EQ(msg_.str(), jmg::get<Str>(obj));
  EXPECT_EQ(msg_.bytes_str(), jmg::get<BytesStr>(obj));

  {
    const TimePoint tp = from(msg_.ts());
    EXPECT_EQ(tp, jmg::get<Ts>(obj));
  }

  const auto ints = jmg::get<Ints>(obj);
  EXPECT_TRUE(SpanT<decltype(ints)>);
  EXPECT_EQ(1, ints.size());
  EXPECT_EQ(msg_.ints(0), ints[0]);

  const auto strs = jmg::get<Strs>(obj);
  EXPECT_EQ(3, strs.size());

  EXPECT_TRUE(rng::range<decltype(strs)>);
  for (const auto [expected, actual] : vws::zip(msg_.strs(), strs)) {
    EXPECT_TRUE((SameAsDecayedT<string_view, decltype(actual)>));
    EXPECT_EQ(string_view(expected), actual);
  }

  const auto inner_msg = jmg::get<InnerMsgFld>(obj);
  EXPECT_TRUE(ObjectT<decltype(inner_msg)>);
  EXPECT_EQ(msg_.inner_msg().inner_int_32(), jmg::get<InnerInt32>(inner_msg));
  EXPECT_FALSE(jmg::try_get<OptInnerStr>(inner_msg));
}

#define VALIDATE_OPT_FLD(fld, obj, val)          \
  do {                                           \
    const auto opt_val = jmg::try_get<fld>(obj); \
    EXPECT_TRUE(opt_val);                        \
    EXPECT_EQ((val), *opt_val);                  \
  } while (0)

TEST_F(ProtoTests, TestTryGet) {
  const auto empty_obj = TestOptMsgObj(all_empty_);

  EXPECT_FALSE(jmg::try_get<OptBool>(empty_obj));

  EXPECT_FALSE(jmg::try_get<OptInt32>(empty_obj));
  EXPECT_FALSE(jmg::try_get<OptUInt32>(empty_obj));
  EXPECT_FALSE(jmg::try_get<OptSFixed32>(empty_obj));
  EXPECT_FALSE(jmg::try_get<OptFixed32>(empty_obj));

  EXPECT_FALSE(jmg::try_get<OptInt64>(empty_obj));
  EXPECT_FALSE(jmg::try_get<OptUInt64>(empty_obj));
  EXPECT_FALSE(jmg::try_get<OptSFixed64>(empty_obj));
  EXPECT_FALSE(jmg::try_get<OptFixed64>(empty_obj));

  EXPECT_FALSE(jmg::try_get<OptFlt>(empty_obj));
  EXPECT_FALSE(jmg::try_get<OptDbl>(empty_obj));

  EXPECT_FALSE(jmg::try_get<OptTs>(empty_obj));

  EXPECT_FALSE(jmg::try_get<OptStr>(empty_obj));
  EXPECT_FALSE(jmg::try_get<OptBytesStr>(empty_obj));

  const auto full_obj = TestOptMsgObj(all_full_);

  VALIDATE_OPT_FLD(OptBool, full_obj, all_full_.opt_bool());

  VALIDATE_OPT_FLD(OptInt32, full_obj, all_full_.opt_int_32());
  VALIDATE_OPT_FLD(OptUInt32, full_obj, all_full_.opt_uint_32());
  VALIDATE_OPT_FLD(OptSFixed32, full_obj, all_full_.opt_sfixed_32());
  VALIDATE_OPT_FLD(OptFixed32, full_obj, all_full_.opt_fixed_32());

  VALIDATE_OPT_FLD(OptInt64, full_obj, all_full_.opt_int_64());
  VALIDATE_OPT_FLD(OptUInt64, full_obj, all_full_.opt_uint_64());
  VALIDATE_OPT_FLD(OptSFixed64, full_obj, all_full_.opt_sfixed_64());
  VALIDATE_OPT_FLD(OptFixed64, full_obj, all_full_.opt_fixed_64());

  VALIDATE_OPT_FLD(OptFlt, full_obj, all_full_.opt_flt());
  VALIDATE_OPT_FLD(OptDbl, full_obj, all_full_.opt_dbl());

  VALIDATE_OPT_FLD(OptStr, full_obj, all_full_.opt_str());
  VALIDATE_OPT_FLD(OptBytesStr, full_obj, all_full_.opt_bytes_str());

  VALIDATE_OPT_FLD(OptTs, full_obj,
                   static_cast<TimePoint>(from(all_full_.opt_ts())));

  auto inner_msg_obj = jmg::try_get<OptInnerMsgFld>(full_obj);
  EXPECT_TRUE(pred(inner_msg_obj));
  const auto& opt_inner_obj = all_full_.opt_inner_msg();

  EXPECT_EQ(jmg::get<InnerInt32>(*inner_msg_obj), opt_inner_obj.inner_int_32());
  VALIDATE_OPT_FLD(OptInnerStr, *inner_msg_obj, opt_inner_obj.opt_inner_str());
}

TEST_F(ProtoTests, TestSet) {
  TestMsg msg;
  auto obj = TestMsgObj(msg);
  jmg::set<Boolean>(obj, false);
  jmg::set<Int32>(obj, 20010911);
  jmg::set<UInt32>(obj, 20070625);
  jmg::set<SFixed32>(obj, -1);
  jmg::set<Fixed32>(obj, 19701204);
  jmg::set<Int64>(obj, 10 * 20010911);
  jmg::set<UInt64>(obj, 10 * 20070625);
  jmg::set<SFixed64>(obj, 10 * -1);
  jmg::set<Fixed64>(obj, 10 * 19701204);
  jmg::set<Flt>(obj, 42.0f);
  jmg::set<Dbl>(obj, 3.14159);
  jmg::set<Str>(obj, "foo"sv);
  jmg::set<BytesStr>(obj, "bar"sv);
  jmg::set<Ts>(obj, tp_);
}
