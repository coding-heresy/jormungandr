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
#include <tuple>

#include <meta/meta.hpp>

#include "jmg/meta.h"
#include "jmg/types.h"

using namespace jmg;
using namespace std;

TEST(MetaprogrammingTests, TestTypeListConcept) {
  using TestList = meta::list<bool, float>;
  EXPECT_TRUE(TypeListT<TestList>);
  EXPECT_FALSE(TypeListT<int>);
}

TEST(MetaprogrammingTests, TestDecayedSameAs) {
  int val;
  const int& ref = val;
  const int& const_ref = val;
  using ValT = decltype(val);
  using RefT = decltype(ref);
  using ConstRefT = decltype(const_ref);
  // ValT is not the same as reference types
  EXPECT_FALSE((std::same_as<ValT, RefT>));
  EXPECT_FALSE((std::same_as<ValT, ConstRefT>));
  // all combinations of fully decayed types are the same as all others
  EXPECT_TRUE((DecayedSameAsT<ValT, RefT>));
  EXPECT_TRUE((DecayedSameAsT<ValT, ConstRefT>));
  EXPECT_TRUE((DecayedSameAsT<RefT, ValT>));
  EXPECT_TRUE((DecayedSameAsT<ConstRefT, ValT>));
  EXPECT_TRUE((DecayedSameAsT<RefT, ConstRefT>));
  EXPECT_TRUE((DecayedSameAsT<ConstRefT, RefT>));
}

TEST(MetaprogrammingTests, TestSameAsDecayed) {
  int val;
  int& ref = val;
  const int& const_ref = val;
  using ValT = decltype(val);
  using RefT = decltype(ref);
  using ConstRefT = decltype(const_ref);
  // only ValT is the same as itself
  EXPECT_TRUE((std::same_as<ValT, ValT>));
  EXPECT_FALSE((std::same_as<ValT, RefT>));
  EXPECT_FALSE((std::same_as<ValT, ConstRefT>));
  // ValT is the same as itself and the decayed type of any reference type
  EXPECT_TRUE((SameAsDecayedT<ValT, ValT>));
  EXPECT_TRUE((SameAsDecayedT<ValT, RefT>));
  EXPECT_TRUE((SameAsDecayedT<ValT, ConstRefT>));
  // decayed types of reference types are not the same as themselves
  EXPECT_FALSE((SameAsDecayedT<RefT, RefT>));
  EXPECT_FALSE((SameAsDecayedT<ConstRefT, ConstRefT>));
}

TEST(MetaprogrammingTests, TestNumericConcepts) {
  // integer types
  EXPECT_FALSE(IntegralT<bool>);
  EXPECT_FALSE(IntegralT<float>);
  EXPECT_TRUE(IntegralT<int>);
  // floating point types
  EXPECT_FALSE(FloatingPointT<bool>);
  EXPECT_TRUE(FloatingPointT<float>);
  EXPECT_FALSE(FloatingPointT<int>);
  // generic numeric types
  EXPECT_FALSE(ArithmeticT<bool>);
  EXPECT_TRUE(ArithmeticT<float>);
  EXPECT_TRUE(ArithmeticT<int>);
}

TEST(MetaprogrammingTests, TestCStyleStringConcept) {
  EXPECT_TRUE(CStyleStringT<const char*>);
  auto* literal = "foo";
  EXPECT_TRUE(CStyleStringT<decltype(literal)>);
  constexpr char compile_time[] = "bar";
  EXPECT_TRUE(CStyleStringT<decltype(compile_time)>);
  EXPECT_TRUE(CStyleStringT<decltype("blub")>);
  char barf[5];
  EXPECT_TRUE(CStyleStringT<decltype(barf)>);
  const char const_barf[5] = "barf";
  EXPECT_TRUE(CStyleStringT<decltype(const_barf)>);

  EXPECT_FALSE(CStyleStringT<int>);

  // std::string and std::string_view don't match
  EXPECT_FALSE(CStyleStringT<string>);
  EXPECT_FALSE(CStyleStringT<string_view>);
}

