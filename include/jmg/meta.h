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
#include <optional>
#include <string>
#include <type_traits>
#include <typeinfo>

#include <cxxabi.h>
#include <meta/meta.hpp>

#include "jmg/safe_types.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// concept for flag in type space (i.e. std::true_type or std::false_type
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept TypeFlagT =
  std::same_as<T, std::true_type> || std::same_as<T, std::false_type>;

////////////////////////////////////////////////////////////////////////////////
// helpers for decaying types (i.e. removing const, volatile and reference
////////////////////////////////////////////////////////////////////////////////

/**
 * slightly less verbose version of std::remove_cvref_t that expresses
 * the wish that std::decay did not get involved with arrays and
 * pointers
 */
template<typename T>
using DecayT = std::remove_cvref_t<T>;

template<typename T, typename U>
concept DecayedSameAsT = std::same_as<DecayT<T>, DecayT<U>>;

template<typename T, typename U>
concept SameAsDecayedT = std::same_as<T, DecayT<U>>;

////////////////////////////////////////////////////////////////////////////////
// concept for types that are specializations of templates
////////////////////////////////////////////////////////////////////////////////

/**
 * @warning this will not work if there are non-type parameters in the pack
 */
template<typename T, template<typename...> typename Template>
concept TemplateSpecializationOfT =
  requires(DecayT<T> t) { []<typename... Ts>(Template<Ts...>&) {}(t); };

////////////////////////////////////////////////////////////////////////////////
// concept for meta::list
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept TypeListT = TemplateSpecializationOfT<T, meta::list>;

////////////////////////////////////////////////////////////////////////////////
// concepts for numeric types
////////////////////////////////////////////////////////////////////////////////

// NOTE: explicitly excluding bool from the set of integral types
template<typename T>
concept IntegralT = std::integral<DecayT<T>> && !std::same_as<bool, DecayT<T>>;

template<typename T>
concept FloatingPointT = std::floating_point<DecayT<T>>;

template<typename T>
concept ArithmeticT = IntegralT<T> || std::floating_point<DecayT<T>>;

////////////////////////////////////////////////////////////////////////////////
// concept for optional types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept OptionalT = TemplateSpecializationOfT<T, std::optional>;

////////////////////////////////////////////////////////////////////////////////
// remove optional to expose value_type
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct RemoveOptional {
  using type = T;
};

template<typename T>
struct RemoveOptional<std::optional<T>> {
  using type = T;
};
}; // namespace detail

template<typename T>
using RemoveOptionalT = detail::RemoveOptional<DecayT<T>>::type;

////////////////////////////////////////////////////////////////////////////////
// concepts for enums
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept EnumT = std::is_enum_v<DecayT<T>> && !std::is_scoped_enum_v<DecayT<T>>;

template<typename T>
concept ScopedEnumT = std::is_scoped_enum_v<DecayT<T>>;

template<typename T>
concept AnyEnumT =
  std::is_enum_v<DecayT<T>> || std::is_scoped_enum_v<DecayT<T>>;

////////////////////////////////////////////////////////////////////////////////
// concept for non-bool types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept NonBoolT = !SameAsDecayedT<bool, T>;

////////////////////////////////////////////////////////////////////////////////
// concepts for spans and vectors
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct IsSpanT : std::false_type {};
template<typename T>
struct IsSpanT<std::span<T>> : std::true_type {};
template<typename T, std::size_t kSz>
struct IsSpanT<std::span<T, kSz>> : std::true_type {};
} // namespace detail

/**
 * concept for span
 */
template<typename T>
concept SpanT = detail::IsSpanT<DecayT<T>>::value;

/**
 * concept for vector
 */
template<typename T>
concept VectorT = TemplateSpecializationOfT<T, std::vector>;

namespace detail
{
template<typename T>
struct IsStdArrayT : std::false_type {};
template<typename T, size_t kSz>
struct IsStdArrayT<std::array<T, kSz>> : std::true_type {};
} // namespace detail

/**
 * concept for array
 */
template<typename T>
concept ArrayT = std::is_array_v<T> || detail::IsStdArrayT<T>::value;

////////////////////////////////////////////////////////////////////////////////
// concepts for class and non-class types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept ClassT = std::is_class_v<DecayT<T>>;

