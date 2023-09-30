#pragma once

namespace jmg
{

/**
 * class template for a union that allows Jormungandr proxy objects to
 * be retrieved for native objects in a way that is integrated into
 * the get/try_get framework
 *
 * @todo add concepts to constrain the specialization to appropriate
 * types
 */
template<typename Obj, typename... Ts>
class Union
{
  using Alternates = meta::list<Ts...>;
public:
  Union() = delete;
  explicit Union(const Obj& obj) : obj_(&obj) {}

  template<typename Tgt>
  Tgt as() const requires (isMemberOfList<Tgt, Alternates>()) {
    return Tgt(*obj_);
  }

private:
  const Obj* obj_;
};

////////////////////////////////////////////////////////////////////////////////
// Union concept
////////////////////////////////////////////////////////////////////////////////
namespace detail
{
template<typename T>
struct IsUnionImpl { using type = std::false_type; };
template<typename T, typename... Ts>
struct IsUnionImpl<Union<T, Ts...>> { using type = std::true_type; };
} // namespace detail

template<typename T>
using IsUnionT = meta::_t<detail::IsUnionImpl<T>>;

template<typename T>
concept UnionT = IsUnionT<T>{}();
} // namespace jmg
