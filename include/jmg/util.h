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
#include <absl/strings/str_format.h>
#include <absl/strings/str_join.h>
#include <more_concepts/associative_containers.hpp>

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
// stream optionals to output
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

inline Octet octetify(const uint8_t arg) { return Octet(arg); }

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
 * emplace a new item in a dictionary or throw an exception if the key already
 * exists
 *
 * NOTE: Key is left as an explicit type parameter in order to
 * correctly support transparent hashing of e.g. std::string_view
 */
template<typename DictContainer, typename Key, typename... Vals>
void emplace_uniq(std::string_view description,
                  DictContainer& dict,
                  const Key& key,
                  Vals&&... vals) {
  const auto [_, inserted] = dict.try_emplace(key, std::forward<Vals>(vals)...);
  JMG_ENFORCE(inserted, "unsupported duplicate key [", key, "] for ",
              description);
}

/**
 * emplace a new item in a set or throw an exception if the item already exists
 *
 * NOTE: Key is left as an explicit type parameter in order to
 * correctly support transparent hashing of e.g. std::string_view
 */
template<typename SetContainer, typename Value>
void insert_uniq(std::string_view description,
                 SetContainer& set_container,
                 Value&& value) {
  const auto [_, inserted] = set_container.insert(std::forward<Value>(value));
  JMG_ENFORCE(inserted, "unsupported duplicate value [", value, "] for ",
              description);
}

/**
 * return a reference to the dictionary item referenced by the
 * argument key or throw an exception if no such item is present
 */
template<typename DictContainer>
decltype(auto) find_required(DictContainer& dict,
                             const typename DictContainer::key_type& key,
                             std::string_view container_type,
                             std::string_view key_type) {
  const auto entry = dict.find(key);
  JMG_ENFORCE(dict.end() != entry, container_type,
              " had no value for required ", key_type, "[", key, "]");
  return value_of(*entry);
}

template<typename T>
void* as_void_ptr(T* ptr) {
  if constexpr (SameAsDecayedT<void, T>) {
    if constexpr (std::is_const_v<T>) { return const_cast<void*>(ptr); }
    else { return ptr; }
  }
  else {
    if constexpr (std::is_const_v<T>) {
      return const_cast<void*>(reinterpret_cast<const void*>(ptr));
    }
    else { return reinterpret_cast<void*>(ptr); }
  }
}

////////////////////////////////////////////////////////////////////////////////
// helper functions for associative containers
////////////////////////////////////////////////////////////////////////////////

/**
 * concept for a container type that supports push_back()
 */
template<typename Container>
concept BackPushableT = requires(Container& container) {
  typename Container::value_type;
  container.push_back(std::declval<typename Container::value_type>());
};

/**
 * generate an insertion iterator suitable for the usual case of
 * inserting new items into a container
 */
template<typename Container>
auto inserterator(Container& container) {
  if constexpr (BackPushableT<Container>) {
    return std::back_inserter(container);
  }
  else {
    // standard way to use an insert iterator with an associative
    // container is to call std::inserter on the container,
    // additionally providing its 'end' iterator as the insertion hint
    return std::inserter(container, container.end());
  }
}

////////////////////////////////////////////////////////////////////////////////
// misc helper functions
////////////////////////////////////////////////////////////////////////////////

/**
 * make any type unsafe, including types that were already unsafe
 *
 * NOTE: should generally be used to simplify generic programming
 * cases such as foreign object/message encodings where safe types
 * must be stored in their unsafe forms
 */
template<typename T>
UnwrapT<T> unsafe_ify(T maybe_safe) {
  if constexpr (SafeT<T>) { return unsafe(maybe_safe); }
  else { return maybe_safe; }
}

/**
 * predicate
 *
 * wrapper around static_cast<bool> because I'm too lazy to type that much and I
 * find the longer text too cluttered for easy reading
 */
template<typename T>
bool pred(const T& val) {
  return static_cast<bool>(val);
}

/**
 * lowercase conversion that uses char instead of int
 */
inline char to_lower(char chr) { return static_cast<char>(std::tolower(chr)); }

/**
 * uppercase conversion that uses char instead of int
 */
inline char to_upper(char chr) { return static_cast<char>(std::toupper(chr)); }

/**
 * convert a string from snake_case to CamelCase or camelCase
 */
std::string snakeCaseToCamelCase(std::string_view str,
                                 bool capitalize_leading = true);

/**
 * convert a string from CamelCase or camelCase to snake_case
 */
std::string camelCaseToSnakeCase(std::string_view str);

/**
 * compute the string representation of the address associated with a pointer
 */
template<typename T>
std::string strAddrOf(const T* ptr) {
  return str_cat(static_cast<uintptr_t>(ptr));
}

////////////////////////////////////////////////////////////////////////////////
// retrieve a specific type from a parameter pack
//
// TODO(bd) move to meta.h?
////////////////////////////////////////////////////////////////////////////////

/**
 * retrieve a value of a specific type from a parameter pack
 */
template<typename Tgt, typename... Args>
Tgt getFromArgs(Args&&... args) {
  Tgt rslt;
  auto processArg = [&]<typename T>(T&& arg) {
    if constexpr (DecayedSameAsT<T, Tgt>) { rslt = std::forward<T>(arg); }
  };
  (processArg(args), ...);
  return rslt;
}

/**
 * try to retrieve a value of a specific type from a parameter pack, return
 * nullopt if the value is not present
 */
template<typename Tgt, typename... Args>
std::optional<Tgt> tryGetFromArgs(Args&&... args) {
  if constexpr (!isMemberOfList<Tgt, meta::list<Args...>>()) {
    return std::nullopt;
  }

  Tgt rslt;
  auto processArg = [&]<typename T>(T&& arg) {
    if constexpr (DecayedSameAsT<T, Tgt>) { rslt = std::forward<T>(arg); }
  };
  (processArg(args), ...);
  return rslt;
}

////////////////////////////////////////////////////////////////////////////////
// cleanup class using RAII idiom
////////////////////////////////////////////////////////////////////////////////

/**
 * automatically execute a cleanup action on scope exit unless the action is
 * canceled
 *
 * shamelessly stolen from Google Abseil
 *
 * TODO(bd) using the 'invokable' concept to constrain the type
 * parameter?
 */
template<typename Fcn>
struct Cleanup {
  explicit Cleanup(Fcn&& fcn) : fcn_(std::move(fcn)) {}
  ~Cleanup() {
    if (!isCxl_) { fcn_(); }
  }
  void cancel() && { isCxl_ = true; }

private:
  bool isCxl_ = false;
  Fcn fcn_;
};

} // namespace jmg
