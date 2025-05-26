/** -*- mode: c++ -*-
 *
 * Copyright (C) 2024 Brian Davis
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

#include "jmg/object.h"
#include "jmg/tuple_object/tuple_object.h"
#include "jmg/util.h"

using namespace jmg;
using namespace jmg::tuple_object;
using namespace std;

using IntFld = FieldDef<int, "int", Required>;
using DblFld = FieldDef<double, "dbl", Required>;
using OptDblFld = FieldDef<double, "dbl", Optional>;
using StrFld = FieldDef<string, "str", Required>;
using OptStrFld = FieldDef<string, "opt_str", Optional>;

TEST(TupleObjectTests, GetTest) {
  using TestObject = Object<IntFld, DblFld>;
  auto obj = TestObject(make_tuple(20010911, 42.0));
  EXPECT_EQ(jmg::get<IntFld>(obj), 20010911);
  EXPECT_DOUBLE_EQ(jmg::get<DblFld>(obj), 42.0);
}

TEST(TupleObjectTests, OptionalTest) {
  using TestObject = Object<IntFld, DblFld, OptDblFld>;
  auto obj = TestObject(make_tuple(20010911, 42.0, nullopt));
  {
    const auto& const_obj = obj;
    const auto opt_dbl = jmg::try_get<OptDblFld>(const_obj);
    EXPECT_FALSE(pred(opt_dbl));
  }

TEST(TupleObjectTests, TestConstructionFromRaw) {
  using TestObject = Object<IntFld, DblFld>;
  auto obj = TestObject(20010911, 42.0);
  EXPECT_EQ(jmg::get<IntFld>(obj), 20010911);
  EXPECT_DOUBLE_EQ(jmg::get<DblFld>(obj), 42.0);
}
}
