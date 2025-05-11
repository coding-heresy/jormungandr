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

#include <st/st.hpp>

#include "jmg/meta.h"

#define JMG_NEW_SAFE_PROTOTYPE(name, ...)                      \
  template<::jmg::UnsafeT T, typename Tag = decltype([]() {})> \
  using name = ::st::type<T, Tag, __VA_ARGS__>

#define JMG_NEW_SAFE_BASE_TYPE(name, unsafe_type, ...) \
  template<typename Tag = decltype([]() {})>           \
  using name = ::st::type<unsafe_type, Tag, __VA_ARGS__>

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// concepts for safe types
////////////////////////////////////////////////////////////////////////////////

template<typename T>
concept SafeT = st::is_strong_type_v<T>;

template<typename T>
concept UnsafeT = (!st::is_strong_type_v<T>);

////////////////////////////////////////////////////////////////////////////////
// type aliases
////////////////////////////////////////////////////////////////////////////////

template<UnsafeT T, typename... Policies>
using SafeType = st::type<T, decltype([]() {}), Policies...>;

/**
 * prototype for an ID type, which must be equality_comparable, hashable
 * and orderable
 *
 * NOTE: ordering is not supported, so such IDs may be used as keys in
 * unordered data structures (e.g. std::unordered_map) but not ordered
 * data structures (e.g. std::map)
 */
JMG_NEW_SAFE_PROTOTYPE(SafeId, st::equality_comparable, st::hashable);

/**
 * base types for common cases of IDs: uint32_t, uint64_t and
 * std::string
 */
JMG_NEW_SAFE_BASE_TYPE(SafeId32,
                       uint32_t,
                       st::equality_comparable,
                       st::hashable,
                       st::orderable);
JMG_NEW_SAFE_BASE_TYPE(SafeId64,
                       uint64_t,
                       st::equality_comparable,
                       st::hashable,
                       st::orderable);
JMG_NEW_SAFE_BASE_TYPE(SafeIdStr,
                       std::string,
                       st::equality_comparable,
                       st::hashable,
                       st::orderable);

////////////////////////////////////////////////////////////////////////////////
// utility functions
////////////////////////////////////////////////////////////////////////////////

/**
 * 'unwrap' a safe type by converting it into the associated unsafe
 * type
 */
auto unsafe(SafeT auto safe) { return safe.value(); }

/**
 * metafunction that returns the unsafe type associated with a safe
 * type
 */
template<SafeT T>
using UnsafeTypeFromT = typename T::value_type;

/**
 * convert a reference from an unsafe type to a reference to a safe
 * type that wraps it
 */
template<SafeT T>
struct SafeRefOf {
  using Unsafe = T::value_type;
  // convert to non-const reference
  constexpr static T& from(Unsafe& ref) { return reinterpret_cast<T&>(ref); }
  // convert to const reference
  constexpr static const T& from(const Unsafe& ref) {
    return reinterpret_cast<const T&>(ref);
  }
};

namespace detail
{
template<SafeT T, typename U>
struct ReturnTypeForSafe {
  using type = std::remove_cvref_t<T>;
};
template<SafeT T, ClassT U>
struct ReturnTypeForSafe<T, U> {
  using type = std::remove_cvref_t<T>&;
};
} // namespace detail

template<typename T>
using ReturnTypeForSafeT =
  detail::ReturnTypeForSafe<T, typename T::value_type>::type;

namespace detail
{
template<typename T>
struct ReturnTypeForAny {
  using type = ReturnTypeForT<T>;
};
template<SafeT T>
struct ReturnTypeForAny<T> {
  using type = ReturnTypeForSafeT<T>;
};
} // namespace detail

template<typename T>
using ReturnTypeForAnyT = detail::ReturnTypeForAny<T>::type;

} // namespace jmg

/**
 * overload of operator<< for safe types
 */
template<jmg::SafeT T>
std::ostream& operator<<(std::ostream& strm, const T& val) {
  strm << jmg::unsafe(val);
  return strm;
}
