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

////////////////////////////////////////////////////////////////////////////////
// tests of 'from' function
////////////////////////////////////////////////////////////////////////////////

TEST(ConversionTests, TestStringFromStringView) {
  const auto src = "foo"sv;
  EXPECT_EQ("foo"s, static_cast<string>(from(src)));
}

TEST(ConversionTests, TestStringFromString) {
  const auto src = "foo"s;
  EXPECT_EQ("foo"s, static_cast<string>(from(src)));
}

TEST(ConversionTests, TestStringFromCStyleString) {
  const char* src = "foo";
  EXPECT_EQ("foo"s, static_cast<string>(from(src)));
}

TEST(ConversionTests, TestStringFromCompileTimeCStyleString) {
  constexpr char src[] = "foo";
  EXPECT_EQ("foo"s, static_cast<string>(from(src)));
}

TEST(ConversionTests, TestIntFromString) {
  EXPECT_EQ(42, static_cast<int>(from("42")));
}

TEST(ConversionTests, TestDoubleFromString) {
  EXPECT_DOUBLE_EQ(0.5, static_cast<double>(from("0.5")));
}

TEST(ConversionTests, TestNumericDeclarations) {
  const auto src = "42"sv;
  const int intVal = from(src);
  EXPECT_EQ(42, intVal);
  const double dblVal = from(src);
  EXPECT_DOUBLE_EQ(42.0, dblVal);
}

TEST(ConversionTests, TestFailedIntFromString) {
  const auto src = "a"sv;
  EXPECT_THROW([[maybe_unused]] int bad = from(src), std::runtime_error);
}

TEST(ConversionTests, TestTimePointFromString) {
  const auto src = "2001-09-11 09:00:00"sv;
  const auto fmt = TimePointFmt("%Y-%m-%d %H:%M:%S");
  const auto tz = getTimeZone(TimeZoneName("America/New_York"));
  TimePoint us_eastern = from(src, fmt, tz);
  // conversion defaults to UTC timezone
  TimePoint utc = from(src, fmt);
  // for time points generated using the same "local clock" time,
  // the UTC time point will be earlier than the US/Eastern time
  // point
  EXPECT_LT(utc, us_eastern);
}

TEST(ConversionTests, TestStringFromTimePoint) {
  const auto tz = getTimeZone(TimeZoneName("America/New_York"));
  const auto tp = [&]() {
    TimePoint rslt = from("2007-06-25T09:00:00", kIso8601Fmt, tz);
    return rslt;
  }();
  const auto fmt = TimePointFmt("%Y-%m-%d %H:%M:%S");
  const std::string actual = from(tp, fmt, tz);
  const auto expected = "2007-06-25 09:00:00"s;
  EXPECT_EQ(expected, actual);
}

////////////////////////////////////////////////////////////////////////////////
// tests of streaming functions
////////////////////////////////////////////////////////////////////////////////

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
