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

#include "jmg/meta.h"

namespace jmg
{

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
 */
template<typename T, StrLiteral kName, TypeFlagT IsRequired>
struct FieldDef : public detail::FieldDefTag {
  static constexpr auto name = kName.value;
  using type = T;
  using required = IsRequired;
};

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
template<typename T, TypeFlagT Required>
struct Optionalize {
  using type = std::remove_cvref_t<T>;
};

template<typename T>
struct Optionalize<T, std::false_type> {
  using type = std::optional<std::remove_cvref_t<T>>;
};

template<FieldDefT Fld>
using OptionalizedFldType =
  Optionalize<typename Fld::type, typename Fld::required>;
} // namespace detail

using Optionalize = meta::quote_trait<detail::OptionalizedFldType>;

/**
 * common field name constant for fields that have no string name
 */
inline constexpr char kPlaceholder[] = "";

} // namespace jmg
