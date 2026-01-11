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

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// concept for flag in type space (i.e. std::true_type or std::false_type
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept TypeFlagT =
  std::same_as<T, std::true_type> || std::same_as<T, std::false_type>;

////////////////////////////////////////////////////////////////////////////////
// helpers for decaying types
////////////////////////////////////////////////////////////////////////////////

/**
 * slightly less verbose version of std::remove_cvref_t that expresses
 * the wish that std::decay did not get involved with arrays and
 * pointers
 */
template<typename T>
using Decay = std::remove_cvref_t<T>;

template<typename T, typename U>
concept DecayedSameAsT = std::same_as<Decay<T>, Decay<U>>;

template<typename T, typename U>
concept SameAsDecayedT = std::same_as<T, Decay<U>>;

////////////////////////////////////////////////////////////////////////////////
// concept for types that are specializations of templates
////////////////////////////////////////////////////////////////////////////////

/**
 * @warning this will not work if there are non-type parameters in the pack
 */
template<typename T, template<typename...> typename Template>
concept TemplateSpecializationOfT =
  requires(Decay<T> t) { []<typename... Ts>(Template<Ts...>&) {}(t); };

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
concept IntegralT = std::integral<Decay<T>> && !std::same_as<bool, Decay<T>>;

template<typename T>
concept FloatingPointT = std::floating_point<Decay<T>>;

template<typename T>
concept ArithmeticT = IntegralT<T> || std::floating_point<Decay<T>>;

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
using RemoveOptionalT = detail::RemoveOptional<Decay<T>>::type;

////////////////////////////////////////////////////////////////////////////////
// concepts for enums
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept EnumT = std::is_enum_v<Decay<T>> && !std::is_scoped_enum_v<Decay<T>>;

template<typename T>
concept ScopedEnumT = std::is_scoped_enum_v<Decay<T>>;

////////////////////////////////////////////////////////////////////////////////
// concept for non-bool types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept NonBoolT = !SameAsDecayedT<bool, T>;

////////////////////////////////////////////////////////////////////////////////
// concepts for class and non-class types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept ClassT = std::is_class_v<Decay<T>>;

template<typename T>
concept NonClassT = !ClassT<T>;

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
concept SpanT = detail::IsSpanT<Decay<T>>{}();

/**
 * concept for vector
 */
template<typename T>
concept VectorT = TemplateSpecializationOfT<T, std::vector>;

////////////////////////////////////////////////////////////////////////////////
// concept for static string constant
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept StaticStringConstT =
  !SameAsDecayedT<char*, T> && !SameAsDecayedT<const char*, T>
  && std::is_array_v<Decay<T>>
  && std::same_as<std::remove_extent_t<Decay<T>>, char>
  && !std::same_as<std::remove_extent_t<Decay<T>>, const char>;

////////////////////////////////////////////////////////////////////////////////
// concepts for string-like types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept CStyleStringT =
  SameAsDecayedT<char*, T> || SameAsDecayedT<const char*, T>
  || std::same_as<std::remove_extent_t<Decay<T>>, char>
  || std::same_as<std::remove_extent_t<Decay<T>>, const char>;

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

namespace detail
{
template<typename T>
struct ReturnTypeFor {
  using type = Decay<T>;
};
template<ClassT T>
struct ReturnTypeFor<T> {
  using type = Decay<T>&;
};
} // namespace detail

template<typename T>
using ReturnTypeForT = detail::ReturnTypeFor<T>::type;

////////////////////////////////////////////////////////////////////////////////
// helper metafunctions for checking list membership and manipulating
// lists
////////////////////////////////////////////////////////////////////////////////

/**
 * decay all types in a type list
 */
template<TypeListT Lst>
using DecayAll = meta::transform<Lst, meta::quote<Decay>>;

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
template<typename T, TypeListT Lst>
inline constexpr bool isMemberOfList() {
  return detail::IsMemberOf<Decay<T>, DecayAll<Lst>>{}();
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

/**
 * determine if a type occurs exactly once in a list
 */
template<typename T, TypeListT Lst>
inline constexpr bool isUniqueMemberOfList() {
  return static_cast<size_t>(1)
         == detail::CountMatches<Decay<T>, DecayAll<Lst>>{};
}

/**
 * determine if a type occurs at most once in a list
 */
template<typename T, TypeListT Lst>
inline constexpr bool isAtMostOnceMemberOfList() {
  return detail::CountMatches<Decay<T>, DecayAll<Lst>>{} <= 1;
}

/**
 * find the index of the first entry of a type in a list
 */
template<typename T, TypeListT Lst>
inline constexpr size_t entryIdx() {
  static_assert(meta::in<Lst, T>{}(),
                "attempted to find the index of a type that is not present in "
                "the argument list");
  using Tail = meta::find<Lst, T>;
  return Lst::size() - Tail::size();
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
  if constexpr (SameAsDecayedT<std::string, T>) { return "std::string"; }
  return demangle(typeid(T));
}

/**
 * return the demangled name of the type of a value
 *
 * Intended for use in development and debugging.
 */
template<typename T>
std::string type_name_for(const T& t) {
  return type_name_for<Decay<T>>();
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
// detect if a type is a specialization of std::tuple
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename... Ts>
struct IsTupleSelect : std::false_type {};
template<typename... Ts>
struct IsTupleSelect<std::tuple<Ts...>> : std::true_type {};
} // namespace detail

template<typename T>
constexpr bool isTuple() {
  return detail::IsTupleSelect<T>{};
}

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
// extract the types in a tuple as a type list
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename... Ts>
struct DeTuplize {};

template<typename... Ts>
struct DeTuplize<std::tuple<Ts...>> {
  using type = meta::list<Ts...>;
};
} // namespace detail

template<typename... Ts>
using DeTuplize = typename detail::DeTuplize<Ts...>::type;

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
// helper macro for sinking exceptions
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
