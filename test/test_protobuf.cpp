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

#include <gmock/gmock.h>
#include <google/protobuf/message.h>

#include "jmg/protobuf/protobuf.h"

#include "test/test.pb.h"

using namespace jmg;
using namespace std;
using namespace std::literals::string_literals;

////////////////////
// aliases for protobuf types

using InnerMsg = jmg::test::InnerMsg;
using TestMsg = jmg::test::TestMsg;
using TestOptMsg = jmg::test::TestOptMsg;

////////////////////
// fields for InnerMsg

using InnerInt32 = protobuf::FieldDef<int32_t, "inner_int_32", Required, 1>;
using OptInnerStr = protobuf::StringField<"opt_inner_str", Optional, 2>;

using InnerMsgObj = protobuf::Object<InnerMsg, InnerInt32, OptInnerStr>;

////////////////////
// fields for TestMsg

using Boolean = protobuf::FieldDef<bool, "boolean", Required, 1>;

using Int32 = protobuf::FieldDef<int32_t, "int_32", Required, 2>;
using UInt32 = protobuf::FieldDef<uint32_t, "uint_32", Required, 3>;
using SFixed32 = protobuf::FieldDef<int32_t, "sfixed_32", Required, 4>;
using Fixed32 = protobuf::FieldDef<uint32_t, "fixed_32", Required, 5>;

using Int64 = protobuf::FieldDef<int64_t, "int_64", Required, 6>;
using UInt64 = protobuf::FieldDef<uint64_t, "uint_64", Required, 7>;
using SFixed64 = protobuf::FieldDef<int64_t, "sfixed_64", Required, 8>;
using Fixed64 = protobuf::FieldDef<uint64_t, "fixed_64", Required, 9>;

using Flt = protobuf::FieldDef<float, "flt", Required, 10>;
using Dbl = protobuf::FieldDef<double, "dbl", Required, 11>;

using Str = protobuf::StringField<"str", Required, 12>;
using BytesStr = protobuf::StringField<"bytes_str", Required, 13>;

using Ints = protobuf::ArrayField<int32_t, "ints", Required, 14>;

#if defined(JMG_COMPLEX_PROTOBUF_FIELDS_WORK)
using InnerMsgFld = protobuf::FieldDef<InnerMsgObj, "inner_msg", Required, 15>;
#endif

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
                                    Ints
#if defined(JMG_COMPLEX_PROTOBUF_FIELDS_WORK)
                                    ,
                                    InnerMsgFld
#endif
                                    >;

using OptBool = protobuf::FieldDef<bool, "opt_bool", Optional, 1>;

using OptInt32 = protobuf::FieldDef<int32_t, "opt_int_32", Optional, 2>;
using OptUInt32 = protobuf::FieldDef<uint32_t, "opt_uint_32", Optional, 3>;
using OptSFixed32 = protobuf::FieldDef<int32_t, "opt_sfixed_32", Optional, 4>;
using OptFixed32 = protobuf::FieldDef<uint32_t, "opt_fixed_32", Optional, 5>;

using OptInt64 = protobuf::FieldDef<int64_t, "opt_int_64", Optional, 6>;
using OptUInt64 = protobuf::FieldDef<uint64_t, "opt_uint_64", Optional, 7>;
using OptSFixed64 = protobuf::FieldDef<int64_t, "opt_sfixed_64", Optional, 8>;
using OptFixed64 = protobuf::FieldDef<uint64_t, "opt_fixed_64", Optional, 9>;

using OptFlt = protobuf::FieldDef<float, "opt_flt", Optional, 10>;
using OptDbl = protobuf::FieldDef<double, "opt_dbl", Optional, 11>;

using OptStr = protobuf::StringField<"opt_str", Optional, 12>;
using OptBytesStr = protobuf::StringField<"opt_bytes_str", Optional, 13>;

#if defined(JMG_COMPLEX_PROTOBUF_FIELDS_WORK)
using OptInnerMsgFld =
  protobuf::FieldDef<InnerMsgObj, "opt_inner_msg", Optional, 14>;
#endif

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
                                       OptBytesStr
#if defined(JMG_COMPLEX_PROTOBUF_FIELDS_WORK)
                                       ,
                                       OptInnerMsgFld
#endif
                                       >;

////////////////////////////////////////////////////////////////////////////////
// test fixture
////////////////////////////////////////////////////////////////////////////////

class ProtoTests : public ::testing::Test {
protected:
  void SetUp() override {
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

    msg_.add_ints(2011);

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
  }

  TestMsg msg_;
  TestOptMsg all_empty_;
  TestOptMsg all_full_;
};

TEST_F(ProtoTests, TestConcepts) {
  EXPECT_TRUE(jmg::protobuf::detail::HasProtoFieldId<Int32>);
  EXPECT_TRUE(jmg::protobuf::ProtoFieldT<Str>);
  EXPECT_TRUE(jmg::OptionalFieldT<OptBool>);
  EXPECT_TRUE(jmg::protobuf::detail::NonSpecializedTypeT<int>);
  EXPECT_TRUE(jmg::protobuf::detail::NonSpecializedTypeT<InnerMsgObj>);
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

  const auto ints = jmg::get<Ints>(obj);
  EXPECT_TRUE(SpanT<decltype(ints)>);
  EXPECT_EQ(1, ints.size());
  EXPECT_EQ(msg_.ints(0), ints[0]);

#if defined(JMG_COMPLEX_PROTOBUF_FIELDS_WORK)
  const auto& inner_msg = jmg::get<InnerMsgFld>(obj);
  EXPECT_TRUE(ObjectT<decltype(inner_msg)>);
  EXPECT_EQ(msg_.inner_msg().inner_int_32(), jmg::get<InnerInt32>(inner_msg));
  EXPECT_FALSE(jmg::try_get<OptInnerStr>(inner_msg));
#endif
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
}
