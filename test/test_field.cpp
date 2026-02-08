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
#include "jmg/types.h"

using namespace jmg;
using namespace std;

// integral type fields
using IntFld = FieldDef<int, "int", Required>;
using OptFld = FieldDef<float, "opt", Optional>;

// string fields
using StrFld = StringField<"str", Required>;
using OptStrFld = StringField<"opt_str", Optional>;

// array fields
using ArrayFld = ArrayField<double, "array_dbl", Required>;
using OptArrayFld = ArrayField<uint64_t, "array_dbl", Optional>;

// class fields
using TimePointFld = FieldDef<TimePoint, "tp", Required>;

TEST(FieldTests, TestArgTypeForFieldT) {
  using ArgTypeForIntFld = ArgTypeForFieldT<IntFld>;
  EXPECT_TRUE((same_as<ArgTypeForIntFld, int>));

  using ArgTypeForOptFld = ArgTypeForFieldT<OptFld>;
  EXPECT_TRUE((same_as<ArgTypeForOptFld, float>));

  // TODO(bd) also support non-const ref return type?
  using ArgTypeForFieldTpFld = ArgTypeForFieldT<TimePointFld>;
  EXPECT_TRUE((same_as<const TimePoint&, ArgTypeForFieldTpFld>));
}

TEST(FieldTests, TestReturnTypeForFieldT) {
  using ReturnTypeForIntFld = ReturnTypeForFieldT<IntFld>;
  EXPECT_TRUE((same_as<int, ReturnTypeForIntFld>));

  using ReturnTypeForOptFld = ReturnTypeForFieldT<OptFld>;
  EXPECT_TRUE((same_as<optional<float>, ReturnTypeForOptFld>));

  // TODO(bd) should be 'const TimePoint&' for the argument type?
  using ReturnTypeForTpFld = ReturnTypeForFieldT<TimePointFld>;
  EXPECT_TRUE((same_as<TimePoint&, ReturnTypeForTpFld>));
}

TEST(FieldTests, TestStringFieldsWorkCorrectly) {
  using ArgTypeForStrFld = ArgTypeForFieldT<StrFld>;
  EXPECT_TRUE((same_as<ArgTypeForStrFld, const string_view>));

  using ArgTypeForOptStrFld = ArgTypeForFieldT<OptStrFld>;
  EXPECT_TRUE((same_as<ArgTypeForOptStrFld, string_view>));

  using ReturnTypeForStrFld = ReturnTypeForFieldT<StrFld>;
  EXPECT_TRUE((same_as<ReturnTypeForStrFld, string_view>));

  using ReturnTypeForOptStrFld = ReturnTypeForFieldT<OptStrFld>;
  EXPECT_TRUE((same_as<ReturnTypeForOptStrFld, optional<string_view>>));
}

TEST(FieldTests, TestArrayFieldsWorkCorrectly) {
  using ReturnTypeForArrayFld = ReturnTypeForFieldT<ArrayFld>;
  EXPECT_TRUE(SpanT<ReturnTypeForArrayFld>);

  using ReturnTypeForOptArrayFld = ReturnTypeForFieldT<OptArrayFld>;
  EXPECT_TRUE(OptionalT<ReturnTypeForOptArrayFld>);
  EXPECT_TRUE(SpanT<RemoveOptionalT<ReturnTypeForOptArrayFld>>);
}
