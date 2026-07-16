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

/**
 * this file contains workarounds for standard library features that
 * may not yet have constexpr versions but for which constexpr
 * versions are trivial to write
 */

#if !defined(__cpp_lib_ascii)
namespace jmg_std
{
// replicates std::ascii::is_lower
constexpr bool is_lower(char c) noexcept { return c >= 'a' && c <= 'z'; }

// replicates std::ascii::is_upper
constexpr char is_upper(char c) noexcept { return c >= 'A' && c <= 'Z'; }

// replicates std::ascii::to_lower
constexpr char to_lower(char c) noexcept {
  return is_upper(c) ? static_cast<char>(c + 32) : c;
}

// replicates std::ascii::to_upper
constexpr char to_upper(char c) noexcept {
  return is_lower(c) ? static_cast<char>(c - 32) : c;
}
} // namespace jmg_std
#else
#include <ascii>
namespace jmg_std
{
using is_lower = std::ascii::is_lower;
using is_upper = std::ascii::is_upper;
using to_lower = std::ascii::to_lower;
using to_upper = std::ascii::to_upper;
} // namespace jmg_std
#endif
