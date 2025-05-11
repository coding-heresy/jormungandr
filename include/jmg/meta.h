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

#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>

#include <cxxabi.h>
#include <meta/meta.hpp>

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// concept for type flag (i.e. std::true_type or std::false_type
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept TypeFlagT =
  std::same_as<T, std::true_type> || std::same_as<T, std::false_type>;

////////////////////////////////////////////////////////////////////////////////
// replacement for std::same_as that removes const, volatile and
// references
////////////////////////////////////////////////////////////////////////////////

template<typename TLhs, typename TRhs>
constexpr bool decayedSameAs() {
  return std::same_as<std::remove_cvref_t<TLhs>, std::remove_cvref_t<TRhs>>;
}

template<typename TLhs, typename TRhs>
constexpr bool sameAsDecayed() {
  return std::same_as<TLhs, std::remove_cvref_t<TRhs>>;
}

////////////////////////////////////////////////////////////////////////////////
// concept for meta::list
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct IsTypeList : std::false_type {};

template<typename... Ts>
struct IsTypeList<meta::list<Ts...>> : std::true_type {};
} // namespace detail

template<typename T>
concept TypeListT = detail::IsTypeList<T>{}();

////////////////////////////////////////////////////////////////////////////////
// concepts for numeric types
////////////////////////////////////////////////////////////////////////////////

// NOTE: explicitly excluding bool from the set of integral types
template<typename T>
concept IntegralT = std::integral<std::remove_cvref_t<T>>
                    && !std::same_as<bool, std::remove_cvref_t<T>>;

template<typename T>
concept FloatingPointT = std::floating_point<std::remove_cvref_t<T>>;

template<typename T>
concept ArithmeticT =
  IntegralT<T> || std::floating_point<std::remove_cvref_t<T>>;

////////////////////////////////////////////////////////////////////////////////
// concepts for enums
////////////////////////////////////////////////////////////////////////////////

#if defined(__cpp_lib_is_scoped_enum)
template<typename T>
concept EnumT = std::is_enum_v<std::remove_cvref_t<T>>
                && !std::is_scoped_enum_v<std::remove_cvref_t<T>>;

template<typename T>
concept ScopedEnumT = std::is_scoped_enum_v<std::remove_cvref_t<T>>;
#else
// TODO remove this section when the library requires c++23
namespace detail
{
template<typename E>
  struct is_scoped_enum : std::bool_constant < requires {
  requires std::is_enum_v<E>;
  requires !std::is_convertible_v<E, std::underlying_type_t<E>>;
} > {};
template<class T>
constexpr bool is_scoped_enum_v = is_scoped_enum<T>::value;
} // namespace detail

template<typename T>
concept EnumT = std::is_enum_v<std::remove_cvref_t<T>>
                && !detail::is_scoped_enum_v<std::remove_cvref_t<T>>;

template<typename T>
concept ScopedEnumT = detail::is_scoped_enum_v<std::remove_cvref_t<T>>;
#endif

////////////////////////////////////////////////////////////////////////////////
// concepts for string-like types
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct IsCStyleString : std::false_type {};
template<>
struct IsCStyleString<char*> : std::true_type {};
template<>
struct IsCStyleString<const char*> : std::true_type {};
template<int kSz>
struct IsCStyleString<char[kSz]> : std::true_type {};
template<int kSz>
struct IsCStyleString<const char[kSz]> : std::true_type {};

template<typename T>
struct IsStringLike : std::false_type {};
template<>
struct IsStringLike<std::string> : std::true_type {};
template<>
struct IsStringLike<std::string_view> : std::true_type {};
template<>
struct IsStringLike<char*> : std::true_type {};
template<>
struct IsStringLike<const char*> : std::true_type {};
template<int kSz>
struct IsStringLike<char[kSz]> : std::true_type {};
template<int kSz>
struct IsStringLike<const char[kSz]> : std::true_type {};
} // namespace detail

template<typename T>
concept CStyleStringT = detail::IsCStyleString<std::remove_cvref_t<T>>{}();

template<typename T>
concept StringLikeT = detail::IsStringLike<std::remove_cvref_t<T>>{}();

////////////////////////////////////////////////////////////////////////////////
// concept for non-bool types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept NonBoolT = !std::same_as<T, bool>;

