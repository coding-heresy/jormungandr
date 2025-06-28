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

#if defined(YAML_ENCODING_TEST)
#include "test/jmg_yaml_example.h"
#elif defined(CBE_ENCODING_TEST)
#include "test/jmg_cbe_example.h"
#else
#error "unknown encoding type"
#endif

using namespace example;
using namespace jmg;
#if defined(CBE_ENCODING_TEST)
using namespace jmg::cbe;
#endif
using namespace std;

// NOTE: these test cases must be kept in sync with data/example.yaml

TEST(JmgExampleTests, TestTypeAliases) {
  EXPECT_TRUE(SafeT<ZipCode>);
  EXPECT_TRUE((is_same_v<string, UnsafeTypeFromT<ZipCode>>));

  EXPECT_TRUE(SafeT<Age>);
  EXPECT_TRUE((is_same_v<uint8_t, UnsafeTypeFromT<Age>>));
}

TEST(JmgExampleTests, TestEnums) {
  EXPECT_TRUE(is_enum_v<TestEnum>);
  EXPECT_TRUE(is_enum_v<TestEnumWithUlType>);
}

TEST(JmgExampleTests, TestGroups) {
  EXPECT_TRUE(FieldGroupDefT<Address>);
  // TODO(bd) verify the fields that are in Address
}

TEST(JmgExampleTests, TestFields) {
  EXPECT_TRUE(FieldDefT<Number>);
  EXPECT_TRUE(RequiredField<Number>);
  EXPECT_TRUE(RequiredField<Street>);
  EXPECT_TRUE(OptionalField<Apartment>);
  EXPECT_TRUE(RequiredField<City>);
  EXPECT_TRUE(RequiredField<State>);
  EXPECT_TRUE(RequiredField<Zip>);
  EXPECT_TRUE(RequiredField<FirstName>);
  EXPECT_TRUE(RequiredField<LastName>);
  EXPECT_TRUE(OptionalField<MiddleName>);
  EXPECT_TRUE(RequiredField<PersonAge>);
  EXPECT_TRUE(RequiredField<Ints>);
  EXPECT_TRUE(RequiredField<Reals>);
#if defined(CBE_ENCODING_TEST)
  // TODO(bd) fix these concepts to work correctly
  // EXPECT_TRUE(StringFieldT<Street>);
  // EXPECT_TRUE(StringFieldT<City>);
  // EXPECT_TRUE(StringFieldT<State>);
  // EXPECT_TRUE(StringFieldT<FirstName>);
  // EXPECT_TRUE(StringFieldT<LastName>);
  // EXPECT_TRUE(StringFieldT<MiddleName>);
  // EXPECT_TRUE(ArrayFieldT<Ints>);
  // EXPECT_TRUE(ArrayFieldT<Reals>);
#endif
}

TEST(JmgExampleTests, TestObjects) {
  EXPECT_TRUE(ObjectDefT<Person>);
  EXPECT_TRUE((isMemberOfObject<FirstName, Person>()));
  EXPECT_TRUE((isMemberOfObject<LastName, Person>()));
  EXPECT_TRUE((isMemberOfObject<MiddleName, Person>()));
  EXPECT_TRUE((isMemberOfObject<PersonAge, Person>()));

  EXPECT_TRUE(ObjectDefT<Numbers>);
  EXPECT_TRUE((isMemberOfObject<Ints, Numbers>()));
  EXPECT_TRUE((isMemberOfObject<Reals, Numbers>()));
}
