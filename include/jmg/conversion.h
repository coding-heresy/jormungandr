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

#include "jmg/meta.h"
#include "jmg/preprocessor.h"
#include "jmg/types.h"

#define DETAIL_ENFORCE_EMPTY_EXTRAS(extras, src_type, tgt_type)         \
  static_assert(0 == sizeof...(extras), "extra arguments are not supported " \
                "for conversion from " #src_type " to " #tgt_type)

namespace jmg
{

template<typename Tgt, typename Src, typename... Extras>
struct ConvertImpl {
  static_assert(always_false<Tgt>, "conversion not supported");
};

/**
 * implementation of conversions from string-like types to other types
 * (including other string-like types)
 */
template<typename Tgt, StringLikeT Src, typename... Extras>
struct ConvertImpl<Tgt, Src, Extras...> {
  static Tgt convert(const Src str, Extras... extras) {
    // convert string-like type to itself
    if constexpr (std::same_as<Tgt, std::remove_cvref_t<Src>>) {
      DETAIL_ENFORCE_EMPTY_EXTRAS(Extras, string, numeric);
      return str;
    }
    // convert string-like type to std::string or std:;string_view
    // (but not itself, which was handled in the previous case)
    else if constexpr(std::same_as<Tgt, std::string> ||
                      std::same_as<Tgt, std::string_view>) {
      DETAIL_ENFORCE_EMPTY_EXTRAS(Extras, string, string);
      return Tgt(str);
    }
    // convert string-like type to numeric type
    else if constexpr (NumericT<Tgt>) {
      DETAIL_ENFORCE_EMPTY_EXTRAS(Extras, string, numeric);
      auto rslt = Tgt();
      const auto [_, err] =
        std::from_chars(str.data(), str.data() + str.size(), rslt);
      JMG_ENFORCE(std::errc() == err, "unable to convert string value ["
                  << str << "] to integral value of type ["
                  << type_name_for<Tgt>() << "]: "
                  << std::make_error_code(err).message());
      return rslt;
    }
    // convert string-like type to time point
    else if constexpr (std::same_as<Tgt, TimePoint>) {
      static_assert(1 <= sizeof...(extras), "conversion from string to time "
                    "point must have at least one extra argument for the "
                    "format");
      std::optional<std::string_view> fmt;
      std::optional<TimeZone> zone;
      auto processArg = [&]<typename T>(const T& arg) {
        if constexpr (std::same_as<TimePointFmt, std::remove_cvref_t<T>>) {
          JMG_ENFORCE(!fmt.has_value(), "more than one format specified when "
                      "converting from string to time point");
          fmt = unsafe(arg);
        }
        else if constexpr (std::same_as<TimeZone, std::remove_cvref_t<T>>) {
          JMG_ENFORCE(!zone.has_value(), "more than one time zone specified "
                      "when converting from string to time point");
          zone = arg;
        }
        else {
          JMG_NOT_EXHAUSTIVE(T);
        }
      };
      (processArg(extras), ...);
      JMG_ENFORCE(fmt.has_value(), "no format specification provided for "
                  "converting string to time point");
      // time zone defaults to UTC if not provided
      if (!zone.has_value()) { zone = utcTimeZone(); }
      std::string errMsg;
      TimePoint rslt;
      JMG_ENFORCE(absl::ParseTime(*fmt, str, *zone, &rslt, &errMsg),
                  "unable to parse string value [" << str << "] as time point "
                  "using format [" << *fmt << "]: " << errMsg);
      return rslt;
    }
    else {
      JMG_NOT_EXHAUSTIVE(Tgt);
    }
  }
};

/**
 * concept for valid string conversion target types
 *
 * @warning this concept must be updated whenever a new conversion
 * implementation is added
 */
template<typename T>
concept StrConvTgtT = NumericT<T> || std::same_as<T, std::string> ||
  std::same_as<T, std::string_view> || std::same_as<T, TimePoint>;

/**
 * converter class for conversions from strings to other types
 *
 * @note this is the return type for several overrides of the 'from'
 * conversion function
 */
template<typename... Extras>
class StrConverter {
public:
  StrConverter(const std::string_view str, Extras&&... extras)
    : str_(str), extras_(std::forward<Extras>(extras)...) {}

  template<StrConvTgtT Tgt>
  operator Tgt() const && {
    const auto args = std::tuple_cat(std::tuple(str_), extras_);
    auto redirect = [](auto... args) {
      return ConvertImpl<Tgt, std::string_view, Extras...>::convert(args...);
    };
    return std::apply(redirect, args);
  }

private:
  std::string_view str_;
  std::tuple<Extras...> extras_;
};

template<StringLikeT Src, typename... Extras>
StrConverter<Extras...> from(const Src& str, Extras&&... extras) {
  return StrConverter<Extras...>(str, std::forward<Extras>(extras)...);
}

#undef DETAIL_ENFORCE_EMPTY_EXTRAS

////////////////////////////////////////////////////////////////////////////////
// string output helpers
////////////////////////////////////////////////////////////////////////////////

template<typename T>
std::ostream& operator<<(std::ostream& strm, const std::optional<T> val) {
  if (!val.has_value()) { strm << "<empty>"; }
  else { strm << *val; }
  return strm;
}

template<typename... Ts>
std::ostream& operator<<(std::ostream& strm, const std::tuple<Ts...>& tpl) {
  apply(
    [&](const auto& arg, const auto&... args) {
      strm << arg;
      ((strm << "," << args), ...);
    },
    tpl);
  return strm;
}

} // namespace jmg