TEST(MetaprogrammingTests, TestStringLikeConcepts) {
  EXPECT_FALSE(StringLikeT<int>);

  // check all the same cases as CStyleStringT
  EXPECT_TRUE(StringLikeT<const char*>);
  auto* literal = "foo";
  EXPECT_TRUE(StringLikeT<decltype(literal)>);
  constexpr char compile_time[] = "bar";
  EXPECT_TRUE(StringLikeT<decltype(compile_time)>);
  EXPECT_TRUE(StringLikeT<decltype("blub")>);
  char barf[5];
  EXPECT_TRUE(StringLikeT<decltype(barf)>);
  const char const_barf[5] = "barf";
  EXPECT_TRUE(StringLikeT<decltype(const_barf)>);

  // also check std::string and std::string_view
  EXPECT_TRUE(StringLikeT<string>);
  EXPECT_TRUE(StringLikeT<string_view>);
}

TEST(MetaprogrammingTests, TestViewStringConcepts) {
  // EXPECT_FALSE(StringLikeT<int>);

  auto* literal = "foo";
  constexpr char compile_time[] = "bar";

  EXPECT_TRUE(NonViewStringT<string>);
  EXPECT_TRUE(NonViewStringT<const char*>);
  EXPECT_TRUE(NonViewStringT<decltype(literal)>);
  EXPECT_TRUE(NonViewStringT<decltype(compile_time)>);
  EXPECT_FALSE(NonViewStringT<string_view>);

  EXPECT_FALSE(ViewStringT<string>);
  EXPECT_FALSE(ViewStringT<const char*>);
  EXPECT_FALSE(ViewStringT<decltype(literal)>);
  EXPECT_FALSE(ViewStringT<decltype(compile_time)>);
  EXPECT_TRUE(ViewStringT<string_view>);
}

TEST(MetaprogrammingTests, TestMiscStringConcepts) {
  EXPECT_TRUE(StaticStringConstT<decltype("foo")>);

  EXPECT_FALSE(NonStringClassT<string>);
  EXPECT_FALSE(NonStringClassT<int>);
  EXPECT_TRUE(NonStringClassT<BufferView>);
}

TEST(MetaprogrammingTests, TestSpanAndVectorConcepts) {
  EXPECT_FALSE(VectorT<int>);
  EXPECT_FALSE(SpanT<int>);
  EXPECT_TRUE(VectorT<vector<int>>);
  EXPECT_FALSE(VectorT<span<int>>);
  EXPECT_FALSE(SpanT<vector<int>>);
  EXPECT_TRUE(SpanT<span<int>>);
  array<float, 5> floats;
  const auto floats_span = span(floats);
  EXPECT_TRUE(SpanT<decltype(floats_span)>);
}

TEST(MetaprogrammingTests, TestClassAndNonClassConcepts) {
  EXPECT_TRUE(ClassT<string>);
  EXPECT_FALSE(NonClassT<string>);
  EXPECT_FALSE(ClassT<double>);
  EXPECT_TRUE(NonClassT<double>);

  EXPECT_TRUE(NonClassT<const char*>);
  auto* literal = "foo";
  EXPECT_TRUE(NonClassT<decltype(literal)>);
  constexpr char compile_time[] = "bar";
  EXPECT_TRUE(NonClassT<decltype(compile_time)>);
}

using namespace meta::placeholders;
template<typename T1, typename T2>
struct Matched : std::integral_constant<uint8_t, 0> {};
template<typename T>
struct Matched<T, T> : std::integral_constant<uint8_t, 1> {};
template<typename T>
using CountMatchesLambda =
  meta::lambda<_a, _b, meta::lazy::plus<_a, Matched<T, _b>>>;
template<typename T, TypeListT Lst>
using CountMatches =
  meta::fold<Lst, std::integral_constant<uint8_t, 0>, CountMatchesLambda<T>>;

