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

#include <concepts>

#include <meta/meta.hpp>

#include "jmg/field.h"
#include "jmg/meta.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// declaration of a field group
////////////////////////////////////////////////////////////////////////////////

template<FieldDefT...>
struct FieldGroupDef {};

namespace detail
{
template<typename T>
struct IsFieldGroupDefImpl {
  using type = std::false_type;
};

template<typename... Ts>
struct IsFieldGroupDefImpl<FieldGroupDef<Ts...>> {
  using type = std::true_type;
};
} // namespace detail

template<typename T>
using IsFieldGroupDefT = meta::_t<detail::IsFieldGroupDefImpl<T>>;

template<typename T>
concept FieldGroupDefT = IsFieldGroupDefT<T>
{}();

////////////////////////////////////////////////////////////////////////////////
// concept that constrains a type to being a field or a field group
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct IsFieldOrGroupDefImpl {
  using type = std::false_type;
};
template<FieldDefT T>
struct IsFieldOrGroupDefImpl<T> {
  using type = std::true_type;
};
template<FieldGroupDefT T>
struct IsFieldOrGroupDefImpl<T> {
  using type = std::true_type;
};
}; // namespace detail
template<typename T>
using IsFieldOrGroupDefT = meta::_t<detail::IsFieldOrGroupDefImpl<T>>;

template<typename T>
concept FieldOrGroupT = FieldDefT<T> || FieldGroupDefT<T>;

template<typename T>
inline constexpr bool isFieldOrGroup() {
  return std::same_as<IsFieldOrGroupDefT<T>, std::true_type>;
}

////////////////////////////////////////////////////////////////////////////////
// concept that constrains an object to have valid fields
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct ValidObjectContentCheckUnwrap {
  static inline constexpr bool check() { return false; }
};
template<typename... Ts>
struct ValidObjectContentCheckUnwrap<meta::list<Ts...>> {
  static inline constexpr bool check() { return (isFieldOrGroup<Ts>() || ...); }
};
} // namespace detail

template<typename T>
inline constexpr bool isValidObjectContent() {
  return detail::ValidObjectContentCheckUnwrap<T>::check();
}
template<typename T>
concept ValidObjectContent = isValidObjectContent<T>();

////////////////////////////////////////////////////////////////////////////////
// type metafunction that expands field groups within an object
// declaration to produce a flattened list of fields
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct FieldExpanderImpl {
  using type = meta::list<T>;
};
// TODO call recursively in case a field group contains another field group
template<typename... Ts>
struct FieldExpanderImpl<FieldGroupDef<Ts...>> {
  using type = meta::list<Ts...>;
};
} // namespace detail
template<typename T>
using FieldExpanderT = meta::_t<detail::FieldExpanderImpl<T>>;

template<TypeList T>
using ExpandedFields =
  meta::join<meta::transform<T, meta::quote<FieldExpanderT>>>;

////////////////////////////////////////////////////////////////////////////////
// declaration of an object
////////////////////////////////////////////////////////////////////////////////

template<FieldOrGroupT... FieldsT>
struct ObjectDef {
  using Fields = ExpandedFields<meta::list<FieldsT...>>;
};

////////////////////////////////////////////////////////////////////////////////
// concept that constrains an object to be valid
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
concept HasFields = requires { typename T::Fields; };
template<typename T>
concept HasValidContent = ValidObjectContent<typename T::Fields>;
} // namespace detail

template<typename T>
concept ObjectDefT = detail::HasFields<T> && detail::HasValidContent<T>;

////////////////////////////////////////////////////////////////////////////////
// compile-time function used to prevent attempts to access or modify
// a field that is not declared to be contained by an object
////////////////////////////////////////////////////////////////////////////////

template<FieldDefT T, ObjectDefT ObjT>
inline constexpr bool isMemberOfObject() {
  return isMemberOfList<T, typename ObjT::Fields>();
}

////////////////////////////////////////////////////////////////////////////////
// definitions of get() and try_get()
////////////////////////////////////////////////////////////////////////////////

template<FieldDefT FieldT, ObjectDefT ObjectT>
typename FieldT::type get(const ObjectT& obj)
  requires(isMemberOfObject<FieldT, ObjectT>())
{
  return obj.template get<FieldT>();
}

template<FieldDefT FieldT, ObjectDefT ObjectT>
std::optional<typename FieldT::type> try_get(const ObjectT& obj)
  requires(isMemberOfObject<FieldT, ObjectT>())
{
  return obj.template try_get<FieldT>();
}

} // namespace jmg
