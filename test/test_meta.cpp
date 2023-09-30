
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
  struct BasePolicy1 {};
  struct DefaultPolicy1 : BasePolicy1 {};
  struct OptionalPolicy1 : BasePolicy1 {};

  struct BasePolicy2 {};
  struct DefaultPolicy2 : BasePolicy2 {};
  struct OptionalPolicy2 : BasePolicy2 {};

  {
    using AllDefaultPolicies = meta::list<>;
    EXPECT_TRUE((is_same_v<DefaultPolicy1,
		 PolicyResolverT<BasePolicy1,
		 DefaultPolicy1,
		 AllDefaultPolicies>>));
    EXPECT_TRUE((is_same_v<DefaultPolicy2,
		 PolicyResolverT<BasePolicy2,
		 DefaultPolicy2,
		 AllDefaultPolicies>>));
  }
  {
    using Default1Optional2 = meta::list<OptionalPolicy2>;
    EXPECT_TRUE((is_same_v<DefaultPolicy1,
		 PolicyResolverT<BasePolicy1,
		 DefaultPolicy1,
		 Default1Optional2>>));
    EXPECT_TRUE((is_same_v<OptionalPolicy2,
		 PolicyResolverT<BasePolicy2,
		 DefaultPolicy2,
		 Default1Optional2>>));
  }
  {
    using Optional1Default2 = meta::list<OptionalPolicy1>;
    EXPECT_TRUE((is_same_v<OptionalPolicy1,
		 PolicyResolverT<BasePolicy1,
		 DefaultPolicy1,
		 Optional1Default2>>));
    EXPECT_TRUE((is_same_v<DefaultPolicy2,
		 PolicyResolverT<BasePolicy2,
		 DefaultPolicy2,
		 Optional1Default2>>));
  }
  {
    using AllOptionalPolicies = meta::list<OptionalPolicy1, OptionalPolicy2>;
    EXPECT_TRUE((is_same_v<OptionalPolicy1,
		 PolicyResolverT<BasePolicy1,
		 DefaultPolicy1,
		 AllOptionalPolicies>>));
    EXPECT_TRUE((is_same_v<OptionalPolicy2,
		 PolicyResolverT<BasePolicy2,
		 DefaultPolicy2,
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
