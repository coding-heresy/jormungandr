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

#include <algorithm>
#include <cctype>
#include <ranges>

#include "jmg/conversion.h"
#include "jmg/util.h"

using namespace std;
namespace rng = std::ranges;
namespace vws = std::views;

namespace jmg
{

string snakeCaseToCamelCase(const string_view str, bool capitalize_leading) {
  return str | vws::enumerate | vws::transform([&](auto&& item) {
           auto [idx, chr] = item;
           const auto capitalize =
             ((idx < 1) && capitalize_leading) || ('_' == str[idx - 1]);
           return capitalize ? to_upper(chr) : to_lower(chr);
         })
         | vws::filter([](const char chr) { return chr != '_'; })
         | rng::to<string>();
}

string camelCaseToSnakeCase(std::string_view str) {
  string rslt;
  rslt.reserve(2 * str.size());
  bool is_first = true;
  rng::copy(str | vws::transform([&](const char chr) -> string {
              if (is_first) {
                is_first = false;
                string rslt = from(to_lower(chr));
                return rslt;
              }
              return isupper(chr) ? str_cat("_", string(1, to_lower(chr)))
                                  : from(chr);
            }) | vws::join,
            inserterator(rslt));
  return rslt;
}

} // namespace jmg