TEST(MetaprogrammingTests, TestListMembershipHelpers) {
  using List = meta::list<int, double, string>;
  EXPECT_TRUE((isMemberOfList<int, List>()));
  EXPECT_FALSE((isMemberOfList<char, List>()));

  // isMemberOfList should work when the list is constructed directly
  // from a parameter pack
  auto dbl_checker = []<typename... Args>(Args&&...) {
    return isMemberOfList<double, meta::list<Args...>>();
  };
  EXPECT_TRUE(dbl_checker(20010911, 42.0, "foo"s));

  // list type arguments should be decayed before checking
  const auto dbl_val = 42.0;
  const auto& dbl_ref = dbl_val;
  EXPECT_TRUE(dbl_checker(20010911, dbl_ref, "foo"s));

  // confirm that unique membership test works
  EXPECT_TRUE((isUniqueMemberOfList<int, List>()));
  EXPECT_FALSE((isUniqueMemberOfList<float, List>()));

  using DuplicateList = meta::list<int, double, string, int>;
  EXPECT_TRUE((isUniqueMemberOfList<double, DuplicateList>()));
  EXPECT_FALSE((isUniqueMemberOfList<int, DuplicateList>()));

  // confirm that "at most once" membership test works
  EXPECT_TRUE((isAtMostOnceMemberOfList<double, DuplicateList>()));
  EXPECT_TRUE((isAtMostOnceMemberOfList<float, DuplicateList>()));
  EXPECT_FALSE((isAtMostOnceMemberOfList<int, DuplicateList>()));

  // confirm that scoped enums work correctly
  enum class Enum { kFoo, kBar };
  using ListWithEnum = meta::list<int, double, Enum, string>;
  EXPECT_TRUE((isMemberOfList<Enum, ListWithEnum>()));
  EXPECT_TRUE((isUniqueMemberOfList<Enum, ListWithEnum>()));

  []<typename... Args>(Args&&...) {
    using ArgsList = meta::list<Args...>;
    EXPECT_TRUE((isMemberOfList<Enum, ArgsList>()));
  }(15, Enum::kBar);
}

TEST(MetaprogrammingTests, TestEntryIdx) {
  using TestList =
    meta::list<bool, uint8_t, uint16_t, uint32_t, uint64_t, float, double>;
  size_t idx = 0;
  EXPECT_EQ(idx++, (entryIdx<bool, TestList>()));
  EXPECT_EQ(idx++, (entryIdx<uint8_t, TestList>()));
  EXPECT_EQ(idx++, (entryIdx<uint16_t, TestList>()));
  EXPECT_EQ(idx++, (entryIdx<uint32_t, TestList>()));
  EXPECT_EQ(idx++, (entryIdx<uint64_t, TestList>()));
  EXPECT_EQ(idx++, (entryIdx<float, TestList>()));
  EXPECT_EQ(idx, (entryIdx<double, TestList>()));
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

TEST(MetaprogrammingTests, TestOptionalConcept) {
  using TestType = int;
  EXPECT_FALSE(OptionalT<TestType>);
  using OptionalType = optional<int>;
  EXPECT_TRUE(OptionalT<OptionalType>);
}

TEST(MetaprogrammingTests, TestRemoveOptional) {
  using TestType = int;
  EXPECT_TRUE((same_as<TestType, RemoveOptionalT<TestType>>));
  using OptionalType = optional<int>;
  EXPECT_TRUE((same_as<TestType, RemoveOptionalT<OptionalType>>));
}

TEST(MetaprogrammingTests, TestEnumConcepts) {
  enum Enum { kFoo, kBar };
  enum class ScopedEnum { kFoo, kBar };

  const auto enumFoo = Enum::kFoo;
  const auto scopedEnumFoo = ScopedEnum::kFoo;
  EXPECT_TRUE(EnumT<decltype(enumFoo)>);
  EXPECT_FALSE(ScopedEnumT<decltype(enumFoo)>);
  EXPECT_FALSE(EnumT<decltype(scopedEnumFoo)>);
  EXPECT_TRUE(ScopedEnumT<decltype(scopedEnumFoo)>);
}

TEST(MetaprogrammingTests, TestTupleHandling) {
  using TestTuple = tuple<int, float>;
  EXPECT_TRUE(isTuple<TestTuple>());
  EXPECT_FALSE(isTuple<int>());
  using TestTypeList = meta::list<int, float>;
  EXPECT_TRUE((same_as<TestTypeList, DeTuplize<TestTuple>>));
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
