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
#pragma once

#include <absl/container/btree_map.h>
#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <absl/time/time.h>

#include "jmg/preprocessor.h"
#include "jmg/safe_types.h"

// standard type aliases

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// containers
////////////////////////////////////////////////////////////////////////////////

template<typename... Ts>
using Dict = absl::flat_hash_map<Ts...>;

template<typename... Ts>
using OrderedDict = absl::btree_map<Ts...>;

template<typename... Ts>
using Set = absl::flat_hash_set<Ts...>;

template<typename... Ts>
using OrderedSet = absl::btree_set<Ts...>;

////////////////////////////////////////////////////////////////////////////////
// time point/duration/zone
////////////////////////////////////////////////////////////////////////////////

using TimePoint = absl::Time;
using TimeZone = absl::TimeZone;
// TODO use std::chrono duration instead of absl::Duration?
using Duration = absl::Duration;

using TimePointFmt = SafeType<std::string_view>;
using TimeZoneName = SafeType<std::string_view>;

// Two formats for ISO 8601: with and without embedded time zone
// specifier
constexpr auto kIso8601Fmt = TimePointFmt("%E4Y-%m-%dT%H:%M:%S");
constexpr auto kIso8601WithZoneFmt = TimePointFmt("%E4Y-%m-%dT%H:%M:%S %z");

inline TimeZone utcTimeZone() {
  static TimeZone utc = absl::UTCTimeZone();
  return utc;
}

inline TimeZone getTimeZone(const TimeZoneName tz_name) {
  TimeZone rslt;
  JMG_ENFORCE(absl::LoadTimeZone(unsafe(tz_name), &rslt), "unable to load "
                                                          "time zone ["
                                                            << tz_name << "]");
  return rslt;
}

} // namespace jmg