template<typename T>
concept NonClassT = !ClassT<T>;

////////////////////////////////////////////////////////////////////////////////
// concept for static string constant
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept StaticStringConstT =
  !SameAsDecayedT<char*, T> && !SameAsDecayedT<const char*, T>
  && std::is_array_v<DecayT<T>>
  && std::same_as<std::remove_extent_t<DecayT<T>>, char>
  && !std::same_as<std::remove_extent_t<DecayT<T>>, const char>;

////////////////////////////////////////////////////////////////////////////////
// concepts for string-like types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept CStyleStringT =
  !SameAsDecayedT<char, T>
  && (SameAsDecayedT<char*, T> || SameAsDecayedT<const char*, T>
      || std::same_as<std::remove_extent_t<DecayT<T>>, char>
      || std::same_as<std::remove_extent_t<DecayT<T>>, const char>);

template<typename T>
concept StdStringLikeT =
  SameAsDecayedT<std::string, T> || SameAsDecayedT<std::string_view, T>;

template<typename T>
concept StringLikeT =
  SameAsDecayedT<std::string, T> || SameAsDecayedT<std::string_view, T>
  || CStyleStringT<T>;

////////////////////
// concepts that differentiate between string_view and all other
// string-like types with the purpose of carefully separating
// ownership from use and facilitating an idiom where the main
// implementation of many functions that work with strings will use
// string_view and overloaded versions of the function will delegate
// to the main implementation

/**
 * concept for string-like types other than string_view
 */
template<typename T>
concept NonViewStringT = StringLikeT<T> && !SameAsDecayedT<std::string_view, T>;

/**
 * concept for string_view
 */
template<typename T>
concept ViewStringT = SameAsDecayedT<std::string_view, T>;

/**
 * concept for any class type that is not a string
 *
 * This is used to allow for special handling such as being able to
 * return string_view for object fields containing strings
 */
template<typename T>
concept NonStringClassT = ClassT<T> && !StringLikeT<T>;

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
// metafunction that determines whether a type should be returned by
// value or by reference
////////////////////////////////////////////////////////////////////////////////

/**
 * tag class that can be used as a base class in order to enable
 * return by value for proxy types
 */
struct ProxyTag {};

/**
 * concept for JMG proxy types
 */
template<typename T>
concept ProxyT = std::derived_from<DecayT<T>, ProxyTag>;

/**
 * concept for classes, other than safe classes, that are returned by reference
 */
template<typename T>
concept UnsafeNonViewableOrProxyClassT =
  UnsafeClassT<T> &&
  // strings are viewable
  NonStringClassT<T> &&
  // spans, vectors and arrays are viewable
  !SpanT<T> && !VectorT<T> && !ArrayT<DecayT<T>> &&
  // proxy types are always returned by value
  !ProxyT<T>;

namespace detail
{
// all of these are returned by value
template<typename T>
struct ReturnTypeFor {
  using type = DecayT<T>;
};
template<AnyEnumT T>
struct ReturnTypeFor<T> {
  using type = DecayT<T>;
};
template<SafePrimitiveT T>
struct ReturnTypeFor<T> {
  using type = DecayT<T>;
};
template<StringLikeT T>
struct ReturnTypeFor<T> {
  using type = std::string_view;
};
template<VectorT T>
struct ReturnTypeFor<T> {
  using type = std::span<typename T::value_type>;
};
template<ArrayT T>
struct ReturnTypeFor<T> {
  using type = std::span<typename T::value_type>;
};
template<SpanT T>
struct ReturnTypeFor<T> {
  using type = std::span<typename T::value_type>;
};
// only these 2 are returned by reference
template<SafeClassT T>
struct ReturnTypeFor<T> {
  using type = DecayT<T>&;
};
template<UnsafeNonViewableOrProxyClassT T>
struct ReturnTypeFor<T> {
  using type = DecayT<T>&;
};
} // namespace detail

template<typename T>
using ReturnTypeForT = detail::ReturnTypeFor<T>::type;

////////////////////////////////////////////////////////////////////////////////
// concepts and metafunctions for checking list membership and
// manipulating lists
////////////////////////////////////////////////////////////////////////////////

