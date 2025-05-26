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

namespace jmg::tuple_object
{

namespace detail
{
using namespace meta::placeholders;

// NOTE: a bunch of code to cleanly handle the case where std::nullopt
// is passed for the value of a std::optional field when constructing
// a tuple object

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
template<FieldDefT... Fields>
class Object : public ObjectDef<Fields...> {
public:
  using FieldList = meta::list<Fields...>;
  using adapted_type = jmg::Tuplize<meta::transform<FieldList, Optionalize>>;

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
  template<RequiredField FldT>
  decltype(auto) get() const {
    using FldType = typename FldT::type;
    using Rslt = ReturnTypeForAnyT<typename FldT::type>;
    const Rslt rslt = std::get<typename FldT::type>(obj_);
    return rslt;
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField FldT>
  decltype(auto) try_get() const {
    using FldType = typename FldT::type;
    // NOTE: actual type stored in the tuple is optional<FldT::type>
    using OptType = std::optional<FldType>;
    using Intermediate = ReturnTypeForAnyT<FldType>;
    if constexpr (std::is_reference_v<Intermediate>) {
      // std::optional is not allowed for reference types, return a
      // pointer instead
      FldType* ptr = nullptr;
      auto& ref = std::get<OptType>(obj_);
      if (ref.has_value()) { ptr = &(*ref); }
      return ptr;
    }
    return std::get<OptType>(obj_);
  }

  adapted_type& getWrapped() { return obj_; }
  const adapted_type& getWrapped() const { return obj_; }

private:
  adapted_type obj_;
};

} // namespace jmg::tuple_object
