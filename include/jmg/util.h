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
#include <tuple>

#include "meta.h"
#include "preprocessor.h"
#include "types.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// stream tuple values to output
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T, typename... Ts>
void streamOutItems(std::ostream& strm, const T& first, const Ts&... rest) {
  strm << first;
  ((strm << ", " << rest), ...);
}
} // namespace detail

template<typename... Ts>
std::ostream& operator<<(std::ostream& strm, const std::tuple<Ts...>& arg) {
  apply([&](auto&&... args) { detail::streamOutItems(strm, args...); }, arg);
  return strm;
}

////////////////////////////////////////////////////////////////////////////////
// stream octet to output
////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& strm, Octet arg) {
  const auto old_fill = strm.fill('0');
  const auto old_width = strm.width(8);
  strm << std::bitset<8>(unsafe(arg));
  strm.fill(old_fill);
  strm.width(old_width);
  return strm;
}

////////////////////////////////////////////////////////////////////////////////
// helper functions for dictionaries
////////////////////////////////////////////////////////////////////////////////

const auto& key_of(const auto& rec) { return std::get<0>(rec); }

const auto& value_of(const auto& rec) { return std::get<1>(rec); }

auto& value_of(auto& rec) { return std::get<1>(rec); }

template<typename DictContainer, typename Key, typename... Vals>
void always_emplace(std::string_view description,
                    DictContainer& dict,
                    Key key,
                    Vals... vals) {
  const auto [_, inserted] = dict.try_emplace(key, std::forward<Vals>(vals)...);
  JMG_ENFORCE(inserted,
              "unsupported duplicate key [" << key << "] for " << description);
}

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
