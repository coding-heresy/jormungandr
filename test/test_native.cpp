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

#include <utility>

#include "jmg/native/native.h"
#include "jmg/object.h"
#include "jmg/util.h"

using namespace jmg;
using namespace std;

using IntFld = FieldDef<int, "int", Required>;
using DblFld = FieldDef<double, "dbl", Required>;
using OptDblFld = FieldDef<double, "dbl", Optional>;
using StrFld = FieldDef<string, "str", Required>;
using OptStrFld = FieldDef<string, "opt_str", Optional>;

// safe types
using Id32 = SafeId32<>;
using Id64 = SafeId64<>;
using SafeIdFld = FieldDef<Id32, "id", Required>;
using OptSafeIdFld = FieldDef<Id64, "opt_id", Optional>;

TEST(NativeObjectTests, TestReturnTypes) {
  using TestObject =
    native::Object<IntFld, OptDblFld, StrFld, OptStrFld, SafeIdFld, OptSafeIdFld>;

#define VALIDATE_GET_RETURN(field, expected)                                   \
  do {                                                                         \
    using field##GetReturn = decltype(jmg::get<field>(declval<TestObject>())); \
    EXPECT_TRUE((same_as<expected, field##GetReturn>));                        \
  } while (0)

#define VALIDATE_TRY_GET_RETURN(field, expected)            \
  do {                                                      \
    using field##TryGetReturn =                             \
      decltype(jmg::try_get<field>(declval<TestObject>())); \
    EXPECT_TRUE((same_as<expected, field##TryGetReturn>));  \
  } while (0)

  // non-class types return by value
  VALIDATE_GET_RETURN(IntFld, int);

  // optional non-class types return by const optional reference
  VALIDATE_TRY_GET_RETURN(OptDblFld, const optional<double>&);

  // class types return by const reference
  VALIDATE_GET_RETURN(StrFld, const string&);

  // optional class types return by const optional reference
  VALIDATE_TRY_GET_RETURN(OptStrFld, const optional<string>&);

  // safe types that wrap non-class types return by value
  using SafeIdFldGetReturn =
    decltype(jmg::get<SafeIdFld>(declval<TestObject>()));
  EXPECT_TRUE(SafeT<SafeIdFldGetReturn>);
  EXPECT_FALSE(is_reference_v<SafeIdFldGetReturn>);
  EXPECT_TRUE((same_as<UnsafeTypeFromT<SafeIdFld::type>, uint32_t>));

  // optional safe types that wrap ono-class types return by const
  // optional reference
  VALIDATE_TRY_GET_RETURN(OptSafeIdFld, const optional<Id64>&);

#undef VALIDATE_GET_RETURN
#undef VALIDATE_TRY_GET_RETURN
}

TEST(NativeObjectTests, TestGet) {
  using TestObject = native::Object<IntFld, DblFld, StrFld, SafeIdFld>;
  auto obj = TestObject(make_tuple(20010911, 42.0, "foo"s, Id32(0)));
  EXPECT_EQ(jmg::get<IntFld>(obj), 20010911);
  EXPECT_DOUBLE_EQ(jmg::get<DblFld>(obj), 42.0);
  EXPECT_EQ(jmg::get<StrFld>(obj), "foo"s);
  EXPECT_EQ(jmg::get<SafeIdFld>(obj), Id32(0));
}

TEST(NativeObjectTests, TestTryGet) {
  using TestObject =
    native::Object<IntFld, DblFld, OptDblFld, OptStrFld, OptSafeIdFld>;
  const auto obj =
    TestObject(make_tuple(20010911, 42.0, nullopt, "bar"s, Id64(64)));
  {
    const auto& opt_dbl = jmg::try_get<OptDblFld>(obj);
    EXPECT_FALSE(pred(opt_dbl));
  }
  {
    const auto& opt_str = jmg::try_get<OptStrFld>(obj);
    EXPECT_TRUE(pred(opt_str));
    EXPECT_EQ(*opt_str, "bar"s);
  }
  {
    const auto& opt_id = jmg::try_get<OptSafeIdFld>(obj);
    EXPECT_TRUE(pred(opt_id));
    EXPECT_EQ(*opt_id, Id64(64));
  }
}

TEST(NativeObjectTests, TestSet) {
  using TestObject =
    native::Object<IntFld, OptDblFld, StrFld, OptStrFld, SafeIdFld>;
  auto obj = TestObject(20010911, 42.0, "foo"s, nullopt, Id32(1));
  EXPECT_EQ(jmg::get<IntFld>(obj), 20010911);
  {
    auto val = jmg::try_get<OptDblFld>(obj);
    EXPECT_TRUE(pred(val));
    EXPECT_EQ(*val, 42.0);
  }

  jmg::set<IntFld>(obj, 20070625);
  EXPECT_EQ(jmg::get<IntFld>(obj), 20070625);
  jmg::set<OptDblFld>(obj, nullopt);
  {
    auto val = jmg::try_get<OptDblFld>(obj);
    EXPECT_FALSE(pred(val));
  }
  jmg::set<OptDblFld>(obj, 1.0);
  {
    auto val = jmg::try_get<OptDblFld>(obj);
    EXPECT_TRUE(pred(val));
    EXPECT_EQ(*val, 1.0);
  }

  jmg::set<StrFld>(obj, "bar"s);
  EXPECT_EQ(jmg::get<StrFld>(obj), "bar"s);

  const auto blub = "blub"s;
  jmg::set<StrFld>(obj, blub);
  EXPECT_EQ(jmg::get<StrFld>(obj), "blub"s);

  jmg::set<OptStrFld>(obj, "something"s);
  {
    auto val = jmg::try_get<OptStrFld>(obj);
    EXPECT_TRUE(pred(val));
    EXPECT_EQ(*val, "something"s);
  }
}

TEST(NativeObjectTests, TestConstructionFromRaw) {
  using TestObject =
    native::Object<IntFld, OptDblFld, StrFld, OptStrFld, SafeIdFld>;
  auto obj = TestObject(20010911, 42.0, "foo"s, nullopt, Id32(1));
  EXPECT_EQ(jmg::get<IntFld>(obj), 20010911);
  {
    const auto& opt_dbl = jmg::try_get<OptDblFld>(obj);
    EXPECT_TRUE(pred(opt_dbl));
    EXPECT_DOUBLE_EQ(*opt_dbl, 42.0);
  }
  EXPECT_EQ(jmg::get<StrFld>(obj), "foo"s);
  {
    const auto& opt_str = jmg::try_get<OptStrFld>(obj);
    EXPECT_FALSE(pred(opt_str));
  }
  EXPECT_EQ(jmg::get<SafeIdFld>(obj), Id32(1));
}
