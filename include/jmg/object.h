#pragma once

#include <concepts>

#include <meta/meta.hpp>

#include "jmg/field.h"
#include "jmg/meta.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// declaration of a field group
////////////////////////////////////////////////////////////////////////////////

template<FieldDefT...>
struct FieldGroupDef {};

namespace detail
{
template<typename T>
struct IsFieldGroupDefImpl { using type = std::false_type; };

template<typename... Ts>
struct IsFieldGroupDefImpl<FieldGroupDef<Ts...>>
{
  using type = std::true_type;
};
} // namespace detail

template<typename T>
using IsFieldGroupDefT = meta::_t<detail::IsFieldGroupDefImpl<T>>;

template<typename T>
concept FieldGroupDefT = IsFieldGroupDefT<T>{}();

////////////////////////////////////////////////////////////////////////////////
// concept that constrains a type to being a field or a field group
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct IsFieldOrGroupDefImpl { using type = std::false_type; };
template<FieldDefT T>
struct IsFieldOrGroupDefImpl<T> { using type = std::true_type; };
template<FieldGroupDefT T>
struct IsFieldOrGroupDefImpl<T> { using type = std::true_type; };
};
template<typename T>
using IsFieldOrGroupDefT = meta::_t<detail::IsFieldOrGroupDefImpl<T>>;

template<typename T>
concept FieldOrGroupT = FieldDefT<T> || FieldGroupDefT<T>;

template<typename T>
inline constexpr bool isFieldOrGroup() {
  return std::same_as<IsFieldOrGroupDefT<T>, std::true_type>;
}

namespace detail
{
template<typename T>
struct ValidObjectContentCheckUnwrap {
  static inline constexpr bool check() {
    return false;
  }
};
template<typename... Ts>
struct ValidObjectContentCheckUnwrap<meta::list<Ts...>> {
  static inline constexpr bool check() {
    return (isFieldOrGroup<Ts>() || ...);
  }
};
} // namespace detail

template<typename T>
inline constexpr bool isValidObjectContent() {
  return detail::ValidObjectContentCheckUnwrap<T>::check();
}
template<typename T>
concept ValidObjectContent = isValidObjectContent<T>();

namespace detail
{
template<typename T>
struct FieldExpanderImpl { using type = meta::list<T>; };
// TODO call recursively in case a field group contains another field group
template<typename... Ts>
struct FieldExpanderImpl<FieldGroupDef<Ts...>> { using type = meta::list<Ts...>; };
} // namespace detail
template<typename T>
using FieldExpanderT = meta::_t<detail::FieldExpanderImpl<T>>;

template<TypeList T>
using ExpandedFields = meta::join<meta::transform<T, meta::quote<FieldExpanderT>>>;

////////////////////////////////////////////////////////////////////////////////
// declaration of an object
////////////////////////////////////////////////////////////////////////////////

template<FieldOrGroupT... FieldsT>
struct ObjectDef
{
  using Fields = ExpandedFields<meta::list<FieldsT...>>;
};

namespace detail
{
template<typename T>
concept HasFields = requires {
  typename T::Fields;
};
template<typename T>
concept HasValidContent = ValidObjectContent<typename T::Fields>;
} // namespace detail

template<typename T>
concept ObjectDefT = detail::HasFields<T> && detail::HasValidContent<T>;

namespace detail
{
template<typename T, typename Lst>
struct IsMemberOfUnwrap {
  static inline constexpr bool isMemberOf() {
    return false;
  }
};

template<typename T, typename... Ts>
struct IsMemberOfUnwrap<T, meta::list<Ts...>> {
  static inline constexpr bool isMemberOf() {
    return (std::same_as<T, Ts> || ...);
  }
};
} // namespace detail
template<FieldDefT T, ObjectDefT ObjT>
inline constexpr bool isMemberOfObject() {
  return isMemberOfList<T, typename ObjT::Fields>();
}

////////////////////////////////////////////////////////////////////////////////
// definitions of get() and try_get()
////////////////////////////////////////////////////////////////////////////////

template<FieldDefT FieldT, ObjectDefT ObjectT>
auto get(const ObjectT& obj) requires (isMemberOfObject<FieldT, ObjectT>()) {
  return obj.template get<FieldT>();
}

template<FieldDefT FieldT, ObjectDefT ObjectT>
auto try_get(const ObjectT& obj) requires (isMemberOfObject<FieldT, ObjectT>()) {
  return obj.template try_get<FieldT>();
}

} // namespace jmg