/**
 * decay all types in a type list
 */
template<TypeListT Lst>
using DecayAllT = meta::transform<Lst, meta::quote<DecayT>>;

namespace detail
{
using namespace meta::placeholders;
template<typename T>
using SameAsLambda =
  meta::lambda<_a, _b, meta::lazy::or_<_a, std::is_same<T, _b>>>;
template<typename T, TypeListT Lst>
using IsMemberOf = meta::fold<Lst, std::false_type, SameAsLambda<T>>;
} // namespace detail

/**
 * determine if a type is a member of a list
 */
template<typename T, typename Lst>
concept MemberOfListT =
  TypeListT<Lst> && detail::IsMemberOf<DecayT<T>, DecayAllT<Lst>>::value;

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

/**
 * determine if a type occurs exactly once in a list
 */
template<typename T, TypeListT Lst>
inline constexpr bool isUniqueMemberOfList() {
  return static_cast<size_t>(1)
         == detail::CountMatches<DecayT<T>, DecayAllT<Lst>>{};
}

/**
 * determine if a type occurs at most once in a list
 */
template<typename T, TypeListT Lst>
inline constexpr bool isAtMostOnceMemberOfList() {
  return detail::CountMatches<DecayT<T>, DecayAllT<Lst>>{} <= 1;
}

/**
 * find the index of the first entry of a type in a list
 */
template<typename T, TypeListT Lst>
inline constexpr size_t entryIdx() {
  static_assert(meta::in<Lst, T>::value,
                "attempted to find the index of a type that is not present in "
                "the argument list");
  using Tail = meta::find<Lst, T>;
  return Lst::size() - Tail::size();
}

////////////////////////////////////////////////////////////////////////////////
// support code for using policies to configure class templates at
// compile time
////////////////////////////////////////////////////////////////////////////////

/**
 * tag that is used as the base class of policy tags to allow
 * PolicyResolverT to recognize them
 */
struct AspectPolicyTag {};

/**
 * concept for AspectPolicyTag
 */
template<typename T>
concept AspectPolicyTagT = std::derived_from<DecayT<T>, AspectPolicyTag>;

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

/**
 * type metaprogram that determines if a list of policies are all
 * correctly associated with an aspect of the class
 */
template<TypeListT AllTags, TypeListT PolicyList>
struct PolicyChecker {
  using NotSubclassPred =
    meta::bind_back<meta::quote<IsNotSubclassMemberOf>, AllTags>;
  using SubclassCheckResult = meta::find_if<PolicyList, NotSubclassPred>;
  using type = meta::empty<SubclassCheckResult>;
};
template<TypeListT AllTags, TypeListT PolicyList>
using PolicyListValidT = meta::_t<PolicyChecker<AllTags, PolicyList>>;

/**
 * implementation of the policy resolver type metafunction that is
 * declared outside of the "detail" namespace
 */
template<AspectPolicyTagT BasePolicy,
         AspectPolicyTagT DefaultPolicy,
         TypeListT AllTags,
         TypeListT PolicyList>
  requires(std::is_base_of_v<BasePolicy, DefaultPolicy>
           && MemberOfListT<BasePolicy, AllTags>
           && PolicyListValidT<AllTags, PolicyList>::value)
struct PolicyResolver {
  using Recognizer = meta::bind_front<meta::quote<std::is_base_of>, BasePolicy>;
  using SearchResult = meta::find_if<PolicyList, Recognizer>;
  using SearchFailed = meta::empty<SearchResult>;
  using type = meta::if_<SearchFailed, DefaultPolicy, safe_front<SearchResult>>;
};
} // namespace detail

/**
 * type metafunction that computes the "policy" type to be used to
 * specialize the object using the "aspect"
 */
template<AspectPolicyTagT BasePolicy,
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
  if constexpr (SameAsDecayedT<std::string, T>) { return "std::string"; }
  else if constexpr (SameAsDecayedT<std::optional<std::string>, T>) {
    return "std::optional<std::string>";
  }
  // the actual type of std::string_view is also annoying
  else if constexpr (SameAsDecayedT<std::string_view, T>) {
    return "std::string_view";
  }
  else if constexpr (SameAsDecayedT<std::optional<std::string_view>, T>) {
    return "std::optional<std::string_view>";
  }
  return demangle(typeid(T));
}

