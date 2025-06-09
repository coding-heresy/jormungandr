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
#include <optional>
#include <span>
#include <string_view>

#include "jmg/meta.h"
#include "jmg/safe_types.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// definition of field
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
/**
 * tag type used to indicate that a type is a field definition
 */
struct FieldDefTag {};
} // namespace detail

/**
 * declaration of a data field
 * @tparam T type of data associated with the field
 * @tparam kName string name of the field
 * @tparam IsRequired indicates if the field is required or optional
 *
 * TODO(bd) constrain T further to prevent FieldDef from being used
 * for string or array types
 */
template<typename T, StrLiteral kName, TypeFlagT IsRequired>
struct FieldDef : public detail::FieldDefTag {
  static constexpr auto name = kName.value;
  using type = T;
  using required = IsRequired;
};

/**
 * special handling for fields containing strings
 * @tparam kName string name of the field
 * @tparam IsRequired indicates if the field is required or optional
 */
template<StrLiteral kName, TypeFlagT IsRequired>
struct StringField : public FieldDef<std::string, kName, IsRequired> {
  using view_type = std::string_view;
};

/**
 * special handling for fields containing arrays
 * @tparam T type of data held in the array
 * @tparam kName string name of the field
 * @tparam IsRequired indicates if the field is required or optional
 *
 * TODO(bd) needs more constraints on T?
 */
template<typename T, StrLiteral kName, TypeFlagT IsRequired>
struct ArrayField : public FieldDef<std::vector<T>, kName, IsRequired> {
  using view_type = std::span<T>;
};

////////////////////////////////////////////////////////////////////////////////
// concepts related to fields
////////////////////////////////////////////////////////////////////////////////

namespace detail
{

/**
 * field definition must have a 'name' static data member that is a
 * string constant.
 */
template<typename T>
concept HasFieldName =
  requires { std::same_as<decltype(T::name), char const*>; };

/**
 * field definition must have a 'type' member
 */
template<typename T>
concept HasFieldType = requires { typename T::type; };

/**
 * field definition must have a 'required' member
 */
template<typename T>
concept HasRequiredSpec = requires { typename T::required; };

template<typename T>
struct IsStringField : std::false_type {};

template<StrLiteral kName, TypeFlagT IsRequired>
struct IsStringField<StringField<kName, IsRequired>> : std::true_type {};

template<typename T>
struct IsArrayField : std::false_type {};

template<typename T, StrLiteral kName, TypeFlagT IsRequired>
struct IsArrayField<ArrayField<T, kName, IsRequired>> : std::true_type {};

} // namespace detail

/**
 * Concept for field definition
 */
template<typename T>
concept FieldDefT =
  std::derived_from<T, detail::FieldDefTag> && detail::HasFieldName<T>
  && detail::HasFieldType<T> && detail::HasRequiredSpec<T>;

/**
 * concept for required field, used by jmg::get
 */
template<typename T>
concept RequiredField =
  FieldDefT<T> && std::same_as<typename T::required, std::true_type>;

/**
 * concept for optional field, used by jmg::try_get
 */
template<typename T>
concept OptionalField =
  FieldDefT<T> && std::same_as<typename T::required, std::false_type>;

/**
 * concept for string field
 */
template<typename T>
concept StringFieldT = detail::IsStringField<T>{}();

/**
 * concept for array field
 */
template<typename T>
concept ArrayFieldT = detail::IsArrayField<T>{}();

/**
 * concept for non-viewable field (i.e. field that has non-viewable
 * type)
 */
template<typename T>
concept NonViewableFieldT =
  FieldDefT<T> && !(StringFieldT<T> || ArrayFieldT<T>);

/**
 * concept for viewable field (i.e. field that has viewable type)
 */
template<typename T>
concept ViewableFieldT = StringFieldT<T> || ArrayFieldT<T>;

/**
 * concept for optional non-viewable field (i.e. optional field that
 * has non-viewable type)
 */
template<typename T>
concept OptionalNonViewableFieldT =
  OptionalField<T> && !(StringFieldT<T> || ArrayFieldT<T>);

/**
 * concept for optional viewable field (i.e. optional field that has
 * viewable type)
 */
template<typename T>
concept OptionalViewableFieldT =
  OptionalField<T> && (StringFieldT<T> || ArrayFieldT<T>);

/**
 * concept for optional string field
 */
template<typename T>
concept OptionalStringFieldT = OptionalField<T> && StringFieldT<T>;

////////////////////////////////////////////////////////////////////////////////
// self-documenting aliases to be used when indicating whether a field
// is required
////////////////////////////////////////////////////////////////////////////////

using Required = std::true_type;
using Optional = std::false_type;

////////////////////////////////////////////////////////////////////////////////
// helpers for computing the effective type associated with a field
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T, TypeFlagT>
struct Optionalize {
  using type = std::remove_cvref_t<T>;
};

template<typename T>
struct Optionalize<T, Optional> {
  using type = std::optional<std::remove_cvref_t<T>>;
};

template<FieldDefT Fld>
using OptionalizedFldType =
  Optionalize<typename Fld::type, typename Fld::required>;
} // namespace detail

using Optionalize = meta::quote_trait<detail::OptionalizedFldType>;

////////////////////////////////////////////////////////////////////////////////
// type metafunction for calculating the correct argument type to use
// when setting a value for a field
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
concept RequiredNonClass = !ClassT<typename T::type> && RequiredField<T>;

template<typename T>
concept RequiredClass = ClassT<typename T::type> && RequiredField<T>
                        && !StringFieldT<T> && !ArrayFieldT<T>;

template<typename T>
concept RequiredViewable =
  RequiredField<T> && (StringFieldT<T> || ArrayFieldT<T>);

template<typename T>
concept OptionalNonViewable =
  OptionalField<T> && !StringFieldT<T> && !ArrayFieldT<T>;

template<typename T>
concept OptionalViewable =
  OptionalField<T> && (StringFieldT<T> || ArrayFieldT<T>);

template<typename T>
struct ArgTypeFor {
  using type = const T::type;
};

template<RequiredClass T>
struct ArgTypeFor<T> {
  using type = const T::type&;
};

template<RequiredViewable T>
struct ArgTypeFor<T> {
  using type = const T::view_type;
};

template<OptionalField T>
struct ArgTypeFor<T> {
  using type = const std::optional<typename T::type>&;
};

template<typename T>
struct ReturnTypeForField {
  using type = ReturnTypeForAnyT<typename T::type>;
};

template<RequiredViewable T>
struct ReturnTypeForField<T> {
  using type = typename T::view_type;
};

template<OptionalNonViewable T>
struct ReturnTypeForField<T> : public ArgTypeFor<T> {};

template<OptionalViewable T>
struct ReturnTypeForField<T> {
  using type = std::optional<typename T::view_type>;
};

} // namespace detail

template<typename T>
using ArgTypeForT = detail::ArgTypeFor<T>::type;

template<typename T>
using ReturnTypeForField = detail::ReturnTypeForField<T>::type;

/**
 * common field name constant for fields that have no string name
 */
inline constexpr char kPlaceholder[] = "";

} // namespace jmg
