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

namespace jmg
{

/**
 * class template for a union that allows Jormungandr proxy objects to
 * be retrieved for native objects in a way that is integrated into
 * the get/try_get framework
 *
 * @todo add concepts to constrain the specialization to appropriate
 * types
 */
template<typename Obj, typename... Ts>
class Union
{
  using Alternates = meta::list<Ts...>;
public:
  Union() = delete;
  explicit Union(const Obj& obj) : obj_(&obj) {}

  template<typename Tgt>
  Tgt as() const requires (isMemberOfList<Tgt, Alternates>()) {
    return Tgt(*obj_);
  }

private:
  const Obj* obj_;
};

////////////////////////////////////////////////////////////////////////////////
// Union concept
////////////////////////////////////////////////////////////////////////////////
namespace detail
{
template<typename T>
struct IsUnionImpl { using type = std::false_type; };
template<typename T, typename... Ts>
struct IsUnionImpl<Union<T, Ts...>> { using type = std::true_type; };
} // namespace detail

template<typename T>
using IsUnionT = meta::_t<detail::IsUnionImpl<T>>;

template<typename T>
concept UnionT = IsUnionT<T>{}();
} // namespace jmg
