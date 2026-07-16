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

#include <meta>

#include "compatibility.h"

namespace jmg
{

////////////////////
// owners for identifiers generated at compile time

/**
 * compile-time owner of a snake_case version of an identifier
 */
template<std::meta::info Identifiable>
class SnakeCaseIdOwner {
  static consteval auto make_snake_case() {
    constexpr auto id = std::meta::identifier_of(Identifiable);
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

/**
 * compile-time owner of a PascalCase version of an identifier
 */
template<std::meta::info Identifiable>
class PascalCaseIdOwner {
protected:
  static consteval auto make_pascal_case() {
    constexpr auto id = std::meta::identifier_of(Identifiable);
    static_assert(id.size() > 1UZ, "PascalCase conversion of empty or single "
                                   "letter strings is not supported");
    // TODO(bd) impose more restrictions on what constitutes a valid
    // identifier to convert to PascalCase?
    static_assert(
      ('_' != id[id.size() - 1] && '_' != id[0]),
      "PascalCase conversion of a string beginning or ending with an "
      "underscore character '_' is not supported");
    constexpr auto rsltSz = [&]() {
      size_t rslt = 1; // always needs null terminator
      for (size_t idx = 0; idx < id.size(); ++idx) {
        // input underscore characters do not appear in the output
        rslt += ('_' == id[idx]) ? 0UZ : 1UZ;
      }
      return rslt;
    }();
    auto rslt = std::array<char, rsltSz>{};
    // first character is always uppercase
    rslt[0] = jmg_std::to_upper(id[0]);
    size_t offset = 1;
    for (size_t idx = 1; idx < id.size(); ++idx) {
      if ('_' == id[idx]) { continue; }
      if ('_' == id[idx - 1]) { rslt[offset++] = jmg_std::to_upper(id[idx]); }
      else { rslt[offset++] = id[idx]; }
    }
    rslt[rslt.size() - 1] = '\0';
    return rslt;
  }

public:
  /**
   * return a C-style string pointer to the identifier
   */
  static constexpr const char* c_str() {
    static constexpr auto owner = make_pascal_case();
    return owner.data();
  }
};

/**
 * compile-time owner of a camelCase version of an identifier
 */
template<std::meta::info Identifiable>
class CamelCaseOwner : PascalCaseIdOwner<Identifiable> {
private:
  static consteval auto make_camel_case() {
    {
      constexpr auto id = std::meta::identifier_of(Identifiable);
      static_assert(id.size() > 1UZ, "camelCase conversion of empty or single "
                                     "letter strings is not supported");
      // TODO(bd) impose more restrictions on what constitutes a valid
      // identifier to convert to camelCase?
      static_assert(
        ('_' != id[id.size() - 1] && '_' != id[0]),
        "camelCase conversion of a string beginning or ending with an "
        "underscore character is not supported");
    }
    auto rslt = PascalCaseIdOwner<Identifiable>::pascal_case();
    rslt[0] = jmg_std::to_lower(rslt[0]);
  }

public:
  /**
   * return a C-style string pointer to the identifier
   */
  static constexpr const char* c_str() {
    static constexpr auto owner = make_camel_case();
    return owner.data();
  }
};

namespace detail
{
/**
 * reflection type metafunction that computes the type of a tuple
 * holding the types of the parameters to a function call
 */
template<std::meta::info FcnInfo>
class FcnArgsTuple {
  static constexpr auto params =
    std::define_static_array(std::meta::parameters_of(FcnInfo));

  /**
   * intermediate function which is never called but returns the
   * tuple-ized version of the static params array and is subsequently
   * used with decltype to compute the type of that tuple at compile
   * time
   */
  template<size_t... kIdxs>
  static auto derive_type(std::index_sequence<kIdxs...>) {
    return std::make_tuple<params[kIdxs]...>();
  }

public:
  using type = decltype(derive_type(std::make_index_sequence<params.size()>{}));
};
} // namespace detail

template<std::meta::info FcnInfo>
using FcnArgsTupleForT = detail::FcnArgsTuple<FcnInfo>::type;

} // namespace jmg
