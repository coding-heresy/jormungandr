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

using namespace jmg;
using namespace std;

using IntFld = FieldDef<int, "int", Required>;
using OptFld = FieldDef<float, "opt", Optional>;
using RawStrFld = FieldDef<string, "raw_str", Required>;
using StrFld = StringField<"str", Required>;
using OptStrFld = StringField<"opt_str", Optional>;
using ArrayFld = ArrayField<double, "array_dbl", Required>;

TEST(FieldTests, TestArgTypeForT) {
  using ArgTypeForIntFld = ArgTypeForT<IntFld>;
  EXPECT_TRUE((same_as<ArgTypeForIntFld, int>));

  using ArgTypeForOptFld = ArgTypeForT<OptFld>;
  EXPECT_TRUE((same_as<ArgTypeForOptFld, float>));

  // #if 0
  //   using ArgTypeForRawStrFld = ArgTypeForT<RawStrFld>;
  //   EXPECT_TRUE((same_as<ArgTypeForRawStrFld, const string&>));
  //   EXPECT_TRUE(is_reference_v<ArgTypeForRawStrFld>);
  // #endif

  using ArgTypeForStrFld = ArgTypeForT<StrFld>;
  EXPECT_TRUE((same_as<ArgTypeForStrFld, const string_view>));
  EXPECT_FALSE(is_reference_v<ArgTypeForStrFld>);

  // #if 0
  //   // TODO(bd) should be 'const optional<string_view>', but this might
  //   // break existing code
  //   using ArgTypeForOptStrFld = ArgTypeForT<OptStrFld>;
  //   EXPECT_TRUE((same_as<ArgTypeForOptStrFld, const optional<string>&>));
  //   EXPECT_TRUE(is_reference_v<ArgTypeForOptStrFld>);
  // #endif
}

TEST(FieldTests, TestReturnTypeForFieldT) {
  using ReturnTypeForIntFld = ReturnTypeForFieldT<IntFld>;
  EXPECT_TRUE((same_as<int, ReturnTypeForIntFld>));
  using ReturnTypeForOptFld = ReturnTypeForFieldT<OptFld>;
  EXPECT_TRUE(detail::OptionalNonViewableNonClass<OptFld>);
  EXPECT_TRUE((same_as<optional<float>, ReturnTypeForOptFld>));
  {
    ReturnTypeForOptFld tmp = 1.0;
    cout << non_decayed_type_name_for(tmp) << endl;
  }
  EXPECT_TRUE((same_as<optional<float>, ReturnTypeForOptFld>));
  using ReturnTypeForStrFld = ReturnTypeForFieldT<StrFld>;
  EXPECT_TRUE((same_as<ReturnTypeForStrFld, string_view>));
  EXPECT_FALSE(is_reference_v<ReturnTypeForStrFld>);

  using ReturnTypeForArrayFld = ReturnTypeForFieldT<ArrayFld>;
  cout << type_name_for<ReturnTypeForArrayFld>() << endl;
}

TEST(FieldTests, TestStringFieldsWorkCorrectly) {
#if defined(TMP_JMG_FIXING_FIELDS)
  EXPECT_FALSE(FieldDefT<RawStrFld>);
#endif
  EXPECT_TRUE(FieldDefT<StrFld>);
  EXPECT_TRUE(FieldDefT<OptStrFld>);
}
