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

#include <gtest/gtest.h>

#include <string>

#include "jmg/field.h"
#include "jmg/safe_types.h"
#include "jmg/types.h"

using namespace jmg;
using namespace std;

// safe base types
using Id32 = SafeId32<>;
#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using SafeTs = SafeType<TimePoint, st::arithmetic>;
#else
JMG_NEW_SAFE_TYPE(SafeTs, TimePoint, st::arithmetic);
#endif

// primitive type fields
using IntFld = FieldDef<int, "int", Required>;
using OptFld = FieldDef<float, "opt", Optional>;
using Id32Fld = FieldDef<Id32, "id32", Required>;
using OptId32Fld = FieldDef<Id32, "opt_id32", Optional>;

// class fields
using TimePointFld = FieldDef<TimePoint, "tp", Required>;
using OptTimePointFld = FieldDef<TimePoint, "tp", Optional>;
using TsFld = FieldDef<SafeTs, "tp", Required>;
using OptTsFld = FieldDef<SafeTs, "tp", Optional>;

// string fields
using StrFld = StringField<"str", Required>;
using OptStrFld = StringField<"opt_str", Optional>;

// array fields
using ArrayFld = ArrayField<double, "array_dbl", Required>;
using OptArrayFld = ArrayField<uint64_t, "array_dbl", Optional>;

TEST(FieldTests, TestArgTypeForFieldT) {
  // primitive fields take argument by value, regardless of whether or
  // not they are safe types
  {
    using ArgTypeForIntFld = ArgTypeForFieldT<IntFld>;
    EXPECT_TRUE((same_as<ArgTypeForIntFld, int>));
  }
  {
    using ArgTypeForOptFld = ArgTypeForFieldT<OptFld>;
    EXPECT_TRUE((same_as<ArgTypeForOptFld, float>));
  }
  // class fields take argument by const ref, regardless of whether or
  // not they are safe types
  {
    using ArgTypeForTpFld = ArgTypeForFieldT<TimePointFld>;
    EXPECT_TRUE((same_as<const TimePoint&, ArgTypeForTpFld>));
  }
  {
    using ArgTypeForOptTpFld = ArgTypeForFieldT<OptTimePointFld>;
    EXPECT_TRUE((same_as<const TimePoint&, ArgTypeForOptTpFld>));
  }
  ////////////////////
  // TODO(bd) investigate these very suspicious results, compiler bug
  // again?

  {
    using ArgTypeForId32Fld = ArgTypeForFieldT<Id32Fld>;
    EXPECT_TRUE(SafeT<ArgTypeForId32Fld>);
    EXPECT_TRUE((same_as<UnsafeTypeFromT<ArgTypeForId32Fld>, uint32_t>));
    // EXPECT_TRUE((same_as<ArgTypeForId32Fld, Id32Fld>));
  }
  {
    using ArgTypeForOptId32Fld = ArgTypeForFieldT<OptId32Fld>;
    EXPECT_TRUE(SafeT<ArgTypeForOptId32Fld>);
    EXPECT_TRUE((same_as<UnsafeTypeFromT<ArgTypeForOptId32Fld>, uint32_t>));
    // EXPECT_TRUE((same_as<ArgTypeForOptId32Fld, Id32Fld>));
  }
  {
    using ArgTypeForTsFld = ArgTypeForFieldT<TsFld>;
    EXPECT_TRUE(SafeT<ArgTypeForTsFld>);
    // EXPECT_TRUE(is_const_v<ArgTypeForTsFld>);
    EXPECT_TRUE(is_reference_v<ArgTypeForTsFld>);
    EXPECT_TRUE((same_as<UnsafeTypeFromT<DecayT<ArgTypeForTsFld>>, TimePoint>));
    // EXPECT_TRUE((same_as<const TimePoint&, ArgTypeForTsFld>));
  }
  {
    using ArgTypeForOptTsFld = ArgTypeForFieldT<OptTsFld>;
    EXPECT_TRUE(SafeT<ArgTypeForOptTsFld>);
    // EXPECT_TRUE(is_const_v<ArgTypeForOptTsFld>);
    EXPECT_TRUE(is_reference_v<ArgTypeForOptTsFld>);
    EXPECT_TRUE(
      (same_as<UnsafeTypeFromT<DecayT<ArgTypeForOptTsFld>>, TimePoint>));
    // EXPECT_TRUE((same_as<const TimePoint&, ArgTypeForOptTsFld>));
  }
}

TEST(FieldTests, TestReturnTypeForFieldT) {
  {
    using ReturnTypeForIntFld = ReturnTypeForFieldT<IntFld>;
    EXPECT_TRUE((same_as<int, ReturnTypeForIntFld>));
  }
  {
    using ReturnTypeForOptFld = ReturnTypeForFieldT<OptFld>;
    EXPECT_TRUE((same_as<optional<float>, ReturnTypeForOptFld>));
  }
  {
    using ReturnTypeForTpFld = ReturnTypeForFieldT<TimePointFld>;
    EXPECT_TRUE((same_as<const TimePoint&, ReturnTypeForTpFld>));
  }
  {
    using ReturnTypeForOptTpFld = ReturnTypeForFieldT<OptTimePointFld>;
    EXPECT_TRUE((same_as<const TimePoint*, ReturnTypeForOptTpFld>));
  }
}

TEST(FieldTests, TestStringFieldsWorkCorrectly) {
  {
    using ArgTypeForStrFld = ArgTypeForFieldT<StrFld>;
    EXPECT_TRUE((same_as<ArgTypeForStrFld, string_view>));
  }
  {
    using ArgTypeForOptStrFld = ArgTypeForFieldT<OptStrFld>;
    EXPECT_TRUE((same_as<ArgTypeForOptStrFld, string_view>));
  }
  {
    using ReturnTypeForStrFld = ReturnTypeForFieldT<StrFld>;
    EXPECT_TRUE((same_as<ReturnTypeForStrFld, string_view>));
  }
  {
    using ReturnTypeForOptStrFld = ReturnTypeForFieldT<OptStrFld>;
    EXPECT_TRUE((same_as<ReturnTypeForOptStrFld, optional<string_view>>));
  }
}

TEST(FieldTests, TestArrayFieldsWorkCorrectly) {
  {
    using ReturnTypeForArrayFld = ReturnTypeForFieldT<ArrayFld>;
    EXPECT_TRUE(SpanT<ReturnTypeForArrayFld>);
  }
  {
    using ReturnTypeForOptArrayFld = ReturnTypeForFieldT<OptArrayFld>;
    EXPECT_TRUE(OptionalT<ReturnTypeForOptArrayFld>);
    EXPECT_TRUE(SpanT<RemoveOptionalT<ReturnTypeForOptArrayFld>>);
  }
}
