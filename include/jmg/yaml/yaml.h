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

#include <yaml-cpp/yaml.h>

#include "jmg/array_proxy.h"
#include "jmg/object.h"
#include "jmg/safe_types.h"
#include "jmg/union.h"
#include "jmg/util.h"

namespace jmg
{
namespace yaml
{

template<typename... Fields>
class Object : public ObjectDef<Fields...> {
public:
  using adapted_type = YAML::Node;

  Object() = delete;
  explicit Object(const adapted_type& node) : node_(node) {}

  /**
   * delegate for jmg::get()
   */
  template<RequiredField FldT>
  typename FldT::type get() const {
    using RsltT = typename FldT::type;
    const char* name = FldT::name;
    if constexpr (SafeT<typename FldT::type>) {
      using UnsafeT = UnsafeTypeFromT<RsltT>;
      return RsltT{node_[name].as<UnsafeT>()};
    }
    else if constexpr (OwningArrayProxyT<typename FldT::type>) {
      return RsltT{YAML::Node{node_[name]}};
    }
    else { return node_[name].as<RsltT>(); }
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField FldT>
  std::optional<typename FldT::type> try_get() const {
    const char* name = FldT::name;
    if (const auto entry = node_[name]; entry) {
      if constexpr (SafeT<typename FldT::type>) {
        using SafeT = typename FldT::type;
        using RsltT = std::optional<SafeT>;
        using UnsafeT = UnsafeTypeFromT<SafeT>;
        return RsltT{entry.as<UnsafeT>()};
      }
      else if constexpr (OwningArrayProxyT<typename FldT::type>) {
        using RsltT = std::optional<typename FldT::type>;
        return RsltT{YAML::Node{entry}};
      }
      else {
        using EffT = typename FldT::type;
        using RsltT = std::optional<EffT>;
        return RsltT{entry.as<EffT>()};
      }
    }
    else { return std::nullopt; }
  }

private:
  adapted_type node_;
};

namespace detail
{
template<ObjectDefT Obj>
struct ArrayTypeFactory {
  using ItrProxy = AdaptingConstItrProxy<YAML::const_iterator, Obj>;
  using ItrPolicy = ProxiedItrPolicy<YAML::Node, ItrProxy>;
  using type = OwningArrayProxy<YAML::Node, ItrPolicy>;
};
} // namespace detail
template<ObjectDefT Obj>
using ArrayT = meta::_t<detail::ArrayTypeFactory<Obj>>;

} // namespace yaml
} // namespace jmg
