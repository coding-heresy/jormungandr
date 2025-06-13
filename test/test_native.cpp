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

#include <gmock/gmock.h>

#include <ranges>
#include <utility>

#include "jmg/native/native.h"
#include "jmg/object.h"
#include "jmg/util.h"

using namespace jmg;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;
using ::testing::ElementsAreArray;

using IntFld = FieldDef<int, "int", Required>;
using DblFld = FieldDef<double, "dbl", Required>;
using OptDblFld = FieldDef<double, "dbl", Optional>;
using StrFld = StringField<"str", Required>;
using OptStrFld = StringField<"opt_str", Optional>;
using ArrayFld = ArrayField<int, "int_array", Required>;
using OptArrayFld = ArrayField<double, "int_array", Optional>;

// safe types
using Id32 = SafeId32<>;
using Id64 = SafeId64<>;
using SafeIdFld = FieldDef<Id32, "id", Required>;
using OptSafeIdFld = FieldDef<Id64, "opt_id", Optional>;

namespace rng = std::ranges;

TEST(NativeObjectTests, TestReturnTypes) {
  using TestObject =
    native::Object<IntFld, OptDblFld, StrFld, OptStrFld, SafeIdFld,
                   OptSafeIdFld, ArrayFld, OptArrayFld>;

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
  VALIDATE_GET_RETURN(StrFld, string_view);

  // optional class types return by const optional reference
  VALIDATE_TRY_GET_RETURN(OptStrFld, optional<string_view>);

  // class types return by const reference
  VALIDATE_GET_RETURN(ArrayFld, span<const int>);

  // optional class types return by const optional reference
  VALIDATE_TRY_GET_RETURN(OptArrayFld, optional<span<const double>>);

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
  using TestObject =
    native::Object<IntFld, DblFld, StrFld, SafeIdFld, ArrayFld>;
  const auto vec = vector{2, 4, 6, 8};
  auto obj = TestObject(make_tuple(20010911, 42.0, "foo"s, Id32(0), vec));
  EXPECT_EQ(jmg::get<IntFld>(obj), 20010911);
  EXPECT_DOUBLE_EQ(jmg::get<DblFld>(obj), 42.0);
  EXPECT_EQ(jmg::get<StrFld>(obj), "foo"s);
  EXPECT_EQ(jmg::get<SafeIdFld>(obj), Id32(0));
  {
    const auto view = jmg::get<ArrayFld>(obj);
    EXPECT_THAT(view, ElementsAreArray(vec));
  }
}

#define VALIDATE_TRY_GET_OPTIONAL(field, obj, expected) \
  do {                                                  \
    auto val = jmg::try_get<field>(obj);                \
    EXPECT_TRUE(pred(val));                             \
    EXPECT_EQ(*val, expected);                          \
  } while (0)

TEST(NativeObjectTests, TestTryGet) {
  using TestObject = native::Object<IntFld, DblFld, OptDblFld, OptStrFld,
                                    OptSafeIdFld, OptArrayFld>;
  const auto vec = vector{2.0, 4.0, 6.0, 8.0};
  const auto obj =
    TestObject(make_tuple(20010911, 42.0, nullopt, "bar"s, Id64(64), vec));
  {
    const auto& opt_dbl = jmg::try_get<OptDblFld>(obj);
    EXPECT_FALSE(pred(opt_dbl));
  }
  VALIDATE_TRY_GET_OPTIONAL(OptStrFld, obj, "bar"s);
  VALIDATE_TRY_GET_OPTIONAL(OptSafeIdFld, obj, Id64(64));
  {
    const auto view = jmg::try_get<OptArrayFld>(obj);
    EXPECT_TRUE(pred(view));
    EXPECT_THAT(*view, ElementsAreArray(vec));
  }
}

