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

using namespace boost::posix_time;
using namespace jmg;
using namespace std;
using namespace std::literals::string_literals;

////////////////////////////////////////////////////////////////////////////////
// tests of 'from' function
////////////////////////////////////////////////////////////////////////////////

////////////////////
// conversions between string types

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

////////////////////
// conversions between numeric and string types

TEST(ConversionTests, TestIntFromString) {
  EXPECT_EQ(42, static_cast<int>(from("42")));
}

TEST(ConversionTests, TestDoubleFromString) {
  EXPECT_DOUBLE_EQ(0.5, static_cast<double>(from("0.5")));
}

TEST(ConversionTests, TestNumericFromStringView) {
  const auto src = "42"sv;
  const int intVal = from(src);
  EXPECT_EQ(42, intVal);
  const double dblVal = from(src);
  EXPECT_DOUBLE_EQ(42.0, dblVal);
}

TEST(ConversionTests, TestFailedIntFromStringViewThrowsRuntimeError) {
  const auto src = "a"sv;
  EXPECT_THROW([[maybe_unused]] int bad = from(src), std::runtime_error);
}

////////////////////
// conversions between time points and string types

const auto kFmt = TimePointFmt("%Y-%m-%d %H:%M:%S");
const auto kUsEasternZone = getTimeZone(TimeZoneName("America/New_York"));

TEST(ConversionTests, TestTimePointFromString) {
  const auto src = "2001-09-11 09:00:00"sv;
  TimePoint us_eastern = from(src, kFmt, kUsEasternZone);
  // conversion defaults to UTC timezone
  TimePoint utc = from(src, kFmt);
  // for time points generated using the same "local clock" time,
  // the UTC time point will be earlier than the US/Eastern time
  // point
  EXPECT_LT(utc, us_eastern);
}

TEST(ConversionTests, TestStringFromTimePoint) {
  const auto tp = []() {
    // NOTE: also testing ISO 8601 format from standard constant
    TimePoint rslt = from("2007-06-25T09:00:00", kIso8601Fmt, kUsEasternZone);
    return rslt;
  }();
  {
    const std::string actual_us_eastern = from(tp, kFmt, kUsEasternZone);
    const auto expected = "2007-06-25 09:00:00"s;
    EXPECT_EQ(expected, actual_us_eastern);
  }
  {
    const std::string actual_gmt = from(tp, kFmt);
    const auto expected = "2007-06-25 13:00:00"s;
    EXPECT_EQ(expected, actual_gmt);
  }
}

////////////////////
// conversions between various time point types

const auto kTimePoint = []() -> TimePoint {
  // NOTE: static_cast will force the appropriate return type
  return static_cast<TimePoint>(from("2001-09-11T09:00:00", kIso8601Fmt,
                                     kUsEasternZone));
}();

// NOTE: the number of seconds corresponding to kTimePoint
const auto kTimePointSeconds = time_t(1000213200);
const auto kEpochSeconds = EpochSeconds(kTimePointSeconds);

TEST(ConversionTests, TestEpochSecondsFromTimePoint) {
  EpochSeconds actual = from(kTimePoint);
  const auto expected = kEpochSeconds;
  EXPECT_EQ(expected, actual);
}

TEST(ConversionTests, TestTimevalFromTimePoint) {
  using Timeval = struct timeval;
  Timeval actual = from(kTimePoint);
  const auto expected = Timeval{.tv_sec = kTimePointSeconds, .tv_usec = 0};
  EXPECT_EQ(expected.tv_sec, actual.tv_sec);
  EXPECT_EQ(expected.tv_usec, actual.tv_usec);
}

TEST(ConversionTests, TestTimespecFromTimePoint) {
  using Timespec = struct timespec;
  Timespec actual = from(kTimePoint);
  const auto expected = Timespec{.tv_sec = kTimePointSeconds, .tv_nsec = 0};
  EXPECT_EQ(expected.tv_sec, actual.tv_sec);
  EXPECT_EQ(expected.tv_nsec, actual.tv_nsec);
}

TEST(ConversionTests, TestPosixTimeFromTimePoint) {
  ptime actual = from(kTimePoint);
  const auto expected = from_time_t(kTimePointSeconds);
  EXPECT_EQ(expected, actual);
}

TEST(ConversionTests, TestTimePointFromEpochSeconds) {
  TimePoint actual = from(kEpochSeconds);
  const auto expected = kTimePoint;
  EXPECT_EQ(expected, actual);
}

TEST(ConversionTests, TestTimePointFromTimeval) {
  using Timeval = struct timeval;
  auto tv_tp = Timeval{.tv_sec = kTimePointSeconds, .tv_usec = 0};
  TimePoint actual = from(tv_tp);
  const auto expected = kTimePoint;
  EXPECT_EQ(expected, actual);
}

TEST(ConversionTests, TestTimePointFromTimespec) {
  using Timespec = struct timespec;
  auto ts_tp = Timespec{.tv_sec = kTimePointSeconds, .tv_nsec = 0};
  TimePoint actual = from(ts_tp);
  const auto expected = kTimePoint;
  EXPECT_EQ(expected, actual);
}

TEST(ConversionTests, TestTimePointFromPosixTime) {
  const auto pt_tp = from_time_t(kTimePointSeconds);
  TimePoint actual = from(pt_tp);
  const auto expected = kTimePoint;
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
