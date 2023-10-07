#pragma once

#include <concepts>

namespace jmg
{

/**
 * tag type used to indicate that a type is a field definition
 *
 * Mostly obsolete due to c++20 concepts, but still useful for
 * self-documenting the code.
 */
struct FieldDef {};

namespace detail
{

/**
 * field definition must have a 'name' static data member that is a
 * string constant.
 */
template<typename T>
concept HasFieldName = requires {
  std::same_as<decltype(T::name), char const *>;
};

/**
 * field definition must have a 'type' member
 */
template<typename T>
concept HasFieldType = requires {
  typename T::type;
};

/**
 * field definition must have a 'required' member
 */
template<typename T>
concept HasRequiredSpec = requires {
  typename T::required;
};

} // namespace detail

/**
 * Concept for field definition
 */
template<typename T>
concept FieldDefT = std::derived_from<T, FieldDef>
  && detail::HasFieldName<T>
  && detail::HasFieldType<T>
  && detail::HasRequiredSpec<T>;

/**
 * concept for required field, used by jmg::get
 */
template<typename T>
concept RequiredField = FieldDefT<T>
  && std::same_as<typename T::required, std::true_type>;

/**
 * concept for optional field, used by jmg::try_get
 */
template<typename T>
concept OptionalField = FieldDefT<T>
  && std::same_as<typename T::required, std::false_type>;

/**
 * common field name constant for fields that have no string name
 */
inline constexpr char kPlaceholder[] = "";

} // namespace jmg

/**
 * helper macro to simplify declaration of fields
 */
#define JMG_FIELD_DEF(field_name, str_name, field_type, is_required)	\
  struct field_name : jmg::FieldDef					\
  {									\
    static constexpr char name[] = str_name;				\
    using type = field_type;						\
    using required = std::is_required##_type;				\
  }
