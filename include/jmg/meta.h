#pragma once

#include <exception>
#include <memory>
#include <string>
#include <typeinfo>

#include <cxxabi.h>
#include <meta/meta.hpp>

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// concept for meta::list
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct IsTypeListImpl { using type = std::false_type; };

template<typename... Ts>
struct IsTypeListImpl<meta::list<Ts...>> { using type = std::true_type; };
} // namespace detail

template<typename T>
using IsTypeListT = meta::_t<detail::IsTypeListImpl<T>>;

template<typename T>
inline constexpr bool isTypeList() {
  return std::same_as<IsTypeListT<T>, std::true_type>;
}

template<typename T>
concept TypeList = isTypeList<T>();

////////////////////////////////////////////////////////////////////////////////
// concept for numeric types
////////////////////////////////////////////////////////////////////////////////
template<typename T>
concept NumericT = std::integral<T> || std::floating_point<T>;

////////////////////////////////////////////////////////////////////////////////
// concept for string-like types
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct IsStringLikeImpl { using type = std::false_type; };

template<>
struct IsStringLikeImpl<std::string> { using type = std::true_type; };

template<>
struct IsStringLikeImpl<std::string_view> { using type = std::true_type; };

template<>
struct IsStringLikeImpl<char*> { using type = std::true_type; };
} // namespace detail

template<typename T>
using IsStringLikeT = meta::_t<detail::IsStringLikeImpl<std::remove_cvref_t<T>>>;

template<typename T>
concept StringLikeT = IsStringLikeT<T>{}();

////////////////////////////////////////////////////////////////////////////////
// replacements for meta::front and meta::back that work on empty lists
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct safe_front_ { using type = meta::nil_; };
template <typename First, typename... Rest>
struct safe_front_<meta::list<First, Rest...>> { using type = First; };
template<typename T>
struct safe_back_ { using type = meta::nil_; };
template <typename First, typename... Rest>
struct safe_back_<meta::list<First, Rest...>> {
  using type = meta::at_c<meta::list<First, Rest...>, sizeof...(Rest)>;
};
} // namespace detail
template<TypeList T>
using safe_front = meta::_t<detail::safe_front_<T>>;
template<TypeList T>
using safe_back = meta::_t<detail::safe_back_<T>>;

////////////////////////////////////////////////////////////////////////////////
// support code for using policies to configure class templates at
// compile time
////////////////////////////////////////////////////////////////////////////////

// TODO find some way to detect when a policy in PolicyList doesn't
// resolve to any policy
namespace detail
{
template<typename BasePolicy, typename DefaultPolicy, TypeList PolicyList>
struct PolicyResolver
{
  using Recognizer = meta::bind_front<meta::quote<std::is_base_of>, BasePolicy>;
  using SearchResult = meta::find_if<PolicyList, Recognizer>;
  using SearchFailed = meta::empty<SearchResult>;
  using type = meta::if_<SearchFailed, DefaultPolicy, safe_front<SearchResult>>;
};
} // namespace detail

template<typename BasePolicy, typename DefaultPolicy, TypeList PolicyList>
using PolicyResolverT = meta::_t<detail::PolicyResolver<BasePolicy, DefaultPolicy, PolicyList>>;

namespace detail
{
using namespace meta::placeholders;
template<typename T>
using SameAsLambda = meta::lambda<_a, _b, meta::lazy::or_<_a, std::is_same<T, _b>>>;
template<typename T, TypeList Lst>
using IsMemberOf = meta::fold<Lst, std::false_type, SameAsLambda<T>>;
} // namespace detail
template<typename T, TypeList Lst>
inline constexpr bool isMemberOfList() {
  return detail::IsMemberOf<T, Lst>{}();
}

////////////////////////////////////////////////////////////////////////////////
// type name demangler functions
////////////////////////////////////////////////////////////////////////////////

/**
 * return the demangled name of a type specified by std::type_info
 *
 * Intended for use in development and debugging.
 */
std::string demangle(const std::type_info& id) {
  using namespace std;

  // c.f. https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html
  // for documentation of __cxa_demangle

  // note: -4 is not in the expected range of returned status values
  int status = -4;
  unique_ptr<char, void(*)(void*)> rslt{
    abi::__cxa_demangle(id.name(), nullptr, nullptr, &status), free};
  if (!rslt) {
    switch (status) {
    case 0:
      throw runtime_error("type name demangle returned success status but NULL result");
    case -1:
      throw runtime_error("unable to allocate memory for demangled type name");
    case -2:
      throw runtime_error("unexpected invalid type name when demangling");
    case -3:
      throw runtime_error("invalid argument to typename demangling function");
    default:
      throw runtime_error("unknown status result from failed typename demangle");
    }
  }
  // NOTE: ignoring the possibility that status could be nonzero when
  // __cxa_demangle() returns a non-NULL result
  return rslt.get();
}

/**
 * convenience overload of demangle for pointer argument
 */
std::string demangle(const std::type_info* id) {
  return demangle(*id);
}

/**
 * return the demangled name of a type
 *
 * Intended for use in development and debugging.
 */
template<typename T>
std::string type_name_for() {
  return demangle(typeid(T));
}

/**
 * return the demangled name of the type of a value
 *
 * Intended for use in development and debugging.
 */
template<typename T>
std::string type_name_for(const T& t) {
  return type_name_for<std::remove_cvref_t<T>>();
}

/**
 * return the demangled name of the current exception
 */
std::string current_exception_type_name() {
  const auto* excType = abi::__cxa_current_exception_type();
  if (!excType) {
    return "<no outstanding exceptions>";
  }
  return demangle(excType);
}

} // namespace jmg
