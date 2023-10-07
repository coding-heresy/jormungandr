#pragma once

#include <optional>

#include <boost/property_tree/ptree.hpp>

#include "jmg/array_proxy.h"
#include "jmg/object.h"
#include "jmg/util.h"
#include "jmg/union.h"

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
  if (!src) {
    return std::nullopt;
  }
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
struct ElementTag : FieldDef
{
  static constexpr char const * name = kPlaceholder;
  using type = std::string;
  using required = std::true_type;
};

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
class Object : public ObjectDef<ElementTag, Fields...>
{
public:
  using adapted_type = boost::property_tree::ptree::value_type;

  Object() = delete;
  explicit Object(const adapted_type& node)
    : node_(&node) {}

  /**
   * delegate for jmg::get()
   */
  template<RequiredField FldT>
  auto get() const {
    if constexpr (std::same_as<ElementTag, FldT>) {
      return key_of(*node_);
    }
    else if constexpr (std::derived_from<FldT, ElementAttr>) {
      return value_of(*node_).get<typename FldT::type>(getAttrXPath(FldT::name));
    }
    else if constexpr (IsUnionT<typename FldT::type>{}()
		       || IsArrayProxyT<typename FldT::type>{}()) {
      return typename FldT::type{value_of(*node_)};
    }
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField FldT>
  std::optional<typename FldT::type> try_get() const {
    if constexpr (std::derived_from<FldT, ElementAttr>) {
      auto rslt = value_of(*node_).get_optional<typename FldT::type>(getAttrXPath(FldT::name));
      return jmg::ptree::detail::convertOptional(std::move(rslt));
    }
    else if constexpr (IsArrayProxyT<typename FldT::type>{}()) {
      const auto& val = value_of(*node_);
      if (val.empty()) {
	return std::nullopt;
      }
      if ((1 == val.size()) && (val.count("<xmlattr>"))) {
	// <xmlattr> is not a 'normal' child of an element so the set
	// of children is empty if it is the only one
	return std::nullopt;
      }
      return typename FldT::type{val};
    }
    else if constexpr (IsUnionT<typename FldT::type>{}()) {
      return typename FldT::type{*node_};
    }
  }

private:
  // TODO find a more efficient solution for the xpath construction
  // (preferably compile-time)
  static std::string getAttrXPath(const char* attrName) {
    static const std::string kBase = "<xmlattr>.";
    return kBase + attrName;
  }

  const adapted_type* node_;
};

////////////////////////////////////////////////////////////////////////////////
// iterator proxy for array of elements
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class ElementsItrProxy
  : public AdaptingConstItrProxy<boost::property_tree::ptree::const_iterator, T>
{
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
    if ("<xmlattr>" == key_of(*Base::itr_)) {
      ++Base::itr_;
    }
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
struct XmlSizePolicy : SizePolicyTag
{
  static size_t size(const SrcContainerT* src) {
    return src->size() - src->count("<xmlattr>");
  }
};

template<ObjectDefT Obj>
struct ElementsArrayTypeFactory
{
  // TODO the result of calling size() and empty() on this proxy for
  // an array of XML elements will generally not agree with the
  // results of iteration due to the way in which ptree encoding of
  // XML data handles attributes, which should be fixed eventually
  using ItrProxy = ElementsItrProxy<Obj>;
  using ItrPolicy = ProxiedItrPolicy<boost::property_tree::ptree, ItrProxy>;
  using SzPolicy = XmlSizePolicy<boost::property_tree::ptree>;
  using type = ArrayProxy<boost::property_tree::ptree, ItrPolicy, SzPolicy>;
};
} // namespace detail
template<ObjectDefT Obj>
using ElementsArrayT = meta::_t<detail::ElementsArrayTypeFactory<Obj>>;

/**
 * Field definition used to return the children of an XML element.
 */
template<typename T, typename Required = std::false_type>
struct Elements : FieldDef
{
  static constexpr char const * name = kPlaceholder;
  using type = ElementsArrayT<T>;
  using required = Required;
};

/**
 * alias for std::true_type intended for use when specifying the
 * 'Required' type parameter of an 'Elements' field definition
 */
using ElementsRequired = std::true_type;

} // namespace xml
} // namespace ptree
} // namespace jmg

#define JMG_XML_FIELD_DEF(field_name, str_name, field_type, is_required) \
  struct field_name : jmg::FieldDef, ptree::xml::ElementAttr		\
  {									\
    static constexpr char name[] = str_name;				\
    using type = field_type;						\
    using required = is_required##_type;				\
  }

// TODO add a separate macro for defining a field that wraps an array?
