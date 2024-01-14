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

#define JMG_SAFE_TYPE(safe_type, unsafe_type, ...)	\
  using safe_type = st::type<unsafe_type,		\
    struct safe_type ## SafeTag, __VA_ARGS__)

#define JMG_SAFE_ID(safe_type, unsafe_type)	\
  using safe_type = st::type<unsafe_type,	\
    struct safe_type ## SafeTag,		\
    st::equality_comparable>

#define JMG_SAFE_ID_32(safe_type)		\
  using safe_type = st::type<uint32_t,		\
    struct safe_type ## SafeTag,		\
    st::equality_comparable>

#define JMG_SAFE_ID_64(safe_type)		\
  using safe_type = st::type<uint64_t,		\
    struct safe_type ## SafeTag,		\
    st::equality_comparable>

namespace jmg
{
template <typename T>
concept SafeType = st::is_strong_type_v<T>;

auto unsafe(SafeType auto safe) {
  return safe.value();
}

template<SafeType T>
using UnsafeTypeFromT = typename T::value_type;

} // namespace jmg

template<jmg::SafeType T>
std::ostream& operator<<(std::ostream& strm, const T& val) {
  strm << jmg::unsafe(val);
  return strm;
}
