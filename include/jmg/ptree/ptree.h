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

#include <optional>

#include <boost/property_tree/ptree.hpp>

#include "jmg/array_proxy.h"
#include "jmg/object.h"
#include "jmg/union.h"
#include "jmg/util.h"

namespace jmg
{

namespace ptree
{

namespace detail
{
/**
 * Annoying conversion from boost::optional to std::optional
 *
 * TODO move to a separate utility code header?
 */
template<typename T>
inline std::optional<T> convertOptional(boost::optional<T>&& src) {
  if (!src) { return std::nullopt; }
  return std::optional(std::move(src.value()));
}
} // namespace detail

////////////////////////////////////////////////////////////////////////////////
// Code for interacting with ptree objects generated from XML data
////////////////////////////////////////////////////////////////////////////////
namespace xml
{

/**
 * field definition for the XML element 'tag', which is effectively
 * the name of the element
 */
struct ElementTag : jmg::FieldDef<std::string, kPlaceholder, std::true_type> {};

/**
 * Tag type used to define Jormungandr fields associated with
 * attributes of an element.
 */
struct ElementAttr {};

/**
 * class template for a Jormungandr object associated with ptree data
 * structured as XML
 */
template<typename... Fields>
class Object : public ObjectDef<ElementTag, Fields...> {
public:
  using adapted_type = boost::property_tree::ptree::value_type;

  Object() = delete;
  explicit Object(const adapted_type& elem) : elem_(&elem) {}

  /**
   * delegate for jmg::get()
   */
  template<RequiredField Fld>
  auto get() const {
    if constexpr (std::same_as<ElementTag, Fld>) { return key_of(*elem_); }
    else if constexpr (std::derived_from<Fld, ElementAttr>) {
      return value_of(*elem_).get<typename Fld::type>(getAttrXPath(Fld::name));
    }
    else if constexpr (UnionT<typename Fld::type>
                       || ViewingArrayProxyT<typename Fld::type>) {
      return typename Fld::type(value_of(*elem_));
    }
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField Fld>
  std::optional<typename Fld::type> try_get() const {
    if constexpr (std::derived_from<Fld, ElementAttr>) {
      auto rslt = value_of(*elem_).get_optional<typename Fld::type>(
        getAttrXPath(Fld::name));
      return jmg::ptree::detail::convertOptional(std::move(rslt));
    }
    else if constexpr (ViewingArrayProxyT<typename Fld::type>) {
      const auto& val = value_of(*elem_);
      if (val.empty()) { return std::nullopt; }
      if ((1 == val.size()) && (val.count("<xmlattr>"))) {
        // <xmlattr> is not a 'normal' child of an element so the set
        // of children is empty if it is the only one
        return std::nullopt;
      }
      return typename Fld::type{val};
    }
    else if constexpr (UnionT<typename Fld::type>) {
      return typename Fld::type(*elem_);
    }
  }

private:
  // TODO find a more efficient solution for the xpath construction
  // (preferably compile-time)
  static std::string getAttrXPath(const char* attrName) {
    static const std::string kBase = "<xmlattr>.";
    return kBase + attrName;
  }

  const adapted_type* elem_;
};

////////////////////////////////////////////////////////////////////////////////
// iterator proxy for array of elements
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class ElementsItrProxy
  : public AdaptingConstItrProxy<boost::property_tree::ptree::const_iterator, T> {
public:
  using ProxiedItr = boost::property_tree::ptree::const_iterator;
  using Base = AdaptingConstItrProxy<ProxiedItr, T>;
  using difference_type = Base::difference_type;
  using value_type = Base::value_type;
  using reference = Base::reference;
  using iterator_category = Base::iterator_category;

  ElementsItrProxy() = default;
  explicit ElementsItrProxy(ProxiedItr&& itr) : Base(std::move(itr)) {
    maybeSkipAttr();
  }

  ElementsItrProxy& operator++() {
    ++Base::itr_;
    maybeSkipAttr();
    return *this;
  }

  ElementsItrProxy operator++(int) {
    auto rslt = *this;
    ++Base::itr_;
    maybeSkipAttr();
    return rslt;
  }

private:
  /**
   * Skip any node named '<xmlattr>' since this is not a 'normal'
   * child of the current node and should not appear in any
   * iteration
   */
  void maybeSkipAttr() {
    if ("<xmlattr>" == key_of(*Base::itr_)) { ++Base::itr_; }
  }
};

////////////////////////////////////////////////////////////////////////////////
// proxy for arrays of elements
////////////////////////////////////////////////////////////////////////////////
namespace detail
{

/**
 * policy that accounts for '<xmlattr>' elements when retrieving the
 * size of the array
 */
template<typename SrcContainerT>
struct XmlSizePolicy : SizePolicyTag {
  static size_t size(const SrcContainerT* src) {
    return src->size() - src->count("<xmlattr>");
  }
};

template<ObjectDefT Obj>
struct ElementsArrayTypeFactory {
  using ItrProxy = ElementsItrProxy<Obj>;
  using ItrPolicy = ProxiedItrPolicy<boost::property_tree::ptree, ItrProxy>;
  using SzPolicy = XmlSizePolicy<boost::property_tree::ptree>;
  using type =
    ViewingArrayProxy<boost::property_tree::ptree, ItrPolicy, SzPolicy>;
};
} // namespace detail
template<ObjectDefT Obj>
using ElementsArrayT = meta::_t<detail::ElementsArrayTypeFactory<Obj>>;

/**
 * Field definition used to return the children of an XML element.
 */
template<typename T, typename Required = std::false_type>
struct Elements : FieldDef<ElementsArrayT<T>, kPlaceholder, Required> {};

/**
 * alias for std::true_type intended for use when specifying the
 * 'Required' type parameter of an 'Elements' field definition
 */
using ElementsRequired = std::true_type;

} // namespace xml
} // namespace ptree
} // namespace jmg

#define JMG_XML_FIELD_DEF(field_name, str_name, field_type, is_required)       \
  struct field_name : jmg::FieldDef<field_type, str_name, is_required##_type>, \
                      ptree::xml::ElementAttr {}

// TODO add a separate macro for defining a field that wraps an array?