////////////////////////////////////////////////////////////////////////////////
// concepts for class and non-class types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept ClassT = std::is_class_v<std::remove_cvref_t<T>>;

template<typename T>
concept NonClassT = !ClassT<T>;

////////////////////////////////////////////////////////////////////////////////
// always_false wrapper to defer evaluation of static_assert
//
// shamelessly stolen from
// https://artificial-mind.net/blog/2020/10/03/always-false
////////////////////////////////////////////////////////////////////////////////

template<typename... T>
constexpr bool always_false = false;

////////////////////////////////////////////////////////////////////////////////
// replacements for meta::front and meta::back that work on empty lists
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct safe_front_ {
  using type = meta::nil_;
};
template<typename First, typename... Rest>
struct safe_front_<meta::list<First, Rest...>> {
  using type = First;
};
template<typename T>
struct safe_back_ {
  using type = meta::nil_;
};
template<typename First, typename... Rest>
struct safe_back_<meta::list<First, Rest...>> {
  using type = meta::at_c<meta::list<First, Rest...>, sizeof...(Rest)>;
};
} // namespace detail
template<TypeListT T>
using safe_front = meta::_t<detail::safe_front_<T>>;
template<TypeListT T>
using safe_back = meta::_t<detail::safe_back_<T>>;

////////////////////////////////////////////////////////////////////////////////
// general use metafunctions
////////////////////////////////////////////////////////////////////////////////

template<TypeListT Lst>
using DecayAll = meta::transform<Lst, meta::quote<std::remove_cvref_t>>;

namespace detail
{
template<typename T>
struct ReturnTypeFor {
  using type = std::remove_cvref_t<T>;
};
template<ClassT T>
struct ReturnTypeFor<T> {
  using type = std::remove_cvref_t<T>&;
};
} // namespace detail

template<typename T>
using ReturnTypeForT = detail::ReturnTypeFor<T>::type;

////////////////////////////////////////////////////////////////////////////////
// helper metafunctions for checking list membership
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
using namespace meta::placeholders;
template<typename T>
using SameAsLambda =
  meta::lambda<_a, _b, meta::lazy::or_<_a, std::is_same<T, _b>>>;
template<typename T, TypeListT Lst>
using IsMemberOf = meta::fold<Lst, std::false_type, SameAsLambda<T>>;
} // namespace detail
template<typename T, TypeListT Lst>
inline constexpr bool isMemberOfList() {
  return detail::IsMemberOf<std::remove_cvref_t<T>, DecayAll<Lst>>{}();
}

namespace detail
{
using namespace meta::placeholders;
template<typename T1, typename T2>
struct Matched : std::integral_constant<size_t, 0> {};
template<typename T>
struct Matched<T, T> : std::integral_constant<size_t, 1> {};

template<typename T>
using CountMatchesLambda =
  meta::lambda<_a, _b, meta::lazy::plus<_a, Matched<T, _b>>>;

template<typename T, TypeListT Lst>
using CountMatches =
  meta::fold<Lst, std::integral_constant<size_t, 0>, CountMatchesLambda<T>>;
} // namespace detail
template<typename T, TypeListT Lst>
inline constexpr bool isUniqueMemberOfList() {
  return static_cast<size_t>(1) == detail::CountMatches<T, Lst>{};
}
template<typename T, TypeListT Lst>
inline constexpr bool entryCount() {
  return detail::CountMatches<T, Lst>{};
}

////////////////////////////////////////////////////////////////////////////////
// support code for using policies to configure class templates at
// compile time
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
using namespace meta::placeholders;
template<typename T>
using SubclassOfLambda =
  meta::lambda<_a, _b, meta::lazy::or_<_a, std::is_base_of<_b, T>>>;
template<typename T, TypeListT Lst>
using IsSubclassMemberOf =
  meta::fold<Lst, std::false_type, SubclassOfLambda<T>>;
template<typename T, TypeListT Lst>
using IsNotSubclassMemberOf = meta::not_<detail::IsSubclassMemberOf<T, Lst>>;

