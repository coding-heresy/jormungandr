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
using StrFld = FieldDef<string, "str", Required>;

TEST(FieldTests, TestArgTypeForT) {
  using ArgTypeForIntFld = ArgTypeForT<IntFld>;
  EXPECT_TRUE((same_as<ArgTypeForIntFld, const int>));
  EXPECT_TRUE(is_const_v<ArgTypeForIntFld>);
  EXPECT_FALSE(is_reference_v<ArgTypeForIntFld>);

  using ArgTypeForOptFld = ArgTypeForT<OptFld>;
  EXPECT_TRUE((same_as<ArgTypeForOptFld, const optional<float>&>));
  EXPECT_TRUE(is_reference_v<ArgTypeForOptFld>);

  using ArgTypeForStrFld = ArgTypeForT<StrFld>;
  EXPECT_TRUE((same_as<ArgTypeForStrFld, const string&>));
  EXPECT_TRUE(is_reference_v<ArgTypeForStrFld>);
}
