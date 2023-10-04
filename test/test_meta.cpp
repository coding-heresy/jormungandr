
#include <iostream>
#include <string>

#include <gtest/gtest.h>
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
    EXPECT_TRUE((is_same_v<DefaultPolicy1,
		 PolicyResolverT<Policy1Tag,
		 DefaultPolicy1,
		 AllTags,
		 AllDefaultPolicies>>));
    EXPECT_TRUE((is_same_v<DefaultPolicy2,
		 PolicyResolverT<Policy2Tag,
		 DefaultPolicy2,
		 AllTags,
		 AllDefaultPolicies>>));
  }
  {
    using Default1Optional2 = meta::list<OptionalPolicy2>;
    EXPECT_TRUE((is_same_v<DefaultPolicy1,
		 PolicyResolverT<Policy1Tag,
		 DefaultPolicy1,
		 AllTags,
		 Default1Optional2>>));
    EXPECT_TRUE((is_same_v<OptionalPolicy2,
		 PolicyResolverT<Policy2Tag,
		 DefaultPolicy2,
		 AllTags,
		 Default1Optional2>>));
  }
  {
    using Optional1Default2 = meta::list<OptionalPolicy1>;
    EXPECT_TRUE((is_same_v<OptionalPolicy1,
		 PolicyResolverT<Policy1Tag,
		 DefaultPolicy1,
		 AllTags,
		 Optional1Default2>>));
    EXPECT_TRUE((is_same_v<DefaultPolicy2,
		 PolicyResolverT<Policy2Tag,
		 DefaultPolicy2,
		 AllTags,
		 Optional1Default2>>));
  }
  {
    using AllOptionalPolicies = meta::list<OptionalPolicy1, OptionalPolicy2>;
    EXPECT_TRUE((is_same_v<OptionalPolicy1,
		 PolicyResolverT<Policy1Tag,
		 DefaultPolicy1,
		 AllTags,
		 AllOptionalPolicies>>));
    EXPECT_TRUE((is_same_v<OptionalPolicy2,
		 PolicyResolverT<Policy2Tag,
		 DefaultPolicy2,
		 AllTags,
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
