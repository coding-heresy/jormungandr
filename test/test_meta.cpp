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

#include <iostream>
#include <string>

#include <meta/meta.hpp>

#include "jmg/meta.h"

using namespace jmg;
using namespace std;

namespace
{
// support code for testing the TypeList concept using specialization
template<typename T>
struct IsTypeList {
  static constexpr bool value = false;
};

template<TypeList T>
struct IsTypeList<T> {
  static constexpr bool value = true;
};
} // namespace

TEST(MetaprogrammingTests, TestTypeListConcept) {
  using TestList = meta::list<bool, float>;
  EXPECT_TRUE(IsTypeList<TestList>::value);
  EXPECT_FALSE(IsTypeList<int>::value);
}

TEST(MetaprogrammingTests, TestPolicyResolver) {
  struct Policy1Tag {};
  struct DefaultPolicy1 : Policy1Tag {};
  struct OptionalPolicy1 : Policy1Tag {};

  struct Policy2Tag {};
  struct DefaultPolicy2 : Policy2Tag {};
  struct OptionalPolicy2 : Policy2Tag {};

  using AllTags = meta::list<Policy1Tag, Policy2Tag>;
  {
    using AllDefaultPolicies = meta::list<>;
    EXPECT_TRUE((
      is_same_v<DefaultPolicy1, PolicyResolverT<Policy1Tag, DefaultPolicy1,
                                                AllTags, AllDefaultPolicies>>));
    EXPECT_TRUE((
      is_same_v<DefaultPolicy2, PolicyResolverT<Policy2Tag, DefaultPolicy2,
                                                AllTags, AllDefaultPolicies>>));
  }
  {
    using Default1Optional2 = meta::list<OptionalPolicy2>;
    EXPECT_TRUE(
      (is_same_v<DefaultPolicy1, PolicyResolverT<Policy1Tag, DefaultPolicy1,
                                                 AllTags, Default1Optional2>>));
    EXPECT_TRUE((
      is_same_v<OptionalPolicy2, PolicyResolverT<Policy2Tag, DefaultPolicy2,
                                                 AllTags, Default1Optional2>>));
  }
  {
    using Optional1Default2 = meta::list<OptionalPolicy1>;
    EXPECT_TRUE((
      is_same_v<OptionalPolicy1, PolicyResolverT<Policy1Tag, DefaultPolicy1,
                                                 AllTags, Optional1Default2>>));
    EXPECT_TRUE(
      (is_same_v<DefaultPolicy2, PolicyResolverT<Policy2Tag, DefaultPolicy2,
                                                 AllTags, Optional1Default2>>));
  }
  {
    using AllOptionalPolicies = meta::list<OptionalPolicy1, OptionalPolicy2>;
    EXPECT_TRUE((is_same_v<OptionalPolicy1,
                           PolicyResolverT<Policy1Tag, DefaultPolicy1, AllTags,
                                           AllOptionalPolicies>>));
    EXPECT_TRUE((is_same_v<OptionalPolicy2,
                           PolicyResolverT<Policy2Tag, DefaultPolicy2, AllTags,
                                           AllOptionalPolicies>>));
  }
}

using namespace std::literals::string_literals;

TEST(MetaprogrammingTests, TestTypeNameDemangler) {
  EXPECT_EQ("double"s, type_name_for<double>());
  const int i = 42;
  EXPECT_EQ("int"s, type_name_for(i));
}

TEST(MetaprogrammingTests, TestExceptionTypeName) {
  try {
    const int i = 42;
    throw i;
  }
  catch (...) {
    EXPECT_EQ("int"s, current_exception_type_name());
  }
}
