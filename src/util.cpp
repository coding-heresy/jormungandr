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

#include "absl/strings/str_replace.h"

#include "jmg/conversion.h"
#include "jmg/util.h"

using namespace std;
using namespace std::chrono;
namespace rng = std::ranges;
namespace vws = std::views;

namespace
{

string snakeCaseConvertImpl(const string_view str,
                            const bool capitalize_leading) {
  return str | vws::enumerate
         | vws::transform([&](auto&& item) {
             auto [idx, chr] = item;
             return // maybe capitalize the leading character
               (capitalize_leading && !idx) ||
                   // capitalize the first character after an underscore
                   ((idx > 0) && ('_' == str[idx - 1]))
                 ? jmg::to_upper(chr)
                 : jmg::to_lower(chr);
           })
         // filter out all underscores
         | vws::filter([](const char chr) { return chr != '_'; })
         | rng::to<string>();
}

} // namespace

namespace jmg
{

string snakeCaseToCamelCase(const string_view str) {
  return snakeCaseConvertImpl(str, false /* capitalize_leading */);
}

string snakeCaseToPascalCase(const string_view str) {
  return snakeCaseConvertImpl(str, true /* capitalize_leading */);
}

string camelCaseToSnakeCase(const string_view str, const bool all_caps) {
  return str | vws::enumerate | vws::transform([&](auto&& item) -> string {
           auto [idx, chr] = item;
           return (!idx) ?
                         // first character converts to the appropriate version
                         // of itself
                    from(all_caps ? to_upper(chr) : to_lower(chr))
                         :
                         // characters after the first
                    (isupper(chr)
                       ?
                       // uppercase characters convert to underscore followed by
                       // the appropriate version of the character
                       str_cat("_", string(1, all_caps ? chr : to_lower(chr)))
                       :
                       // lowercase characters convert to the appropriate
                       // version of themselves
                       from(all_caps ? to_upper(chr) : chr));
         })
         | vws::join | rng::to<string>();
}

std::string translateTypeNames(std::string&& content) {
  using namespace std::string_view_literals;
  using TypeStrTranslation =
    std::vector<std::pair<std::string_view, std::string_view>>;
  static const auto kReplacements = TypeStrTranslation{
    {"std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >"sv,
     "std::string"sv},
    {"std::basic_string_view<char, std::char_traits<char> >"sv,
     "std::string_view"sv},
    {" >"sv, ">"sv}};

  return absl::StrReplaceAll(content, kReplacements);
}

Duration epoch_duration_from(const TimePoint tp) {
  return duration_cast<nanoseconds>(tp.time_since_epoch());
}

} // namespace jmg
