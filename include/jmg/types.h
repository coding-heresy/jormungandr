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

#include <sys/time.h>

#include <ctime>

#include <string>
#include <string_view>

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
// concept for wrapper type, i.e. common cases where a type 'contains' some
// other type in some way
//
// NOTE: concept is declared here because it depends on Safe Type declarations
// but is not strictly a safe type
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept WrapperT = SafeT<T> || ScopedEnumT<T> || OptionalT<T>;

////////////////////////////////////////////////////////////////////////////////
// metafunction that returns the wrapped type of a wrapper type
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct UnwrapT;
template<SafeT T>
struct UnwrapT<T> {
  using type = typename T::value_type;
};
template<ScopedEnumT T>
struct UnwrapT<T> {
  using type = std::underlying_type_t<T>;
};
template<OptionalT T>
struct UnwrapT<T> {
  using type = RemoveOptionalT<T>;
};
} // namespace detail

template<typename T>
using UnwrapT = typename detail::UnwrapT<T>::type;

////////////////////////////////////////////////////////////////////////////////
// return the wrapped type from a wrapper
////////////////////////////////////////////////////////////////////////////////

template<typename T>
constexpr auto unwrap(const T& wrapped) {
  if constexpr (SafeT<T>) { return unsafe(wrapped); }
  else if constexpr (ScopedEnumT<T>) {
    return static_cast<UnwrapT<T>>(wrapped);
  }
  else if constexpr (OptionalT<T>) { return wrapped.get(); }
  else { JMG_NOT_EXHAUSTIVE(T); }
}

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
// TODO(bd) use std::chrono duration instead of absl::Duration?
using Duration = absl::Duration;

#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using TimePointFmt = SafeType<std::string_view, SafeType>;
using TimeZoneName = SafeType<std::string_view, SafeIdType>;
#else
JMG_NEW_SAFE_TYPE(TimePointFmt, std::string_view, SafeIdType);
JMG_NEW_SAFE_TYPE(TimeZoneName, std::string_view, SafeIdType);
#endif

// standard POSIX epoch is 1970-01-01
#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using EpochSeconds = SafeType<time_t, st::arithmetic>;
#else
JMG_NEW_SAFE_TYPE(EpochSeconds, time_t, st::arithmetic);
#endif

// epoch for spreadsheets conforming to the ECMA Office Open XML
// specification
#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using SpreadsheetEpochSeconds = SafeType<double, st::arithmetic>;
#else
JMG_NEW_SAFE_TYPE(SpreadsheetEpochSeconds, time_t, st::arithmetic);
#endif

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
  JMG_ENFORCE(absl::LoadTimeZone(unsafe(tz_name), &rslt),
              "unable to load time zone [", tz_name, "]");
  return rslt;
}

inline TimePoint getCurrentTime() { return absl::Now(); }

////////////////////////////////////////////////////////////////////////////////
// strings
////////////////////////////////////////////////////////////////////////////////

/**
 * subclass of std::string_view that is guaranteed to be a view to a
 * NULL-delimited string
 */
class c_string_view : public std::string_view {
  using Base = std::string_view;

public:
  c_string_view() = default;
  c_string_view(const char* str) : std::string_view(str) {}
  c_string_view(const std::string& str) : std::string_view(str) {}
  const char* c_str() const { return data(); }
};

////////////////////////////////////////////////////////////////////////////////
// miscellaneous
////////////////////////////////////////////////////////////////////////////////

#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using Port = SafeType<uint16_t, SafeIdType>;
using Octet = SafeType<uint8_t, st::arithmetic>;
// safe types wrapping various file descriptors
using FileDescriptor = SafeType<int, SafeIdType>;
using EventFd = SafeType<int, SafeIdType>;
#else
JMG_NEW_SAFE_TYPE(Port, uint16_t, SafeIdType);
JMG_NEW_SAFE_TYPE(Octet, uint8_t, st::arithmetic);
JMG_NEW_SAFE_TYPE(FileDescriptor, int, SafeIdType);
JMG_NEW_SAFE_TYPE(EventFd, int, SafeIdType);
#endif
inline constexpr auto kInvalidFileDescriptor = FileDescriptor(-1);
inline constexpr auto kStdoutFd = FileDescriptor(STDOUT_FILENO);

/**
 * concept for any safe type which wraps an int that should be interpreted as a
 * file descriptor
 */
template<typename T>
concept DescriptorT =
  SameAsDecayedT<EventFd, T> || SameAsDecayedT<FileDescriptor, T>;

using BufferView = std::span<const uint8_t, std::dynamic_extent>;
using BufferProxy = std::span<uint8_t, std::dynamic_extent>;

/**
 * create a BufferView from a type, typically used when reading a
 * small amount of data directly from a variable such as an int
 */
template<typename T>
BufferView buffer_from(const T& ref) {
  return BufferView(reinterpret_cast<const uint8_t*>(&ref), sizeof(ref));
}

/**
 * create a BufferProxy from a type, typically used when writing a
 * small amount of data directly into a variable such as an int
 */
template<typename T>
BufferProxy buffer_from(T& ref) {
  return BufferProxy(reinterpret_cast<uint8_t*>(&ref), sizeof(ref));
}

} // namespace jmg
