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

#include <array>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_set>

#include "jmg/safe_types.h"
#include "jmg/util.h"

using namespace jmg;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace rng = std::ranges;
namespace vws = std::ranges::views;

TEST(GeneralUtilitiesTest, TestStreamTupleOut) {
  const auto tpl = make_tuple(20010911, 42.0, "foo"s);
  ostringstream strm;
  strm << tpl;
  EXPECT_EQ("20010911,42,foo"s, strm.str());
}

TEST(GeneralUtilitiesTest, TestStreamOptionalOut) {
  optional<int> val;
  {
    ostringstream strm;
    strm << val;
    EXPECT_EQ("<empty>"s, strm.str());
  }
  val = 20010911;
  {
    ostringstream strm;
    strm << val;
    EXPECT_EQ("20010911"s, strm.str());
  }
}

TEST(GeneralUtilitiesTest, TestStreamOctetOut) {
  auto value = uint8_t(1);
  std::array<char, 9> expected;
  expected.fill('0');
  expected[8] = '\0';
  for (size_t counter = 0; counter < 8; ++counter) {
    if (counter > 0) { expected[7 - (counter - 1)] = '0'; }
    expected[7 - counter] = '1';
    ostringstream strm;
    strm << Octet(value);
    EXPECT_EQ(strm.str(), string(expected.data()));
    value <<= 1;
  }

  const auto raw_octets = array<uint8_t, 5>{0, 1, 2, 3, 4};
  const auto bitwise_octets_str =
    str_join(raw_octets
               | vws::transform([](const uint8_t arg) { return Octet(arg); }),
             " "sv, kOctetFmt);
  EXPECT_EQ("00000000 00000001 00000010 00000011 00000100"s, bitwise_octets_str);
}

TEST(GeneralUtilitiesTest, TestUniqers) {
  Dict<int, double> stuff;
  emplace_uniq("stuff"sv, stuff, 1, 42.0);
  emplace_uniq("stuff"sv, stuff, 2, -1.0);
  EXPECT_THROW(emplace_uniq("stuff"sv, stuff, 1, 3.14159), runtime_error);

  Set<int> things;
  insert_uniq("things"sv, things, 1);
  insert_uniq("things"sv, things, 5);
  EXPECT_THROW(insert_uniq("things"sv, things, 1), runtime_error);

  auto find_stuff = [&](const int key) {
    return find_required(stuff, key, "stuff"sv, "stuff key"sv);
  };

  EXPECT_DOUBLE_EQ(42.0, find_stuff(1));
  EXPECT_DOUBLE_EQ(-1.0, find_stuff(2));
  EXPECT_THROW(find_stuff(3), runtime_error);
}

TEST(GeneralUtilitiesTest, TestInserterator) {
  const auto keys = std::array{1, 2, 3};

  std::unordered_set<int> int_set;
  int_set.reserve(keys.size());
  rng::copy(keys, inserterator(int_set));

  EXPECT_EQ(keys.size(), int_set.size());

  std::vector<int> int_vec;
  int_vec.reserve(keys.size());
  ;
  rng::copy(keys, inserterator(int_vec));

  EXPECT_EQ(keys.size(), int_vec.size());

  // TODO(bd) use zip view to test insertion of keys and values into
  // unordered_map
}

TEST(GeneralUtilitiesTest, TestGetFromArgs) {
  const auto int_val = 20010911;
  const auto dbl_val = 42.0;
  const auto str_val = "foo"s;

  // order of arguments doesn't matter
  EXPECT_EQ(20010911, getFromArgs<int>(int_val, dbl_val, str_val));
  EXPECT_EQ(20010911, getFromArgs<int>(dbl_val, str_val, int_val));
  EXPECT_EQ(20010911, getFromArgs<int>(str_val, int_val, dbl_val));
}

TEST(GeneralUtilitiesTest, TestAbseilRework) {
  EXPECT_EQ("foobar"s, str_cat("foo"s, "bar"s));
  {
    auto str = "foo"s;
    str_append(str, "bar"s);
    EXPECT_EQ("foobar"s, str);
  }
  {
    const auto strs = array{"foo"s, "bar"s};
    EXPECT_EQ("foo,bar"s, str_join(strs, ","sv));
  }
}

// TODO(bd) investigate more corner cases?
TEST(GeneralUtilitiesTest, TestSnakeCaseToCamelCase) {
  EXPECT_EQ("Foo"s, snakeCaseToCamelCase("foo"));
  EXPECT_EQ("foo"s, snakeCaseToCamelCase("foo", false /*capitalize_leading */));
  EXPECT_EQ("FooBar"s, snakeCaseToCamelCase("foo_bar"));
  EXPECT_EQ("blub"s,
            snakeCaseToCamelCase("blub_", false /*capitalize_leading */));
}

TEST(GeneralUtilitiesTest, TestCamelCaseToSnakeCase) {
  EXPECT_EQ("foo"s, camelCaseToSnakeCase("Foo"));
  EXPECT_EQ("foo"s, camelCaseToSnakeCase("foo"));
  EXPECT_EQ("foo_bar"s, camelCaseToSnakeCase("FooBar"));
  EXPECT_EQ("foo_bar"s, camelCaseToSnakeCase("fooBar"));
  EXPECT_EQ("foo_bar"s, camelCaseToSnakeCase("foo_bar"));

  EXPECT_EQ("FOO"s, camelCaseToSnakeCase("Foo", true /* all_caps */));
  EXPECT_EQ("FOO"s, camelCaseToSnakeCase("foo", true /* all_caps */));
  EXPECT_EQ("FOO_BAR"s, camelCaseToSnakeCase("FooBar", true /* all_caps */));
  EXPECT_EQ("FOO_BAR"s, camelCaseToSnakeCase("fooBar", true /* all_caps */));
  EXPECT_EQ("FOO_BAR"s, camelCaseToSnakeCase("foo_bar", true /* all_caps */));
}

TEST(GeneralUtilitiesTest, TestUnsafeIfier) {
  using TestId = SafeId32<>;
  constexpr auto int32 = 20010911;
  constexpr auto id32 = TestId(int32);
  EXPECT_EQ(int32, unsafe_ify(int32));
  EXPECT_EQ(int32, unsafe_ify(id32));
}
