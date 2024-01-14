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

#include <quickfix/Message.h>

#include "jmg/conversion.h"
#include "jmg/field.h"
#include "jmg/object.h"
#include "jmg/preprocessor.h"

namespace jmg
{

namespace quickfix
{

template<typename... Fields>
class Object : public ObjectDef<Fields...>
{
public:
  using adapted_type = FIX::Message;

  Object() = delete;
  explicit Object(const adapted_type& adapted) : adapted_(&adapted) {}


  /**
   * delegate for jmg::get()
   */
  template<RequiredField FldT>
  typename FldT::type get() const {
    return getImpl<FldT>();
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField FldT>
  std::optional<typename FldT::type> try_get() const {
    if (!adapted_->isSetField(FldT::kFixTag)) {
      return std::nullopt;
    }
    return getImpl<FldT>();
  }

private:
  /**
   * implementation for get/try_get
   */
  template<typename FldT>
  typename FldT::type getImpl() const
    requires ConversionTgtT<typename FldT::type>
  {
    const std::string& fieldRef = adapted_->getField(FldT::kFixTag);
    return static_cast<typename FldT::type>(from_string(fieldRef));
  }
  const adapted_type* adapted_;
};

} // namespace quickfix
} // namespace jmg
