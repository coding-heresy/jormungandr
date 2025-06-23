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

#include <charconv>
#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "jmg/meta.h"
#include "jmg/preprocessor.h"
#include "jmg/types.h"

#define DETAIL_ENFORCE_EMPTY_EXTRAS(extras, src_type, tgt_type) \
  static_assert(0 == sizeof...(extras),                         \
                "extra arguments are not supported "            \
                "for conversion from " #src_type " to " #tgt_type)

namespace jmg
{

namespace detail
{

/**
 * implementation class that holds the logic for converting one type
 * to another
 *
 * New conversions should be added to the static 'convert' function.
 *
 * TODO try to come up with a way to use customization point objects
 * to avoid having all of the conversions residing here
 */
template<typename Tgt, typename Src, typename... Extras>
struct ConvertImpl {
  static Tgt convert(const Src src, Extras&&... extras) {
    using namespace boost::posix_time;
    if constexpr (StringLikeT<std::remove_cvref_t<Src>>) {
      // conversions of strings to other types

      // convert string-like type to itself
      if constexpr (sameAsDecayed<Tgt, Src>()) {
        DETAIL_ENFORCE_EMPTY_EXTRAS(Extras, string, numeric);
        return src;
      }
      // convert string-like type to std::string or std:;string_view
      // (but not itself, which was handled in the previous case)
      else if constexpr (std::same_as<Tgt, std::string>
                         || std::same_as<Tgt, std::string_view>) {
        DETAIL_ENFORCE_EMPTY_EXTRAS(Extras, string, string);
        return Tgt(src);
      }
      // convert string-like type to arithmetic type
      else if constexpr (ArithmeticT<Tgt>) {
        DETAIL_ENFORCE_EMPTY_EXTRAS(Extras, string, numeric);
        auto rslt = Tgt();
        const auto [_, err] =
          std::from_chars(src.data(), src.data() + src.size(), rslt);
        JMG_ENFORCE(std::errc() == err,
                    "unable to convert string value ["
                      << src << "] to integral value of type ["
                      << type_name_for<Tgt>()
                      << "]: " << std::make_error_code(err).message());
        return rslt;
      }
      // convert string-like type to time point
      else if constexpr (std::same_as<Tgt, TimePoint>) {
        return str2TimePoint(src, std::forward<Extras>(extras)...);
      }
      else { JMG_NOT_EXHAUSTIVE(Tgt); }
    }
    else if constexpr (sameAsDecayed<TimePoint, Src>()) {
      // conversion of TimePoint to other types or representations

      // convert TimePoint to string
      if constexpr (std::same_as<std::string, Tgt>) {
        return timePoint2Str(src, std::forward<Extras>(extras)...);
      }
      // convert TimePoint to EpochSeconds
      else if constexpr (std::same_as<EpochSeconds, Tgt>) {
        return EpochSeconds(absl::ToTimeT(src));
      }
      // convert TimePoint to timeval
      else if constexpr (std::same_as<timeval, Tgt>) {
        return absl::ToTimeval(src);
      }
      // convert TimePoint to timespec
      else if constexpr (std::same_as<timespec, Tgt>) {
        return absl::ToTimespec(src);
      }
      // convert TimePoint to boost::posix_time::ptime
      else if constexpr (std::same_as<ptime, Tgt>) {
        static const auto kPtimeEpoch = from_time_t(0);
        return kPtimeEpoch + nanoseconds(absl::ToUnixNanos(src));
      }
      // TODO convert TimePoint to std::chrono
      else { JMG_NOT_EXHAUSTIVE(Tgt); }
    }
    // convert from EpochSeconds to TimePoint
    else if constexpr (sameAsDecayed<EpochSeconds, Src>()) {
      static_assert(std::same_as<TimePoint, Tgt>,
                    "conversion from EpochSeconds must target TimePoint");
      return absl::FromTimeT(unsafe(src));
    }
    // convert from timesval to TimePoint
    else if constexpr (sameAsDecayed<timeval, Src>()) {
      static_assert(std::same_as<TimePoint, Tgt>,
                    "conversion from timeval must target TimePoint");
      return absl::FromUnixMicros([&]() {
        return (src.tv_sec * 1000000) + src.tv_usec;
      }());
    }
    // convert from timespec to TimePoint
    else if constexpr (sameAsDecayed<timespec, Src>()) {
      static_assert(std::same_as<TimePoint, Tgt>,
                    "conversion from timespec must target TimePoint");
      return absl::FromUnixNanos([&]() {
        return (src.tv_sec * 1000000000) + src.tv_nsec;
      }());
    }
    // convert from boost::posix_time::ptime to TimePoint
    else if constexpr (sameAsDecayed<ptime, Src>()) {
      static_assert(
        std::same_as<TimePoint, Tgt>,
        "conversion from boost::posix_time::ptime must target TimePoint");
      static const auto kPtimeEpoch = from_time_t(0);
      const auto since_epoch = src - kPtimeEpoch;
      return absl::FromUnixNanos(since_epoch.total_nanoseconds());
    }

    // TODO convert from std::chrono::duration to Duration

    // TODO convert from Duration to std::chrono::duration

    else { JMG_NOT_EXHAUSTIVE(Src); }
  }

private:
  /**
   * helper class for conversions between string and time point
   *
   * Handles time zone and string format specifications provided as
   * variadic arguments to the general conversion function.
   */
  struct TimePointConversionSpec {
    explicit TimePointConversionSpec(Extras&&... extras) {
      static_assert(1 <= sizeof...(extras),
                    "conversion between string and time point must have at "
                    "least one extra argument for the format");
      std::optional<std::string_view> opt_fmt;
      std::optional<TimeZone> opt_zone;
      auto processArg = [&]<typename T>(const T& arg) {
        if constexpr (sameAsDecayed<TimePointFmt, T>()) {
          JMG_ENFORCE(!opt_fmt.has_value(),
                      "more than one format specified "
                      "when converting between string and time point");
          opt_fmt = unsafe(arg);
        }
        else if constexpr (sameAsDecayed<TimeZone, T>()) {
          JMG_ENFORCE(!opt_zone.has_value(),
                      "more than one time zone specified when converting "
                      "between string and time point");
          opt_zone = arg;
        }
        else { JMG_NOT_EXHAUSTIVE(T); }
      };
      (processArg(extras), ...);
      JMG_ENFORCE(opt_fmt.has_value(),
                  "no format specification provided for "
                  "converting between string and time point");
      fmt = *opt_fmt;
      // time zone defaults to UTC if not provided
      if (!opt_zone.has_value()) { zone = utcTimeZone(); }
      else { zone = *opt_zone; }
    }

    std::string_view fmt;
    TimeZone zone;
  };

  /**
   * convert a string-like object to a time point
   */
  static TimePoint str2TimePoint(Src str, Extras&&... extras) {
    const auto spec = TimePointConversionSpec(std::forward<Extras>(extras)...);
    // difficult to understand why the Abseil designers chose this
    // brain-dead signature that returns a bool with an error message
    // in an old-school result parameter for ParseTime and a perfectly
    // reasonable signature for FormatTime
    std::string errMsg;
    TimePoint rslt;
    JMG_ENFORCE(absl::ParseTime(spec.fmt, str, spec.zone, &rslt, &errMsg),
                "unable to parse string value [" << str
                                                 << "] as time point "
                                                    "using format ["
                                                 << spec.fmt
                                                 << "]: " << errMsg);
    return rslt;
  }

  static std::string timePoint2Str(Src tp, Extras&&... extras) {
    const auto spec = TimePointConversionSpec(std::forward<Extras>(extras)...);
    const auto rslt = absl::FormatTime(spec.fmt, tp, spec.zone);
    // quick sanity check since Abseil can't be trusted to do the
    // right thing and throw an exception...
    JMG_ENFORCE(!rslt.empty(), "unable to generate string value for time "
                               "point using format ["
                                 << spec.fmt << "]");
    return rslt;
  }
};

/**
 * helper class that is the return value of the 'from' function which
 * will bind to the receiving type to select the correct conversion
 *
 * Think of this class as a conduit that routes the arguments of the
 * 'from' function to the conversion code in ConvertImpl
 *
 * @warning don't try this at home ladies and gentlemen, we are
 *          trained professionals
 */
template<typename Src, typename... Extras>
class Converter {
public:
  /**
   * construct that stores the arguments of 'from' for later use in
   * the conversion operator
   */
  Converter(Src&& src, Extras&&... extras)
    : src_(std::forward<Src>(src)), extras_(std::forward<Extras>(extras)...) {}

  template<typename Tgt>
  operator Tgt() const&& {
    // do some fancy tuple footwork to collect the parts together in a
    // form that can be used to instantiate the correct specialization
    // of ConvertImpl
    auto args = std::tuple_cat(std::tuple(src_), extras_);
    auto redirect = [](auto... args) {
      return ConvertImpl<Tgt, Src, Extras...>::convert(args...);
    };
    return std::apply(redirect, args);
  }

private:
  Src src_;
  std::tuple<Extras...> extras_;
};

} // namespace detail

/**
 * convert from one type to another through the magic of return value
 * overloading
 */
template<typename Src, typename... Extras>
auto from(Src&& src, Extras&&... extras) {
  if constexpr (CStyleStringT<Src>) {
    // convert C-style strings to string_view to keep things simple
    return detail::Converter<std::string_view, Extras...>(
      std::string_view(src), std::forward<Extras>(extras)...);
  }
  else {
    return detail::Converter<Src, Extras...>(std::forward<Src>(src),
                                             std::forward<Extras>(extras)...);
  }
}

#undef DETAIL_ENFORCE_EMPTY_EXTRAS

} // namespace jmg
