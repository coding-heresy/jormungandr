
#include <gtest/gtest.h>

#include "jmg/array_proxy.h"
#include "jmg/object.h"
#include "jmg/meta.h"
#include "jmg/union.h"

using namespace jmg;
using namespace std;

/**
 * helper macro that simplifies construction of metaprogramming types
 * that can be used to test concepts
 */
#define MAKE_CONCEPT_TESTER(concept)		\
  template<typename T>				\
  struct Is##concept##Concept			\
  { static constexpr bool value = false; };	\
  template<concept T>				\
  struct Is##concept##Concept<T>		\
  { static constexpr bool value = true; }

MAKE_CONCEPT_TESTER(FieldDefT);
MAKE_CONCEPT_TESTER(FieldGroupDefT);
MAKE_CONCEPT_TESTER(FieldOrGroupT);
MAKE_CONCEPT_TESTER(ObjectDefT);

#undef MAKE_CONCEPT_TESTER

JMG_FIELD_DEF(TestField, "field", unsigned, true);

// Some field names
JMG_FIELD_DEF(GroupStringField, "group_string_field", string, true);
JMG_FIELD_DEF(GroupDblField, "group_dbl_field", double, true);
JMG_FIELD_DEF(GroupOptionalField, "group_optional_field", int, false);

using TestFieldGroup = FieldGroupDef<GroupStringField, GroupDblField, GroupOptionalField>;
using TestObject = ObjectDef<TestField, TestFieldGroup>;

TEST(ObjectTests, TestConceptsAndCharacteristics) {
  // TestField is a field, not a field group or object
  EXPECT_TRUE(IsFieldDefTConcept<TestField>::value);
  EXPECT_FALSE(IsFieldGroupDefTConcept<TestField>::value);
  EXPECT_TRUE(IsFieldOrGroupTConcept<TestField>::value);
  EXPECT_FALSE(IsObjectDefTConcept<TestField>::value);

  // TestFieldGroup is a field group, not a field or object
  EXPECT_FALSE(IsFieldDefTConcept<TestFieldGroup>::value);
  EXPECT_TRUE(IsFieldGroupDefTConcept<TestFieldGroup>::value);
  EXPECT_TRUE(IsFieldOrGroupTConcept<TestFieldGroup>::value);
  EXPECT_FALSE(IsObjectDefTConcept<TestFieldGroup>::value);

  // TestObject is an object, not a field or field group
  EXPECT_FALSE(IsFieldDefTConcept<TestObject>::value);
  EXPECT_FALSE(IsFieldGroupDefTConcept<TestObject>::value);
  EXPECT_FALSE(IsFieldOrGroupTConcept<TestObject>::value);
  EXPECT_TRUE(IsObjectDefTConcept<TestObject>::value);

  // double is not a field, field group or object
  EXPECT_FALSE(IsFieldDefTConcept<double>::value);
  EXPECT_FALSE(IsFieldGroupDefTConcept<double>::value);
  EXPECT_FALSE(IsFieldOrGroupTConcept<double>::value);
  EXPECT_FALSE(IsObjectDefTConcept<double>::value);

  // TestField is associated with a value of type 'unsigned' and the
  // field is required to be present in the object
  EXPECT_TRUE((std::is_same_v<unsigned, TestField::type>));
  EXPECT_TRUE(TestField::required{}());

  // GroupOptionalField is associated with a value of type 'int' and
  // the field is not required to be present in the object
  EXPECT_TRUE((std::is_same_v<int, GroupOptionalField::type>));
  EXPECT_FALSE(GroupOptionalField::required{}());

  // TestObject contains 4 fields after TestFieldGroup is properly
  // expanded
  EXPECT_EQ(4, meta::size<TestObject::Fields>{});
}

TEST(ObjectTests, TestArrayProxy) {
  using Vec = vector<int32_t>;
  using ItrProxy = AdaptingConstItrProxy<Vec::const_iterator, int64_t>;
  using ItrPolicy = ProxiedItrPolicy<Vec, ItrProxy>;
  using Proxy = ArrayProxy<Vec, ItrPolicy>;

  const Vec v{ 42 };
  Proxy p{v};

  EXPECT_FALSE(v.empty());
  EXPECT_FALSE(p.empty());

  EXPECT_NE(v.begin(), v.end());
  EXPECT_NE(p.begin(), p.end());

  {
    auto itr = v.begin();
    EXPECT_TRUE((std::is_same_v<decltype(itr), Vec::const_iterator>));
    auto value = *itr;
    EXPECT_TRUE((std::is_same_v<decltype(value), int32_t>));
  }
  {
    auto itr = p.begin();
    EXPECT_FALSE((std::is_same_v<decltype(itr), Vec::const_iterator>));
    auto value = *itr;
    EXPECT_TRUE((std::is_same_v<decltype(value), int64_t>));
  }
}
