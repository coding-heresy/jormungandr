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

#include <variant>

#include "jmg/field.h"
#include "jmg/meta.h"

namespace jmg
{

#if !defined(JMG_USE_BACKWARDS_COMPATIBLE_UNION)

/**
 * class template for a union that allows implementations like protobuf oneof to
 * be support by JMG get(), try_get() and set()
 *
 * TODO(bd) add concepts to constrain the specialization to appropriate
 * types
 */
template<RequiredFieldT... Flds>
class Union {
public:
  using fields = meta::list<Flds...>;
  using members = meta::transform<fields, meta::quote<meta::_t>>;
  using return_type =
    VariantizeT<meta::transform<DecayAllT<members>, meta::quote<ReturnTypeForT>>>;

  Union() = delete;
};

#else

/**
 * backwards-compatible version of Union that avoids breaking existing
 * functionality that is in limited use
 */
template<typename Obj, typename... Ts>
class Union {
public:
  using Alternates = meta::list<Ts...>;

  Union() = delete;
  explicit Union(const Obj& obj) : obj_(&obj) {}

  template<typename Tgt>
  Tgt as() const
    requires(MemberOfListT<Tgt, Alternates>)
  {
    return Tgt(*obj_);
  }

private:
  const Obj* obj_;
};

#endif

////////////////////////////////////////////////////////////////////////////////
// Union concept
////////////////////////////////////////////////////////////////////////////////
namespace detail
{
template<typename T>
struct IsUnion : std::false_type {};
template<typename T, typename... Ts>
struct IsUnion<Union<T, Ts...>> : std::true_type {};
} // namespace detail

template<typename T>
concept UnionT = detail::IsUnion<T>::value;

#if !defined(JMG_USE_BACKWARDS_COMPATIBLE_UNION)

////////////////////////////////////////////////////////////////////////////////
// concept for member of union
////////////////////////////////////////////////////////////////////////////////

template<typename U, typename T>
concept UnionMemberT = UnionT<U> && MemberOfListT<T, typename U::members>;

#endif

} // namespace jmg
