#pragma once

#include <st/st.hpp>

#define JMG_SAFE_TYPE(strong_type, base_type, ...)	\
  using strong_type = st::type<base_type,		\
    struct strong_type ## SafeTag, __VA_ARGS__)

#define JMG_SAFE_ID_32(strong_type)		\
  using strong_type = st::type<uint32_t,	\
    struct strong_type ## SafeTag,		\
    st::equality_comparable>

#define JMG_SAFE_ID_64(strong_type)		\
  using strong_type = st::type<uint64_t,	\
    struct strong_type ## SafeTag,		\
    st::equality_comparable>

namespace jmg
{
template <typename T>
concept StrongType = st::is_strong_type_v<T>;

auto unsafe(StrongType auto safe) {
  return safe.value();
}

} // namespace jmg
