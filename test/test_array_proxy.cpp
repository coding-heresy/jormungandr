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

#include <gtest/gtest.h>

#include <ranges>
#include <span>
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

TEST(ArrayProxyTests, TestTrivialViewProxy) {
  // non-owning proxy for a vector of integers that can be iterated
  // over using the native iterator
  using IntVecProxy = ViewingArrayProxy<IntVec, RawItrPolicy<IntVec>>;
  IntVecProxy proxy{raw};
  EXPECT_FALSE(proxy.empty());
  auto pred = [](const auto val) { return 3 == val; };
  const auto entry = ranges::find_if(proxy, pred);
  EXPECT_NE(proxy.end(), entry);
  EXPECT_EQ(3, *entry);
}

TEST(ArrayProxyTests, TestAdaptingViewProxy) {
  // proxy object that wraps (and owns) a raw integer
  struct IntOwningProxy {
    explicit IntOwningProxy(int i) : val(i) {}
    int val;
  };

  // proxy for an iterator over a container where the entries are
  // adapted using IntOwningProxy
  using ItrProxy =
    AdaptingConstItrProxy<IntVec::const_iterator, IntOwningProxy>;
  EXPECT_TRUE(isInputIterator<ItrProxy>());
  EXPECT_TRUE(isInputOrOutputIterator<ItrProxy>());
  // policy declaring that a iteration over a vector of integers will
  // be accomplished via proxy
  using ItrPolicy = ProxiedItrPolicy<IntVec, ItrProxy>;
  // non-owning proxy for a vector of integers that can be iterated
  // over using a proxy iterator
  using AdaptingProxy = ViewingArrayProxy<IntVec, ItrPolicy>;

  const AdaptingProxy proxy{raw};
  EXPECT_TRUE(!proxy.empty());
  EXPECT_NE(proxy.begin(), proxy.end());
  auto pred = [](const auto val) { return 3 == val.val; };

  {
    // check that std::ranges iteration works
    const auto entry = ranges::find_if(proxy, pred);
    EXPECT_NE(proxy.begin(), entry);
    EXPECT_NE(proxy.end(), entry);
    EXPECT_EQ(3, (*entry).val);
  }

  {
    // check that STL iteration works
    const auto entry = find_if(proxy.begin(), proxy.end(), pred);
    EXPECT_NE(proxy.begin(), entry);
    EXPECT_NE(proxy.end(), entry);
    EXPECT_EQ(3, (*entry).val);
  }
}

TEST(ArrayProxyTests, TestOwningProxy) {
  auto spanAsProxy = span{raw};
  using SpanAsProxy = decltype(spanAsProxy);

  // owning proxy that owns the span that is viewing the array
  using IntVecSpanProxy = OwningArrayProxy<SpanAsProxy>;
  // @todo: OLD CODE IntVecSpanProxy proxy{span{raw}};
  IntVecSpanProxy proxy{std::move(spanAsProxy)};
  EXPECT_FALSE(proxy.empty());

  // check that std::ranges iteration works
  auto pred = [](const auto val) { return 1 == val; };
  const auto entry = ranges::find_if(proxy, pred);
  EXPECT_EQ(proxy.begin(), entry);
  EXPECT_NE(proxy.end(), entry);
  EXPECT_EQ(1, *entry);
}
