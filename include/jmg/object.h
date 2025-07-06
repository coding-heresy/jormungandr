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

template<typename...>
struct FieldGroupDef {};

namespace detail
{
template<typename T>
struct IsFieldGroupDef : std::false_type {};
template<typename... Ts>
struct IsFieldGroupDef<FieldGroupDef<Ts...>> : std::true_type {};
} // namespace detail

template<typename T>
concept FieldGroupDefT = detail::IsFieldGroupDef<T>{}();

////////////////////////////////////////////////////////////////////////////////
// concept that constrains a type to being a field or a field group
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept FieldOrGroupT = FieldDefT<T> || FieldGroupDefT<T>;

////////////////////////////////////////////////////////////////////////////////
// concept that constrains the Fields typelist associated with an
// Object to contain only fields or field groups
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct ValidObjectContentCheckUnwrap {
  static inline constexpr bool check() { return false; }
};
template<typename... Ts>
struct ValidObjectContentCheckUnwrap<meta::list<Ts...>> {
  static inline constexpr bool check() { return (FieldOrGroupT<Ts> || ...); }
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
// TODO(bd) call recursively in case a field group contains another
// field group
template<typename... Ts>
struct FieldExpanderImpl<FieldGroupDef<Ts...>> {
  using type = meta::list<Ts...>;
};
template<typename T>
using FieldExpander = meta::_t<FieldExpanderImpl<T>>;
} // namespace detail

template<TypeListT T>
using ExpandedFields =
  meta::join<meta::transform<T, meta::quote<detail::FieldExpander>>>;

////////////////////////////////////////////////////////////////////////////////
// declaration of an object
////////////////////////////////////////////////////////////////////////////////

template<FieldOrGroupT... Flds>
struct ObjectDef {
  using Fields = ExpandedFields<meta::list<Flds...>>;
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

template<FieldDefT T, ObjectDefT Obj>
inline constexpr bool isMemberOfObject() {
  return isMemberOfList<T, typename Obj::Fields>();
}

////////////////////////////////////////////////////////////////////////////////
// definitions of get() and try_get()
////////////////////////////////////////////////////////////////////////////////

// TODO(bd) figure out how to return string_view instead of string

template<RequiredField Fld, ObjectDefT Obj>
decltype(auto) get(const Obj& obj)
  requires(isMemberOfObject<Fld, Obj>())
{
  return obj.template get<Fld>();
}

template<OptionalField Fld, ObjectDefT Obj>
decltype(auto) try_get(const Obj& obj)
  requires(isMemberOfObject<Fld, Obj>())
{
  return obj.template try_get<Fld>();
}

/**
 * version of set() that will copy
 */
template<FieldDefT Fld, ObjectDefT Obj>
void set(Obj& obj, ArgTypeForT<Fld> arg)
  requires(isMemberOfObject<Fld, Obj>())
{
  obj.template set<Fld>(arg);
}

/**
 * version of set() that will copy from underlying type of viewable
 * type
 */
template<ViewableFieldT Fld, ObjectDefT Obj>
void set(Obj& obj, const typename Fld::type& arg)
  requires(isMemberOfObject<Fld, Obj>())
{
  obj.template set<Fld>(arg);
}

/**
 * special case of set() for string field (optional or required) that
 * will copy from C-style string
 */
template<StringFieldT Fld, ObjectDefT Obj>
void set(Obj& obj, const char* arg)
  requires(isMemberOfObject<Fld, Obj>())
{
  obj.template set<Fld>(arg);
}

/**
 * version of set() that will move
 */
template<FieldDefT Fld, ObjectDefT Obj>
void set(Obj& obj, typename Fld::type&& arg)
  requires(isMemberOfObject<Fld, Obj>() && ClassT<typename Fld::type>)
{
  obj.template set<Fld>(std::move(arg));
}

// TODO(bd) add special case to allow array fields to be set from
// std::array?

} // namespace jmg
