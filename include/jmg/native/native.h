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

#include <tuple>

#include "jmg/meta.h"
#include "jmg/object.h"
#include "jmg/safe_types.h"

namespace jmg::native
{

namespace detail
{
using namespace meta::placeholders;

// NOTE: a bunch of code to cleanly handle the case where std::nullopt
// is passed for the value of a std::optional field when constructing
// a native object

////////////////////
// the FieldTypesMatch metafunction will return true under the
// following conditions:
// 1. the types are the same
// 2. one is std::nullopt_t and the other is any specialization of
//    std::optional
// 3. one is type T and the other is std::optional<T>
template<typename Lhs, typename Rhs>
struct FieldTypesMatch : std::false_type {};
template<typename T>
struct FieldTypesMatch<T, T> : std::true_type {};
template<typename T>
struct FieldTypesMatch<std::nullopt_t, std::optional<T>> : std::true_type {};
template<typename T>
struct FieldTypesMatch<std::optional<T>, std::nullopt_t> : std::true_type {};
template<typename T>
struct FieldTypesMatch<T, std::optional<Decay<T>>> : std::true_type {};
template<typename T>
struct FieldTypesMatch<std::optional<Decay<T>>, T> : std::true_type {};

/**
 * the FieldTypeMatches metafunction takes a type list consisting of a
 * pair of types (that was previously constructed via meta::zip) and
 * applies the FieldTypesMatch metafunction to the separated types
 */
template<TypeListT Pair>
using FieldTypeMatches =
  meta::_t<FieldTypesMatch<Decay<meta::front<Pair>>, Decay<meta::back<Pair>>>>;

/**
 * the MatchesFromFieldTypes converts a list of type pairs (where a
 * type pair is encoded as a list of size 2) to a list of booleans in
 * type space
 */
template<TypeListT ZippedLst>
using MatchesFromFieldTypes =
  meta::transform<ZippedLst, meta::quote<FieldTypeMatches>>;

/**
 * the ReduceAllMatches metafunction takes a list of type pairs (where
 * a type pair is encoded as a list of size 2) to a single boolean
 * indicating whether or not all the types match
 */
template<typename ZippedLst>
using ReduceAllMatches =
  meta::fold<MatchesFromFieldTypes<ZippedLst>,
             std::true_type,
             meta::lambda<_a, _b, meta::lazy::and_<_a, _b>>>;

template<typename Adapted, typename Arg>
constexpr bool isAdaptedObject() {
  if constexpr (!isTuple<Arg>()) { return false; }
  else {
    using RawTypes = DeTuplize<Adapted>;
    using ArgTypes = DeTuplize<Arg>;
    using Zipable = meta::list<RawTypes, ArgTypes>;
    using AllFieldsMatch = detail::ReduceAllMatches<meta::zip<Zipable>>;
    return AllFieldsMatch{};
  }
}

} // namespace detail

/**
 * class template for a standard interface object associated with a
 * raw tuple
 */
template<FieldDefT... Flds>
class Object : public ObjectDef<Flds...> {
  using base = ObjectDef<Flds...>;

public:
  using Fields = typename base::Fields;
  using adapted_type = jmg::Tuplize<meta::transform<Fields, Optionalize>>;

  Object() = default;
  template<typename... Args>
  explicit Object(Args&&... args) {
    using PackList = meta::list<Args...>;
    using Front = meta::front<PackList>;
    if constexpr ((1 == PackList::size())
                  && detail::isAdaptedObject<adapted_type, Front>()) {
      // there is only 1 argument and it consists of the type of tuple
      // that this object holds
      obj_ = adapted_type(std::forward<Args>(args)...);
    }
    else {
      using RawTypes = DeTuplize<adapted_type>;
      // NOTE: the argument to the meta::zip metafunction must consist
      // of a list of lists
      using Zipable = meta::list<RawTypes, PackList>;
      using AllFieldsMatch = detail::ReduceAllMatches<meta::zip<Zipable>>;
      static_assert(AllFieldsMatch{}, "parameters to tuple object constructor "
                                      "do not match its declared type list");
      obj_ = adapted_type(std::forward<Args>(args)...);
    }
  }

  // TODO handle return types better across safe and non-safe types

  /**
   * delegate for jmg::get()
   */
  template<RequiredField Fld>
  decltype(auto) get() const {
    using FldType = typename Fld::type;
    using Rslt = ReturnTypeForAnyT<typename Fld::type>;
    if constexpr (std::is_reference_v<Rslt>) {
      // TODO(bd) figure out why the const-ness of obj_ doesn't seem
      // to apply to the result of std::get
      const Rslt& rslt =
        const_cast<const Rslt&>(std::get<typename Fld::type>(obj_));
      static_assert(std::is_reference_v<decltype(rslt)>,
                    "return type is not a reference as expected");
      return static_cast<const Decay<Rslt>&>(rslt);
    }
    else {
      const Rslt rslt = std::get<typename Fld::type>(obj_);
      static_assert(!std::is_reference_v<decltype(rslt)>,
                    "return type is not a value as expected");
      return rslt;
    }
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField Fld>
  decltype(auto) try_get() const {
    using FldType = typename Fld::type;
    // NOTE: actual type stored in the tuple is optional<Fld::type>
    using OptType = std::optional<FldType>;
    return std::get<OptType>(obj_);
  }

  /**
   * delegate for jmg::set()
   */
  template<FieldDefT Fld>
  void set(ArgTypeForT<Fld> arg) {
    constexpr auto idx = entryIdx<Fld, typename base::Fields>();
    auto& entry = std::get<idx>(obj_);
    entry = arg;
  }

  // TODO(bd) replace these with set() member functions
  adapted_type& getWrapped() { return obj_; }
  const adapted_type& getWrapped() const { return obj_; }

private:
  adapted_type obj_;
};

} // namespace jmg::native