template<TypeListT AllTags, TypeListT PolicyList>
struct PolicyChecker {
  using NotSubclassPred =
    meta::bind_back<meta::quote<IsNotSubclassMemberOf>, AllTags>;
  using SubclassCheckResult = meta::find_if<PolicyList, NotSubclassPred>;
  using type = meta::empty<SubclassCheckResult>;
};
template<TypeListT AllTags, TypeListT PolicyList>
using PolicyListValidT = meta::_t<PolicyChecker<AllTags, PolicyList>>;

template<typename BasePolicy,
         typename DefaultPolicy,
         TypeListT AllTags,
         TypeListT PolicyList>
  requires(std::is_base_of_v<BasePolicy, DefaultPolicy>
           && isMemberOfList<BasePolicy, AllTags>()
           && PolicyListValidT<AllTags, PolicyList>{}())
struct PolicyResolver {
  using Recognizer = meta::bind_front<meta::quote<std::is_base_of>, BasePolicy>;
  using SearchResult = meta::find_if<PolicyList, Recognizer>;
  using SearchFailed = meta::empty<SearchResult>;
  using type = meta::if_<SearchFailed, DefaultPolicy, safe_front<SearchResult>>;
};
} // namespace detail

template<typename BasePolicy,
         typename DefaultPolicy,
         TypeListT AllTags,
         TypeListT PolicyList>
using PolicyResolverT =
  meta::_t<detail::PolicyResolver<BasePolicy, DefaultPolicy, AllTags, PolicyList>>;

////////////////////////////////////////////////////////////////////////////////
// type name demangler functions
////////////////////////////////////////////////////////////////////////////////

/**
 * return the demangled name of a type specified by std::type_info
 *
 * Intended for use in development and debugging.
 */
inline std::string demangle(const std::type_info& id) {
  // c.f.
  // https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html
  // for documentation of __cxa_demangle

  // note: -4 is not in the expected range of returned status values
  int status = -4;
  std::unique_ptr<char, void (*)(void*)> rslt{
    abi::__cxa_demangle(id.name(), nullptr, nullptr, &status), free};
  if (!rslt) {
    switch (status) {
      case 0:
        throw std::runtime_error(
          "type name demangle returned success status but NULL result");
      case -1:
        throw std::runtime_error(
          "unable to allocate memory for demangled type name");
      case -2:
        throw std::runtime_error(
          "unexpected invalid type name when demangling");
      case -3:
        throw std::runtime_error(
          "invalid argument to typename demangling function");
      default:
        throw std::runtime_error(
          "unknown status result from failed typename demangle");
    }
  }
  // NOTE: ignoring the possibility that status could be nonzero when
  // __cxa_demangle() returns a non-NULL result
  return rslt.get();
}

/**
 * convenience overload of demangle for pointer argument
 */
inline std::string demangle(const std::type_info* id) { return demangle(*id); }

/**
 * return the demangled name of a type
 *
 * Intended for use in development and debugging.
 */
template<typename T>
std::string type_name_for() {
  // the actual type of std::string is annoying
  if constexpr (std::same_as<T, std::string>) { return "std::string"; }
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
inline std::string current_exception_type_name() {
  const auto* excType = abi::__cxa_current_exception_type();
  if (!excType) { return "<no outstanding exceptions>"; }
  return demangle(excType);
}

////////////////////////////////////////////////////////////////////////////////
// holder for compile time string literal template parameter
////////////////////////////////////////////////////////////////////////////////

/**
 * compile-time string literal
 *
 * shamelessly ripped from
 * https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
 */
template<size_t N>
struct StrLiteral {
  constexpr StrLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }
  char value[N];
};

////////////////////////////////////////////////////////////////////////////////
// convert from type list to tuple
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename... Ts>
struct Tuplize {};

template<typename... Ts>
struct Tuplize<meta::list<Ts...>> {
  using type = std::tuple<Ts...>;
};
} // namespace detail

template<typename... Ts>
using Tuplize = typename detail::Tuplize<Ts...>::type;

////////////////////////////////////////////////////////////////////////////////
// helper macro for final 'else' case of 'if constexpr' case analysis
// over a type
////////////////////////////////////////////////////////////////////////////////
#define JMG_NOT_EXHAUSTIVE(type)                                          \
  do {                                                                    \
    static_assert(always_false<type>, "case analysis is not exhaustive"); \
  } while (0)

} // namespace jmg
