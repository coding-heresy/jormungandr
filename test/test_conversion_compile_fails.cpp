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

// this code will fail to compile and is intended for use with the
// cc_build_error rule from the rules_build_error repository

// NOTE: there are multiple tests that correspond to different rules in BUILD
// and the tests are selected by conditional compilation with the goal of
// avoiding a profusion of files with low signal to noise ratio and mirroring
// the structure of the existing unit test files

#include "jmg/conversion.h"

using namespace jmg;

#if defined(VERIFY_MISSING_TIME_FORMAT_FAILS)
TimePoint missingTimeFormtInTimePointConversionShouldFail() {
  TimePoint rslt = from("2007-06-25T09:00:00");
  return rslt;
}
#endif

#if defined(VERIFY_MULTIPLE_TIME_FORMATS_FAIL)
const auto kUsFmt = TimePointFmt("%m/%d/%Y %H:%M:%S");

TimePoint multipleTimeFormtsInTimePointConversionShouldFail() {
  TimePoint rslt = from("2007-06-25T09:00:00", kIso8601Fmt, kUsFmt);
  return rslt;
}
#endif

#if defined(VERIFY_MULTIPLE_TIME_ZONES_FAIL)
const auto kUsEasternZone = getTimeZone(TimeZoneName("America/New_York"));
const auto kUsCentralZone = getTimeZone(TimeZoneName("America/Chicago"));

TimePoint multipleTimeZonesInTimePointConversionShouldFail() {
  TimePoint rslt = from("2007-06-25T09:00:00", kIso8601Fmt, kUsFmt);
  return rslt;
}
#endif
