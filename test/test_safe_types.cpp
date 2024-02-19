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

#include <map>
#include <unordered_map>

#include "jmg/meta.h"
#include "jmg/safe_types.h"

using namespace jmg;
using namespace std;
using namespace std::string_literals;

using TestId32 = SafeId32<>;
using OtherId32 = SafeId32<>;
using TestIdStr = SafeIdStr<>;
using OtherIdStr = SafeId<string>;

#define CONFIRM_STRONG_TYPES(type1, type2)                      \
  do {                                                          \
    EXPECT_FALSE((is_same_v<type1, type2>));                    \
    type1 t1, t2;                                               \
    type2 t3;                                                   \
    EXPECT_TRUE((is_same_v<decltype(t1), decltype(t2)>));       \
    EXPECT_FALSE((is_same_v<decltype(t1), decltype(t3)>));      \
  } while(0)

TEST(SafeTypesTests, TypesAreStrong) {
  CONFIRM_STRONG_TYPES(TestId32, OtherId32);
  CONFIRM_STRONG_TYPES(TestIdStr, OtherIdStr);
}

#undef CONFIRM_STRONG_TYPES

TEST(SafeTypesTests, RetrieveUnsafeType) {
  EXPECT_TRUE((is_same_v<uint32_t, UnsafeTypeFromT<TestId32>>));
  EXPECT_TRUE((is_same_v<string, UnsafeTypeFromT<TestIdStr>>));
}

TEST(SafeTypesTests, StreamOutput) {
  const auto id = TestId32(42);
  ostringstream strm;
  strm << id;
  EXPECT_EQ("42"s, strm.str());
}

#define CONFIRM_COMPARABLE(type, val1, val2)    \
  do {                                          \
    const auto id1 = type(val1);                \
    const auto id2 = type(val1);                \
    const auto id3 = type(val2);                \
    EXPECT_EQ(id1, id2);                        \
    EXPECT_NE(id1, id3);                        \
  } while (0)

TEST(SafeTypesTests, IdsAreComparable) {
  CONFIRM_COMPARABLE(TestId32, 42, 20010911);
  CONFIRM_COMPARABLE(TestIdStr, "foo"s, "bar"s);
}

#undef CONFIRM_COMPARABLE

#define CONFIRM_DICT_HANDLING(dict, key_type, val_type, key_val, val_val) \
  do {                                                                  \
    dict<key_type, val_type> dict;                                      \
    const auto key = key_type(key_val);                                 \
    const val_type value = val_val;                                     \
    const auto [itr, inserted] = dict.try_emplace(key, value);          \
    EXPECT_TRUE(inserted);                                              \
    EXPECT_TRUE(itr != dict.end());                                     \
    EXPECT_FALSE(dict.empty());                                         \
    EXPECT_EQ(dict.count(key), 1);                                      \
    EXPECT_EQ(dict.at(key), value);                                     \
  } while(0)

TEST(SafeTypesTests, IdsAreHashable) {
  CONFIRM_DICT_HANDLING(unordered_map, TestId32, string, 20010911, "foo"s);
  CONFIRM_DICT_HANDLING(unordered_map, TestIdStr, int, "foo"s, 20010911);
}

using TestOrderedId32 = SafeOrderedId32<>;
using TestOtherOrderedId32 = SafeOrderedId32<>;
using TestOrderedIdStr = SafeOrderedIdStr<>;
using OtherOrderedIdStr = SafeOrderedId<string>;

TEST(SafeTypesTests, OrderedIdsAreOrderable) {
  CONFIRM_DICT_HANDLING(map, TestOrderedId32, string, 20010911, "foo"s);
  CONFIRM_DICT_HANDLING(map, TestOrderedIdStr, int, "foo"s, 20010911);
}

#undef CONFIRM_DICT_HANDLING
