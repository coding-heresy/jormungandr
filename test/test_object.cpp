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

#include "jmg/array_proxy.h"
#include "jmg/meta.h"
#include "jmg/object.h"
#include "jmg/union.h"

using namespace jmg;
using namespace std;

/**
 * helper macro that simplifies construction of metaprogramming types
 * that can be used to test concepts
 */
#define MAKE_CONCEPT_TESTER(concept)     \
  template<typename T>                   \
  struct Is##concept##Concept {          \
    static constexpr bool value = false; \
  };                                     \
  template<concept T>                    \
  struct Is##concept##Concept<T> {       \
    static constexpr bool value = true;  \
  }

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

using TestFieldGroup =
  FieldGroupDef<GroupStringField, GroupDblField, GroupOptionalField>;
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
