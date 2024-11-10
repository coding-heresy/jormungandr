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

#include "jmg/meta.h"

// support for bitwise operations on scoped enum types

namespace jmg
{

#define MAKE_BINARY_OPS(op)                                              \
  template<ScopedEnumT T>                                                \
  inline constexpr T operator op(const T lhs, const T rhs) {             \
    using Ul = underlying_type_t<T>;                                     \
    return static_cast<T>(static_cast<Ul>(lhs) op static_cast<Ul>(rhs)); \
  }                                                                      \
  template<ScopedEnumT T>                                                \
  inline constexpr T& operator op #=(T & lhs, const T rhs) {             \
    using Ul = underlying_type_t<T>;                                     \
    lhs = static_cast<T>(static_cast<Ul>(lhs) op static_cast<Ul>(rhs));  \
    return lhs;                                                          \
  }

#define MAKE_SHIFT_OPS(op)                                              \
  template<ScopedEnumT T>                                               \
  inline constexpr T operator op(const T lhs, const size_t rhs) {       \
    using Ul = underlying_type_t<T>;                                    \
    return static_cast<T>(static_cast<Ul>(lhs) op rhs);                 \
  }                                                                     \
  template<ScopedEnumT T>                                               \
  inline constexpr T& operator op #=(T & lhs, const size_t rhs) {       \
    using Ul = underlying_type_t<T>;                                    \
    lhs = static_cast<T>(static_cast<Ul>(lhs) << static_cast<Ul>(rhs)); \
    return lhs;                                                         \
  }

MAKE_BINARY_OPS(|)
MAKE_BINARY_OPS(&)
MAKE_BINARY_OPS(^)

MAKE_SHIFT_OPS(<<)
MAKE_SHIFT_OPS(>>)

template<ScopedEnumT T>
inline constexpr T operator~(const T val) {
  using Ul = underlying_type_t<T>;
  return static_cast<T>(~(static_cast<Ul>(val)));
}

#undef MAKE_BINARY_OPS
#undef MAKE_SHIFT_OPS

} // namespace jmg
