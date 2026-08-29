// clang-format Language: Cpp
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

#include <meta>
#include <ranges>
#include <tuple>

#include "compatibility.h"

namespace jmg
{

////////////////////
// owners for identifiers generated at compile time

/**
 * compile-time owner of a snake_case version of an identifier
 */
template<std::meta::info kIdentifiable>
class SnakeCaseIdOwner {
  static consteval auto make_snake_case() {
    constexpr auto id = std::meta::identifier_of(kIdentifiable);
    static_assert(id.size() > 1UZ, "snake_case conversion of empty or single "
                                   "letter strings is not supported");
    // TODO(bd) impose more restrictions on what constitutes a valid
    // identifier to convert to snake_case?
    static_assert(!jmg_std::is_upper(id[id.size() - 1]),
                  "snake_case conversion of a string ending with an uppercase "
                  "character is not supported");
    constexpr auto rsltSz = [&]() {
      // start with 1 result character for the required NULL
      // terminator and 1 result character for the first input
      // character, regardless of its case
      size_t rslt = 2;
      for (size_t idx = 1; idx < id.size(); ++idx) {
        // uppercase input characters require 2 output characters, all
        // other characters require 1 output character
        rslt += jmg_std::is_upper(id[idx]) ? 2UZ : 1UZ;
      }
      return rslt;
    }();
    auto rslt = std::array<char, rsltSz>{};
    // uppercase first character doesn't require preceding underscore
    rslt[0] = jmg_std::to_lower(id[0]);
    size_t offset = 1;
    for (size_t idx = 1; idx < id.size(); ++idx) {
      if (jmg_std::is_upper(id[idx]) && jmg_std::is_lower(id[idx - 1])) {
        rslt[offset++] = '_';
      }
      rslt[offset++] = jmg_std::to_lower(id[idx]);
    }
    rslt[rslt.size() - 1] = '\0';
    return rslt;
  }

public:
  /**
   * return a C-style string pointer to the identifier
   */
  static constexpr const char* c_str() {
    static constexpr auto owner = make_snake_case();
    return owner.data();
  }
};

namespace detail
{

/**
 * helper that generates either a PascalCase or a camelCase identifier
 * using the metadata for an entity
 */
template<std::meta::info kIdentifiable, bool kMakePascalCase = true>
consteval auto makePascalOrCamelCase() {
  static_assert(std::meta::has_identifier(kIdentifiable),
                "unable to generate PascalCase or camelCase identifier for "
                "entity that has no identifier");
  constexpr auto id = std::meta::identifier_of(kIdentifiable);
  static_assert(id.size() > 1UZ,
                "PascalCase or camelCase conversion of empty "
                "or single letter identifiers is not supported");
  // TODO(bd) impose more restrictions on what constitutes a valid
  // identifier to convert to PascalCase or camelCase?
  static_assert(
    ('_' != id[id.size() - 1] && '_' != id[0]),
    "PascalCase or camelCase conversion of a string beginning or ending "
    "with an underscore character '_' is not supported");
  constexpr auto rsltSz = [&]() {
    size_t rslt = 1; // always needs null terminator
    for (size_t idx = 0; idx < id.size(); ++idx) {
      // input underscore characters do not appear in the output
      rslt += ('_' == id[idx]) ? 0UZ : 1UZ;
    }
    return rslt;
  }();
  auto rslt = std::array<char, rsltSz>{};
  if constexpr (kMakePascalCase) {
    // first character is always uppercase
    rslt[0] = jmg_std::to_upper(id[0]);
  }
  else {
    // first character is always lowercase
    rslt[0] = jmg_std::to_lower(id[0]);
  }
  size_t offset = 1;
  for (size_t idx = 1; idx < id.size(); ++idx) {
    if ('_' == id[idx]) { continue; }
    if ('_' == id[idx - 1]) { rslt[offset++] = jmg_std::to_upper(id[idx]); }
    else { rslt[offset++] = id[idx]; }
  }
  rslt[rslt.size() - 1] = '\0';
  return rslt;
}

} // namespace detail

/**
 * compile-time owner of a PascalCase version of an identifier
 */
template<std::meta::info kIdentifiable>
struct PascalCaseIdOwner {
  /**
   * return a C-style string pointer to the identifier
   */
  static constexpr const char* c_str() {
    static constexpr auto owner =
      detail::makePascalOrCamelCase<kIdentifiable>();
    return owner.data();
  }
};

/**
 * compile-time owner of a camelCase version of an identifier
 */
template<std::meta::info kIdentifiable>
struct CamelCaseIdOwner {
  /**
   * return a C-style string pointer to the identifier
   */
  static constexpr const char* c_str() {
    static constexpr auto owner =
      detail::makePascalOrCamelCase<kIdentifiable, false /* kMakePascalCase */>();
    return owner.data();
  }
};

/**
 * concept for reflection metadata associated with a function
 */
template<std::meta::info Meta>
concept FcnMetaT = std::meta::is_function(Meta);

namespace detail
{
/**
 * reflection type metafunction that computes the type of a tuple
 * holding the types of the parameters to a function call
 */
template<std::meta::info FcnMeta>
  requires FcnMetaT<FcnMeta>
class FcnArgsTuple {
  static constexpr auto kParamsMeta =
    std::define_static_array(std::meta::parameters_of(FcnMeta)
                             | std::views::transform(std::meta::type_of));

  static constexpr auto kTpl = std::meta::substitute(^^std::tuple, kParamsMeta);

public:
  using type = typename[:kTpl:];
};
} // namespace detail

template<auto FcnPtr>
using FcnParamsTupleForFcnPtrT =
  typename detail::FcnArgsTuple<std::meta::reflect_function(*FcnPtr)>::type;

template<std::meta::info FcnMeta>
using FcnParamsTupleForFcnMetaT = typename detail::FcnArgsTuple<FcnMeta>::type;

/**
 * hacky mechanism for forcing the compiler to output a compile time
 * string during compilation
 */
template<const char* kMsg>
struct CompileTimeMsgHack {
  static_assert(kMsg == nullptr, "!!!!!!!!!! COMPILE TIME MESSAGE !!!!!!!!!!");
};

} // namespace jmg
