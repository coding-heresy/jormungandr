#pragma once

#include <ranges>

#include "jmg/meta.h"

namespace jmg
{

/**
 * proxy for a const_iterator type that returns a proxy to the actual
 * value in the container when dereferenced
 */
template<typename ProxiedItrT, typename ValueProxyT>
class AdaptingConstItrProxy
{
public:
  using value_type = ValueProxyT;

  AdaptingConstItrProxy() = delete;
  explicit AdaptingConstItrProxy(ProxiedItrT&& itr) : itr_(std::move(itr)) {}

  ValueProxyT operator*() const { return ValueProxyT{*itr_}; }

  void operator++() {
    ++itr_;
  }

  friend bool operator==(const AdaptingConstItrProxy& lhs, const AdaptingConstItrProxy& rhs) {
    return lhs.itr_ == rhs.itr_;
  }

  friend bool operator!=(const AdaptingConstItrProxy& lhs, const AdaptingConstItrProxy& rhs) {
    return lhs.itr_ != rhs.itr_;
  }
protected:
  ProxiedItrT itr_;
};

/**
 * tag that supports using the PolicyResolverT infrastructure to allow
 * specification of policies for how to iterate over the elements of a
 * container proxied with ArrayProxy
 */
struct ItrPolicyTag {};

/**
 * policy that specifies using the raw iterator from the underlying
 * container in ArrayProxy
 */
template<typename SrcContainerT>
struct RawItrPolicy
{
  static auto begin(const SrcContainerT* src) { return std::begin(*src); }
  static auto end(const SrcContainerT* src) { return std::end(*src); }
  static auto cbegin(const SrcContainerT* src) { return std::cbegin(*src); }
  static auto cend(const SrcContainerT* src) { return std::cend(*src); }
};

/**
 * policy that specifies using a proxy for iterating over elements in
 * ArrayProxy
 */
template<typename SrcContainerT, typename ItrProxyT>
struct ProxiedItrPolicy : public ItrPolicyTag
{
  static auto begin(const SrcContainerT* src) { return ItrProxyT{std::begin(*src)}; }
  static auto end(const SrcContainerT* src) { return ItrProxyT{std::end(*src)}; }
  static auto cbegin(const SrcContainerT* src) { return ItrProxyT{std::cbegin(*src)}; }
  static auto cend(const SrcContainerT* src) { return ItrProxyT{std::cend(*src)}; }
};

/**
 * class template for a proxy that can be used to wrap an adapter some
 * container type to look like an array
 *
 * TODO add more array-like features (such as random access) to the
 * proxy
 */
template<typename ProxiedT, typename... Ps>
class ArrayProxy
{
  using Policies = meta::list<Ps...>;
  using DefaultItrPolicy = RawItrPolicy<ProxiedT>;
  using ItrPolicy = PolicyResolverT<ItrPolicyTag, DefaultItrPolicy, Policies>;
public:
  ArrayProxy() = delete;
  explicit ArrayProxy(const ProxiedT& src) : src_(&src) {}

  auto size() const { return src_->size(); }
  auto empty() const { return src_->empty(); }

  auto begin() const { return ItrPolicy::begin(src_); }
  auto end() const { return ItrPolicy::end(src_); }
  auto cbegin() const { return ItrPolicy::cbegin(src_); }
  auto cend() const { return ItrPolicy::cend(src_); }

private:
  const ProxiedT* src_;
};

// TODO add support for an array proxy that owns the data it is proxying

////////////////////////////////////////////////////////////////////////////////
// ArrayProxyT concept
////////////////////////////////////////////////////////////////////////////////
namespace detail
{
template<typename T>
struct IsArrayProxyImpl { using type = std::false_type; };
template<typename... Ts>
struct IsArrayProxyImpl<ArrayProxy<Ts...>> { using type = std::true_type; };
} // namespace detail
template <typename T>
using IsArrayProxyT = meta::_t<detail::IsArrayProxyImpl<T>>;

template<typename T>
concept ArrayProxyT = IsArrayProxyT<T>{}();

} // namespace jmg
