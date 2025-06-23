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

/**
 * general purpose utilities
 */

#include <optional>
#include <ranges>
#include <tuple>

#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>

#include "meta.h"
#include "preprocessor.h"
#include "types.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// stream tuple values to output
////////////////////////////////////////////////////////////////////////////////

template<typename... Ts>
std::ostream& operator<<(std::ostream& strm, const std::tuple<Ts...>& tpl) {
  apply(
    [&](const auto& arg, const auto&... args) {
      strm << arg;
      ((strm << "," << args), ...);
    },
    tpl);
  return strm;
}

////////////////////////////////////////////////////////////////////////////////
// steam optionals to output
////////////////////////////////////////////////////////////////////////////////

template<typename T>
std::ostream& operator<<(std::ostream& strm, const std::optional<T> val) {
  if (!val.has_value()) { strm << "<empty>"; }
  else { strm << *val; }
  return strm;
}

////////////////////////////////////////////////////////////////////////////////
// stream octet to output
////////////////////////////////////////////////////////////////////////////////

inline std::ostream& operator<<(std::ostream& strm, Octet arg) {
  const auto old_fill = strm.fill('0');
  const auto old_width = strm.width(8);
  strm << std::bitset<8>(unsafe(arg));
  strm.fill(old_fill);
  strm.width(old_width);
  return strm;
}

/**
 * formatter to use with str_join
 */
struct OctetFmt {
  void operator()(std::string* out, Octet arg) const {
    // TODO(bd) probably could be much better than this but good enough for now...
    std::ostringstream strm;
    strm << arg;
    out->append(strm.str());
  }
};

static constexpr auto kOctetFmt = OctetFmt();

// TODO(bd) create a utility to print buffer contents as bitwise octets

////////////////////////////////////////////////////////////////////////////////
// rework annoying Abseil function names
//
// function names SHOULD NOT begin with a capital letter!
////////////////////////////////////////////////////////////////////////////////

template<typename Itr>
std::string str_join(Itr start, Itr finish, const std::string_view separator) {
  return absl::StrJoin(start, finish, separator);
}

template<std::ranges::range Rng>
std::string str_join(const Rng& range, const std::string_view separator) {
  return absl::StrJoin(range, separator);
}

template<std::ranges::range Rng, typename Fmt>
std::string str_join(const Rng& range,
                     const std::string_view separator,
                     Fmt&& fmt) {
  return absl::StrJoin(range, separator, std::forward<Fmt>(fmt));
}

// TODO(bd) add more absl::StrJoin overloads?

template<typename... Args>
std::string str_cat(Args&&... args) {
  return absl::StrCat(std::forward<Args>(args)...);
}

template<typename... Args>
void str_append(std::string& tgt, Args&&... args) {
  return absl::StrAppend(&tgt, std::forward<Args>(args)...);
}

// TODO(bd) add more?

////////////////////////////////////////////////////////////////////////////////
// helper functions for dictionaries
////////////////////////////////////////////////////////////////////////////////

/**
 * retrieve the key from a dictionary value_type
 */
const auto& key_of(const auto& rec) { return std::get<0>(rec); }

/**
 * retrieve the value (AKA mapped_type) from a dictionary value_type
 */
const auto& value_of(const auto& rec) { return std::get<1>(rec); }

/**
 * retrieve the value (AKA mapped_type) from a dictionary value_type
 */
auto& value_of(auto& rec) { return std::get<1>(rec); }

/**
 * function template that emplaces a new item in a dictionary or
 * throws an exception if the key already exists
 */
template<typename DictContainer, typename Key, typename... Vals>
void always_emplace(std::string_view description,
                    DictContainer& dict,
                    Key key,
                    Vals... vals) {
  const auto [_, inserted] = dict.try_emplace(key, std::forward<Vals>(vals)...);
  JMG_ENFORCE(inserted,
              "unsupported duplicate key [" << key << "] for " << description);
}

////////////////////////////////////////////////////////////////////////////////
// misc helper functions
////////////////////////////////////////////////////////////////////////////////

/**
 * function template that is a simple wrapper around static_cast<bool>
 * because I'm too lazy to type that much and I find the longer text
 * too cluttered for easy reading
 */
template<typename T>
bool pred(const T& val) {
  return static_cast<bool>(val);
}

////////////////////////////////////////////////////////////////////////////////
// retrieve a specific type from a parameter pack
////////////////////////////////////////////////////////////////////////////////

/**
 * function template that retrieves a value of a specific type from a
 * parameter pack
 */
template<typename Tgt, typename... Args>
Tgt getFromArgs(Args&&... args) {
  Tgt rslt;
  auto processArg = [&]<typename T>(T&& arg) {
    if constexpr (decayedSameAs<T, Tgt>()) { rslt = std::forward<T>(arg); }
  };
  (processArg(args), ...);
  return rslt;
}

template<typename Tgt, typename... Args>
std::optional<Tgt> tryGetFromArgs(Args&&... args) {
  if constexpr (!isMemberOfList<Tgt, meta::list<Args...>>()) {
    return std::nullopt;
  }

  Tgt rslt;
  auto processArg = [&]<typename T>(T&& arg) {
    if constexpr (decayedSameAs<T, Tgt>()) { rslt = std::forward<T>(arg); }
  };
  (processArg(args), ...);
  return rslt;
}

////////////////////////////////////////////////////////////////////////////////
// cleanup class using RAII idiom
////////////////////////////////////////////////////////////////////////////////

/**
 * class template that automatically executes a cleanup action on
 * scope exit unless it is canceled
 *
 * shamelessly stolen from Google Abseil
 */
template<typename Fcn>
struct Cleanup {
  Cleanup(Fcn&& fcn) : fcn_(std::move(fcn)) {}
  ~Cleanup() {
    if (!isCxl_) { fcn_(); }
  }
  void cancel() { isCxl_ = true; }

private:
  bool isCxl_ = false;
  Fcn fcn_;
};

} // namespace jmg
