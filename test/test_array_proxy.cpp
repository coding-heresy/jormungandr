
#include <gtest/gtest.h>

#include <ranges>
#include <vector>

#include "jmg/array_proxy.h"

using namespace jmg;
using namespace std;

JMG_MAKE_CONCEPT_CHECKER(InputIterator, std::input_iterator)
JMG_MAKE_CONCEPT_CHECKER(InputOrOutputIterator, std::input_or_output_iterator)

namespace
{
using IntVec = vector<int>;
const IntVec raw{1, 2, 3};
} // namespace

TEST(ArrayProxyTests, TestTrivialProxy) {
  using IntVecProxy = ArrayProxy<IntVec, RawItrPolicy<IntVec>>;
  IntVecProxy proxy{raw};
  EXPECT_TRUE(!proxy.empty());
  auto pred = [](const auto val) { return 3 == val; };
  const auto entry = ranges::find_if(proxy, pred);
  EXPECT_NE(proxy.end(), entry);
  EXPECT_EQ(3, *entry);
}

TEST(ArrayProxyTests, TestAdaptingProxy) {
  struct IntProxy {
    explicit IntProxy(int i) : val(i) {}
    int val;
  };

  using ItrProxy = AdaptingConstItrProxy<IntVec::const_iterator, IntProxy>;
  EXPECT_TRUE(isInputIterator<ItrProxy>());
  EXPECT_TRUE(isInputOrOutputIterator<ItrProxy>());
  using ItrPolicy = ProxiedItrPolicy<IntVec, ItrProxy>;
  using AdaptingProxy = ArrayProxy<IntVec, ItrPolicy>;

  const AdaptingProxy proxy{raw};
  EXPECT_TRUE(!proxy.empty());
  auto pred = [](const auto val) { return 3 == val.val; };

  {
    // std::ranges iteration works
    const auto entry = ranges::find_if(proxy, pred);
    EXPECT_NE(proxy.end(), entry);
    EXPECT_NE(proxy.begin(), entry);
    EXPECT_EQ(3, (*entry).val);
  }

  {
    // STL iteration works
    const auto entry = find_if(proxy.begin(), proxy.end(), pred);
    EXPECT_NE(proxy.end(), entry);
    EXPECT_NE(proxy.begin(), entry);
    EXPECT_EQ(3, (*entry).val);
  }
}
