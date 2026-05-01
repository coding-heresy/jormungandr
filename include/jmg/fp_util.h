/** -*- mode: c++ -*-
 *
 * Copyright (C) 2026 Brian Davis
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

namespace jmg::ieee754
{
////////////////////
// IEEE 754

// single precision (32 bits) has 1 sign bit (position 31), 8 exponent
// bits (positions 23 through 30) and 23 fraction bits (positions 0
// through 22)

// double precision (64 bits) has 1 sign bit (position 63), 11
// exponent bits (positions 52 through 62) and 52 fraction bit
// (positions 0 through 51)

namespace detail
{

template<FloatingPointT T>
struct OctetsTypeFor {
  using type = uint32_t;
};

template<>
struct OctetsTypeFor<double> {
  using type = uint64_t;
};

template<FloatingPointT T>
struct ExpTypeFor {
  using type = uint8_t;
};

template<>
struct ExpTypeFor<double> {
  using type = uint16_t;
};

template<FloatingPointT T>
struct FracTypeFor {
  using type = int32_t;
};

template<>
struct FracTypeFor<double> {
  using type = int64_t;
};

} // namespace detail

template<FloatingPointT T>
using OctetsTypeForT = _T<detail::OctetsTypeFor<T>>;

template<FloatingPointT T>
using ExpTypeForT = _T<detail::ExpTypeFor<T>>;

template<FloatingPointT T>
using FracTypeForT = _T<detail::ExpTypeFor<T>>;

template<FloatingPointT T>
struct TypeTraits {
  using OctetType = OctetsTypeForT<T>;
  using ExpType = ExpTypeForT<T>;
  using FracType = FracTypeForT<T>;
};

#define JMG_BASE_TRAITS(type)        \
  using Base = TypeTraits<type>;     \
  using OctetType = Base::OctetType; \
  using ExpType = Base::ExpType;     \
  using FracType = Base::FracType

template<FloatingPointT T = float>
struct Traits {
  JMG_BASE_TRAITS(T);
  static constexpr size_t kExpOffset = 23;
  static constexpr auto kSignMask = OctetType(2147483648U);
  static constexpr auto kExpMask = OctetType(2139095040U);
  static constexpr auto kFracMask = OctetType(8388607U);
};

template<>
struct Traits<double> {
  JMG_BASE_TRAITS(double);
  static constexpr size_t kExpOffset = 52;
  static constexpr auto kSignMask = OctetType(9223372036854775808UL);
  static constexpr auto kExpMask = OctetType(9218868437227405312UL);
  static constexpr auto kFracMask = OctetType(4503599627370495UL);
};

#undef JMG_BASE_TRAITS

} // namespace jmg::ieee754
