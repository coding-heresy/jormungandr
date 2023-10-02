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
