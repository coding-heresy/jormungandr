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
#include <string>
#include <string_view>
#include <system_error>

#include "jmg/meta.h"
#include "jmg/preprocessor.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// return-type overload conversions from strings to other types
//
// Stolen from
// https://artificial-mind.net/blog/2020/10/10/return-type-overloading
////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct StringConvertImpl
{
  static_assert(always_false<T>, "conversion not supported");
};

template<NumericT T>
struct StringConvertImpl<T>
{
  static T from_string(const std::string_view str) {
    T rslt{};
    const auto [_, err] = std::from_chars(str.data(), str.data() + str.size(), rslt);
    JMG_ENFORCE(std::errc() == err, "unable to convert string value [" << str
		<< "] to integral value of type [" << type_name_for<T>() << "]: "
		<< std::make_error_code(err).message());
    return rslt;
  }
};

template<>
struct StringConvertImpl<std::string>
{
  static std::string from_string(const std::string_view str) {
    return std::string{str};
  }
};

////////////////////////////////////////////////////////////////////////////////
// add new implementations here
////////////////////////////////////////////////////////////////////////////////

/**
 * concept for valid conversion target types
 *
 * @warning this concept must be updated whenever a new conversion
 * implementation is added
 */
template<typename T>
concept ConversionTgtT = NumericT<T>
  || std::same_as<T, std::string>;

struct StringConvertT
{
  std::string_view str;

  template<ConversionTgtT T>
  operator T() const {
    return StringConvertImpl<T>::from_string(str);
  }
};

StringConvertT from_string(const std::string_view str) {
  return StringConvertT{str};
}

StringConvertT from_string(const std::string& str) {
  return from_string(std::string_view(str));
}

StringConvertT from_string(const char* str) {
  return from_string(std::string_view(str));
}

} // namespace jmg
