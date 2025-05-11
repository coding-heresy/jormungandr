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
  explicit Object(const adapted_type& obj) : obj_(obj) {}
  explicit Object(adapted_type&& obj) : obj_(std::move(obj)) {}

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
