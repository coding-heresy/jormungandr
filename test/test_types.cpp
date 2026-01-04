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

#include <concepts>
#include <optional>

#include <gtest/gtest.h>

#include "jmg/types.h"

using namespace jmg;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

constexpr char kTestStr[] = "test";

namespace
{
enum class SomeEnum : uint8_t { kSomeValue = 0, kSomeOtherValue = 1 };
} // namespace

TEST(TypesTest, TestWrappedConcept) {
  EXPECT_FALSE(WrapperT<int>);
  EXPECT_TRUE(WrapperT<optional<int>>);
  EXPECT_TRUE(WrapperT<SomeEnum>);
  EXPECT_TRUE(WrapperT<EpochSeconds>);
}

TEST(TypesTest, TestUnwrapMetafunction) {
  EXPECT_TRUE((same_as<int, UnwrapT<optional<int>>>));
  EXPECT_TRUE((same_as<uint8_t, UnwrapT<SomeEnum>>));
  EXPECT_TRUE((same_as<time_t, UnwrapT<EpochSeconds>>));
}

TEST(TypesTest, TestCStringView) {
  {
    const auto str_view = c_string_view(kTestStr);
    EXPECT_STREQ(kTestStr, str_view.c_str());
  }
  {
    const auto str = std::string(kTestStr);
    const auto str_view = c_string_view(str);
    EXPECT_STREQ(kTestStr, str_view.c_str());
  }
}

TEST(TypesTest, TestBufferView) {
  const auto uint64 = uint64_t(20010911);
  const auto str = "test string"s;

  const auto uint64_buf = buffer_from(uint64);
  EXPECT_EQ(8, uint64_buf.size());

  const auto str_buf = buffer_from(str);
  EXPECT_EQ(str.size(), str_buf.size());

  const auto str_view = string_view(str);
  const auto str_view_buf = buffer_from(str_view);
  EXPECT_EQ(str_view.size(), str_view_buf.size());
}
