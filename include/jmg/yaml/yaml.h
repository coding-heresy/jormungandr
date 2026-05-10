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
#include "jmg/conversion.h"
#include "jmg/object.h"
#include "jmg/safe_types.h"
#include "jmg/union.h"
#include "jmg/util.h"

/**
 * The YAML object API is very messy due to the fact that array objects (AKA
 * YAML "sequences") require complex proxying to function as expected.
 * Hopefully, the mess will get cleaned up over time.
 *
 * TODO(bd) use the type interrogation member functions supported by YAML::Node
 * (`IsScalar`, `IsSequence`, `IsMap` and maybe `IsNull`) to add sanity checking
 * to the various branches of the get() implementation
 */

namespace jmg::yaml
{

namespace detail
{
JMG_TAG_TYPE(Field);
JMG_TAG_TYPE(Object);
} // namespace detail

JMG_FIELD_CONCEPT();
JMG_OBJECT_CONCEPT();

namespace detail
{

/**
 * class template that extracts the value of the required type from the object
 * that the yaml-cpp iterator returns when it is dereferenced, and holds the
 * value until it is needed
 *
 * NOTE: this class handles the case of YAML "sequences" of primitive objects.
 * Since yaml nodes can be interpreted as various types, the `as` member
 * function template of the object returned by dereferencing the yaml-cpp
 * iterator must be called with the required type, which is "backed into" this
 * class upon instantiation. This is not a problem for JMG objects, which are
 * constructed directly from YAML::node objects.
 *
 * TODO(bd) based on the description here, it seems like dereferencing the
 * iterator should produce a YAML::Node, but that wasn't evident upon looking at
 * the information spewed out by the compiler so maybe investigate this
 * possibility with an eye toward simplifying this
 */
template<typename T>
struct ItrValueProxy {
  // NOTE: the constructor is a function template because dereferencing
  // YAML::const_iterator produces an object of a complex type that I really
  // don't feel like figuring out at this point, so I'll let the compiler handle
  // the heavy lifting
  template<typename ItrDeref>
  explicit ItrValueProxy(const ItrDeref& itr) : val_(itr.template as<T>()) {}
  operator T() { return val_; }
  operator T() const { return val_; }

private:
  T val_;
};

////////////////////
// type metafunction that calculates the correct type of iterator proxy to use
// for an array field based on the type associated with it

template<typename T>
struct ItrProxyFactory {
  // primitive type iterator proxy uses the YAML::Node iterator's `as` member
  // function template to convert the node value into the required actual value
  using type = AdaptingConstItrProxy<YAML::const_iterator, ItrValueProxy<T>>;
};

template<yaml::ObjectT T>
struct ItrProxyFactory<T> {
  // object type iterator proxy constucts the required JMG object from the YAML::Node
  using type = AdaptingConstItrProxy<YAML::const_iterator, T>;
};

/**
 * type metafunction that generates the array proxy type for an array field
 */
template<typename T>
struct ArrayTypeFactory {
  using ItrProxy = jmg::_T<ItrProxyFactory<T>>;
  using ItrPolicy = ProxiedItrPolicy<YAML::Node, ItrProxy>;
  using type = OwningArrayProxy<YAML::Node, ItrPolicy>;
};

} // namespace detail

/**
 * public-facing version of the type metafunction that calculates the array
 * proxy type for an array field
 */
template<typename T>
using ArrayFldImplT = meta::_t<detail::ArrayTypeFactory<T>>;

/**
 * class template for field definitions that are specific to YAML
 * objects
 */
template<typename T, StrLiteral kName, TypeFlagT IsRequired>
struct Field : FieldDef<T, kName, IsRequired>, public detail::FieldTag {};

/**
 * class template for string field definitions that are specific to
 * YAML objects
 */
template<StrLiteral kName, TypeFlagT IsRequired>
struct StringField : public yaml::Field<std::string, kName, IsRequired>,
                     public jmg::detail::StringFieldTag {
  using view_type = std::string_view;
  using const_view_type = std::string_view;
};

/**
 * class template for array field definitions that are specific to YAML
 * objects
 *
 * TODO(bd) use safe type instead of uint32_t for field ID?
 */
template<typename T, StrLiteral kName, TypeFlagT IsRequired>
struct ArrayField : public yaml::Field<ArrayFldImplT<T>, kName, IsRequired>,
                    public jmg::detail::ArrayFieldTag {
  using view_type = ArrayFldImplT<T>;
  using const_view_type = ArrayFldImplT<const T>;
};

// TODO(bd) constrain the types of fields with the correct concept
template<typename... Fields>
class Object : public ObjectDef<Fields...>, public detail::ObjectTag {
public:
  using adapted_type = YAML::Node;

  Object() = default;
  explicit Object(const adapted_type& node) : node_(node) {}

  /**
   * delegate for jmg::get()
   *
   * NOTE: due to the way that yaml-cpp is implemented, returning
   * references is very troublesome
   */
  template<RequiredFieldT Fld>
  decltype(auto) get() const {
    using Rslt = typename Fld::type;
    const char* name = Fld::name;
    if constexpr (SafeT<typename Fld::type>) {
      using UnsafeType = UnsafeTypeFromT<Rslt>;
      return Rslt(node_[name].as<UnsafeType>());
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
        using UnsafeType = UnsafeTypeFromT<SafeT>;
        return Rslt(entry.as<UnsafeType>());
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
    if constexpr (SafeT<T>) { node_[Fld::name] = unsafe(val); }
    else if constexpr (StringFieldT<Fld>) {
      node_[Fld::name] = static_cast<std::string>(from(val));
    }
    else if constexpr (ArrayFieldT<Fld>) { node_[Fld::name] = val; }
    else { node_[Fld::name] = val; }
  }

  /**
   * delegate for jmg::clear()
   */
  template<OptionalFieldT Fld>
  void clear() {
    static_assert(false, "clear() is not yet supported for YAML");
  }

  template<UnionFieldT UnionFld, FieldDefT MemberFld>
  bool union_has() const
    requires(UnionMemberFieldT<UnionFld, MemberFld>)
  {
    JMG_ENFORCE_USING(std::logic_error, false, "unions are not yet supported");
  }

  template<UnionFieldT UnionFld, FieldDefT MemberFld>
  decltype(auto) union_get() const
    requires(UnionMemberFieldT<UnionFld, MemberFld>)
  {
    JMG_ENFORCE_USING(std::logic_error, false, "unions are not yet supported");
  }

  template<UnionFieldT UnionFld, typename Fcn>
  void union_visit(Fcn&& fcn) const {
    JMG_ENFORCE_USING(std::logic_error, false, "unions are not yet supported");
  }

  template<UnionFieldT UnionFld, FieldDefT MemberFld, typename Arg>
  decltype(auto) union_set(Arg&& arg)
    requires(UnionMemberFieldT<UnionFld, MemberFld>
             && DecayedSameAsT<ArgTypeForFieldT<MemberFld>, Arg>)
  {
    JMG_ENFORCE_USING(std::logic_error, false, "unions are not yet supported");
  }

private:
  adapted_type node_;
};

} // namespace jmg::yaml
};
} // namespace detail
template<yaml::ObjectT Obj>
using ArrayField = meta::_t<detail::ArrayTypeFactory<Obj>>;

} // namespace jmg::yaml
