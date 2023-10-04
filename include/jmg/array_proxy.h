#pragma once

#include <ranges>

#include "jmg/meta.h"
#include "jmg/preprocessor.h"

namespace jmg
{

/**
 * proxy for a const_iterator type that returns a stashed proxy to the
 * actual value in the container when dereferenced
 */
template<typename ProxiedItrT, typename ValueProxyT>
class StashingConstItrProxy
{
public:
  using difference_type = ptrdiff_t;
  using value_type = ValueProxyT;
  using pointer = ValueProxyT*;
  using reference = ValueProxyT&;
  using iterator_concept = std::input_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  explicit StashingConstItrProxy() = default;
  JMG_DEFAULT_MOVEABLE(StashingConstItrProxy);
  JMG_DEFAULT_COPYABLE(StashingConstItrProxy);
  explicit StashingConstItrProxy(ProxiedItrT&& itr) : itr_(std::move(itr)) {
    if (itr_) { stash_ = ValueProxyT{*(itr_.value())}; }
  }

  const ValueProxyT& operator*() const { return stash_.value(); }
  const ValueProxyT* operator->() const { return &(stash_.value()); }

  StashingConstItrProxy& operator++() {
    ++(itr_.value());
    stash_ = ValueProxyT{*(itr_.value())};
    return *this;
  }

  StashingConstItrProxy operator++(int) {
    auto rslt = *this;
    ++(itr_.value());
    stash_ = ValueProxyT{*(itr_.value())};
    return rslt;
  }

  friend bool operator==(const StashingConstItrProxy& lhs, const StashingConstItrProxy& rhs) {
    return lhs.itr_ == rhs.itr_;
  }

  friend bool operator!=(const StashingConstItrProxy& lhs, const StashingConstItrProxy& rhs) {
    return lhs.itr_ != rhs.itr_;
  }
protected:
  std::optional<ProxiedItrT> itr_;
  std::optional<ValueProxyT> stash_;
};

/**
 * proxy for a const_iterator type that returns a proxy to the actual
 * value in the container when dereferenced
 */
template<typename ProxiedItrT, typename ValueProxyT>
class AdaptingConstItrProxy
{
public:
  using difference_type = ptrdiff_t;
  using value_type = ValueProxyT;
  using reference = ValueProxyT;
  using iterator_category = std::input_iterator_tag;

  AdaptingConstItrProxy() = default;
  JMG_DEFAULT_MOVEABLE(AdaptingConstItrProxy);
  JMG_DEFAULT_COPYABLE(AdaptingConstItrProxy);
  explicit AdaptingConstItrProxy(ProxiedItrT&& itr) : itr_(std::move(itr)) {}

  ValueProxyT operator*() const { return ValueProxyT{*itr_}; }

  AdaptingConstItrProxy& operator++() {
    ++itr_;
    return *this;
  }

  AdaptingConstItrProxy operator++(int) {
    auto rslt = *this;
    ++itr_;
    return rslt;
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
struct RawItrPolicy : ItrPolicyTag
{
  static auto begin(const SrcContainerT* src) { return std::begin(*src); }
  static auto end(const SrcContainerT* src) { return std::end(*src); }
  // NOTE: begin/end of non-const containers should return const
  // iterators at this point
  static auto begin(SrcContainerT* src) { return std::cbegin(*src); }
  static auto end(SrcContainerT src) { return std::cend(*src); }
  static auto cbegin(const SrcContainerT* src) { return std::cbegin(*src); }
  static auto cend(const SrcContainerT* src) { return std::cend(*src); }
};

/**
 * policy that specifies using a proxy for iterating over elements in
 * ArrayProxy
 */
template<typename SrcContainerT, typename ItrProxyT>
struct ProxiedItrPolicy : ItrPolicyTag
{
  static auto begin(const SrcContainerT* src) { return ItrProxyT{std::begin(*src)}; }
  static auto end(const SrcContainerT* src) { return ItrProxyT{std::end(*src)}; }
  // NOTE: begin/end of non-const containers should return const
  // iterators at this point
  static auto begin(SrcContainerT* src) { return ItrProxyT{std::cbegin(*src)}; }
  static auto end(SrcContainerT* src) { return ItrProxyT{std::cend(*src)}; }
  static auto cbegin(const SrcContainerT* src) { return ItrProxyT{std::cbegin(*src)}; }
  static auto cend(const SrcContainerT* src) { return ItrProxyT{std::cend(*src)}; }
};

struct SizePolicyTag {};

/**
 * policy that retrieves the size of the array by directly querying
 * the raw container interface
 */
template<typename SrcContainerT>
struct DefaultSizePolicy : SizePolicyTag
{
  static size_t size(const SrcContainerT* src) {
    return src->size();
  }
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
  using AllPolicyTags = meta::list<ItrPolicyTag, SizePolicyTag>;
  using DefaultItrPolicy = RawItrPolicy<ProxiedT>;
  using ItrPolicy = PolicyResolverT<ItrPolicyTag, DefaultItrPolicy, AllPolicyTags, Policies>;
  using DefaultSzPolicy = DefaultSizePolicy<ProxiedT>;
  using SizePolicy = PolicyResolverT<SizePolicyTag, DefaultSzPolicy, AllPolicyTags, Policies>;

public:
  ArrayProxy() = delete;
  explicit ArrayProxy(const ProxiedT& src) : src_(&src) {}

  auto size() const { return SizePolicy::size(src_); }
  auto empty() const { return 0 == size(); }

  auto begin() const { return ItrPolicy::begin(src_); }
  auto end() const { return ItrPolicy::end(src_); }
  auto begin() { return ItrPolicy::begin(src_); }
  auto end() { return ItrPolicy::end(src_); }
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
