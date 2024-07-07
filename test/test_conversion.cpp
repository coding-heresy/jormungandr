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

#include "jmg/conversion.h"

using namespace jmg;
using namespace std;
using namespace std::literals::string_literals;

TEST(ConversionTests, TestStringFromString) {
  const string_view src = "foo";
  EXPECT_EQ("foo"s, static_cast<string>(from_string(src)));
}

TEST(ConversionTests, TestIntFromString) {
  EXPECT_EQ(42, static_cast<int>(from_string("42")));
}

TEST(ConversionTests, TestDoubleFromString) {
  EXPECT_DOUBLE_EQ(0.5, static_cast<double>(from_string("0.5")));
}

TEST(ConversionTests, TestFromStringOverloading) {
  const string_view str = "42";
  const int intVal = from_string(str);
  EXPECT_EQ(42, intVal);
  const double dblVal = from_string(str);
  EXPECT_DOUBLE_EQ(42.0, dblVal);
}

TEST(ConversionTests, TestFailedIntFromString) {
  EXPECT_THROW([[maybe_unused]] int bad = from_string("a"), std::runtime_error);
}

TEST(ConversionTests, TestOptionalStreamOutput) {
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

TEST(ConversionTests, TestTupleStreamOutput) {
  auto tpl = make_tuple(42.0, 20010911);
  ostringstream strm;
  strm << tpl;
  EXPECT_EQ("42,20010911"s, strm.str());
}