/**
 * return the demangled name of the type of a value
 *
 * Intended for use in development and debugging.
 */
template<typename T>
std::string type_name_for(const T& t) {
  return type_name_for<DecayT<T>>();
}

/**
 * return the demangled name of the type of a value
 *
 * Intended for use in development and debugging.
 */
template<typename T>
std::string non_decayed_type_name_for(const T& t) {
  return type_name_for<T>();
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
// use the parameter pack of a typelist to specialize a variadic template
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<template<typename...> typename Wrapper, typename... Ts>
struct Wrapify {};

template<template<typename...> typename Wrapper, typename... Ts>
struct Wrapify<Wrapper, meta::list<Ts...>> {
  using type = Wrapper<Ts...>;
};
} // namespace detail

template<template<typename...> typename Wrapper, TypeListT Lst>
using WrapifyT = typename detail::Wrapify<Wrapper, Lst>::type;

////////////////////////////////////////////////////////////////////////////////
// extract a typelist of the parameter pack from a specialized variadic template
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename... Ts>
struct DeWrapify {};

template<template<typename...> typename Wrapper, typename... Ts>
struct DeWrapify<Wrapper<Ts...>> {
  using type = meta::list<Ts...>;
};
} // namespace detail

template<typename T>
using DeWrapifyT = typename detail::DeWrapify<T>::type;

////////////////////////////////////////////////////////////////////////////////
// concepts and metafunctions related to std::tuple
////////////////////////////////////////////////////////////////////////////////

/**
 * concept for a specialization of std::tuple
 */
template<typename T>
concept TupleT = TemplateSpecializationOfT<T, std::tuple>;

/**
 * specialize std::tuple using a typelist
 */
template<TypeListT T>
using TuplizeT = WrapifyT<std::tuple, T>;

/**
 * convert the parameter pack used to specialize a std::tuple into a type list
 */
template<TupleT T>
using DeTuplizeT = DeWrapifyT<T>;

////////////////////////////////////////////////////////////////////////////////
// concepts and metafunctions related to std::variant
////////////////////////////////////////////////////////////////////////////////

/**
 * concept for a specialization of std::tuple
 */
template<typename T>
concept VariantT = TemplateSpecializationOfT<T, std::variant>;

/**
 * specialize std::variant using a typelist
 */
template<TypeListT T>
using VariantizeT = WrapifyT<std::variant, T>;

/**
 * convert the parameter pack used to specialize a std::variant into a type list
 */
template<VariantT T>
using DeVariantizeT = DeWrapifyT<T>;

////////////////////////////////////////////////////////////////////////////////
// helper macro for final 'else' case of 'if constexpr' case analysis
// over a type
////////////////////////////////////////////////////////////////////////////////
#define JMG_NOT_EXHAUSTIVE(type)                                          \
  do {                                                                    \
    static_assert(always_false<type>, "case analysis is not exhaustive"); \
  } while (0)

} // namespace jmg

////////////////////////////////////////////////////////////////////////////////
// helper macros for sinking exceptions
////////////////////////////////////////////////////////////////////////////////

#define JMG_SINK_ALL_EXCEPTIONS(location)                                  \
  catch (const std::exception& e) {                                        \
    std::cout << "caught exception at " << location << ": " << e.what()    \
              << "\n";                                                     \
  }                                                                        \
  catch (...) {                                                            \
    std::cout << "caught exception of type ["                              \
              << jmg::current_exception_type_name() << "] at " << location \
              << "\n";                                                     \
  }

/**
 * sink all exceptions for an app that use traditional semantics of sending output
 * to stdout for further processing in a pipeline and error messages to stderr
 */
#define JMG_SINK_ALL_EXCEPTIONS_TO_STDERR(location)                        \
  catch (const std::exception& e) {                                        \
    std::cerr << "caught exception at " << location << ": " << e.what()    \
              << "\n";                                                     \
  }                                                                        \
  catch (...) {                                                            \
    std::cerr << "caught exception of type ["                              \
              << jmg::current_exception_type_name() << "] at " << location \
              << "\n";                                                     \
  }
