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

namespace jmg::yaml
{

namespace detail
{
JMG_TAG_TYPE(Field);
JMG_TAG_TYPE(Object);
} // namespace detail

JMG_FIELD_CONCEPT();
JMG_OBJECT_CONCEPT();

// TODO(bd) constrain the types of fields with the correct concept
template<typename... Fields>
class Object : public ObjectDef<Fields...>, public detail::ObjectTag {
public:
  using adapted_type = YAML::Node;

  Object() = delete;
  explicit Object(const adapted_type& node) : node_(node) {}

  /**
   * delegate for jmg::get()
   *
   * NOTE: due to the way that yaml-cpp is implemented, returning
   * references is very troublesome
   */
  template<RequiredFieldT Fld>
  typename Fld::type get() const {
    using Rslt = typename Fld::type;
    const char* name = Fld::name;
    if constexpr (SafeT<typename Fld::type>) {
      using UnsafeT = UnsafeTypeFromT<Rslt>;
      return Rslt(node_[name].as<UnsafeT>());
    }
    else if constexpr (OwningArrayProxyT<Rslt>) {
      return Rslt(YAML::Node(node_[name]));
    }
    else if constexpr (AnyEnumT<Rslt>) {
      return Rslt(node_[name].as<std::underlying_type_t<DecayT<Rslt>>>());
    }
    else if constexpr (yaml::ObjectT<Rslt>) { return Rslt(node_[name]); }
    else { return node_[name].as<Rslt>(); }
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalFieldT Fld>
  std::optional<typename Fld::type> try_get() const {
    const char* name = Fld::name;
    using Type = typename Fld::type;
    if (const auto entry = node_[name]; entry) {
      if constexpr (SafeT<Type>) {
        using SafeT = Type;
        using Rslt = std::optional<SafeT>;
        using UnsafeT = UnsafeTypeFromT<SafeT>;
        return Rslt(entry.as<UnsafeT>());
      }
      else if constexpr (AnyEnumT<Type>) {
        using Rslt = std::optional<Type>;
        return Rslt(Type(node_[name].as<std::underlying_type_t<DecayT<Type>>>()));
      }
      else if constexpr (OwningArrayProxyT<Type>) {
        using Rslt = std::optional<Type>;
        return Rslt(YAML::Node(entry));
      }
      else if constexpr (yaml::ObjectT<Type>) {
        using Rslt = std::optional<Type>;
        return Rslt(node_[name]);
      }
      else {
        using EffT = typename Fld::type;
        using Rslt = std::optional<EffT>;
        return Rslt(entry.as<EffT>());
      }
    }
    else { return std::nullopt; }
  }

  /**
   * delegate for jmg::set()
   */
  template<yaml::FieldT Fld, typename T>
  void set(T val) {
    static_assert(false, "set() is not yet supported for YAML");
  }

  /**
   * delegate for jmg::clear()
   */
  template<OptionalFieldT Fld>
  void clear() {
    static_assert(false, "clear() is not yet supported for YAML");
  }

private:
  adapted_type node_;
};

namespace detail
{
template<yaml::ObjectT Obj>
struct ArrayTypeFactory {
  using ItrProxy = AdaptingConstItrProxy<YAML::const_iterator, Obj>;
  using ItrPolicy = ProxiedItrPolicy<YAML::Node, ItrProxy>;
  using type = OwningArrayProxy<YAML::Node, ItrPolicy>;
};
} // namespace detail
template<yaml::ObjectT Obj>
using Array = meta::_t<detail::ArrayTypeFactory<Obj>>;

} // namespace jmg::yaml