TEST(NativeObjectTests, TestSet) {
  using TestObject =
    native::Object<IntFld, OptDblFld, StrFld, OptStrFld, SafeIdFld, ArrayFld>;
  auto obj =
    TestObject(20010911, 42.0, "foo"s, nullopt, Id32(1), vector<int>());
  EXPECT_EQ(jmg::get<IntFld>(obj), 20010911);
  VALIDATE_TRY_GET_OPTIONAL(OptDblFld, obj, 42.0);

  jmg::set<IntFld>(obj, 20070625);
  EXPECT_EQ(jmg::get<IntFld>(obj), 20070625);
  jmg::set<OptDblFld>(obj, nullopt);
  {
    auto val = jmg::try_get<OptDblFld>(obj);
    EXPECT_FALSE(pred(val));
  }
  jmg::set<OptDblFld>(obj, 1.0);
  VALIDATE_TRY_GET_OPTIONAL(OptDblFld, obj, 1.0);

  ////////////////////
  // numerous special cases for fields containing viewable types

  // raw string works for required string field
  jmg::set<StrFld>(obj, "bar"s);
  EXPECT_EQ(jmg::get<StrFld>(obj), "bar"s);

  // string from variable works for required string field
  const auto blub = "blub"s;
  jmg::set<StrFld>(obj, blub);
  EXPECT_EQ(jmg::get<StrFld>(obj), "blub"sv);

  // raw C-style string works for required string field
  jmg::set<StrFld>(obj, "blab");
  EXPECT_EQ(jmg::get<StrFld>(obj), "blab"sv);

  // C-style string from variable works for required string field
  const char* blob = "blob";
  jmg::set<StrFld>(obj, blob);
  EXPECT_EQ(jmg::get<StrFld>(obj), "blob"sv);

  // raw string works for optional string field
  jmg::set<OptStrFld>(obj, "something"s);
  VALIDATE_TRY_GET_OPTIONAL(OptStrFld, obj, "something"sv);

  // string from variable works for optional string field
  const auto something_else = "something else"s;
  jmg::set<OptStrFld>(obj, something_else);
  VALIDATE_TRY_GET_OPTIONAL(OptStrFld, obj, "something else"sv);

  // raw C-style string works for optional string field
  jmg::set<OptStrFld>(obj, "another_thing");
  VALIDATE_TRY_GET_OPTIONAL(OptStrFld, obj, "another_thing"sv);

  // C-style string from variable works for optional string field
  const char* yet_another_thing = "yet another thing";
  jmg::set<OptStrFld>(obj, yet_another_thing);
  VALIDATE_TRY_GET_OPTIONAL(OptStrFld, obj, "yet another thing"sv);

  // vector copy works for required vector field
  {
    const auto vec = vector{1, 2, 3};
    jmg::set<ArrayFld>(obj, vec);
    {
      const auto view = jmg::get<ArrayFld>(obj);
      EXPECT_THAT(view, ElementsAreArray(vec));
    }
  }
}

TEST(NativeObjectTests, TestConstructionFromRaw) {
  using TestObject =
    native::Object<IntFld, OptDblFld, StrFld, OptStrFld, SafeIdFld, OptArrayFld>;
  auto obj = TestObject(20010911, 42.0, "foo"s, nullopt, Id32(1), nullopt);
  EXPECT_EQ(jmg::get<IntFld>(obj), 20010911);
  VALIDATE_TRY_GET_OPTIONAL(OptDblFld, obj, 42.0);
  EXPECT_EQ(jmg::get<StrFld>(obj), "foo"s);
  {
    const auto& opt_str = jmg::try_get<OptStrFld>(obj);
    EXPECT_FALSE(pred(opt_str));
  }
  EXPECT_EQ(jmg::get<SafeIdFld>(obj), Id32(1));
  {
    const auto opt_array = jmg::try_get<OptArrayFld>(obj);
    EXPECT_FALSE(pred(opt_array));
  }
}

#undef VALIDATE_TRY_GET_OPTIONAL
