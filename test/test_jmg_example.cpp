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

#include "test/jmg_example.h"

using namespace example;
using namespace jmg;
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

TEST(JmgExampleTests, TestObjects) {
  EXPECT_TRUE(ObjectDefT<Person>);
  EXPECT_TRUE((isMemberOfObject<FirstName, Person>()));
  EXPECT_TRUE((isMemberOfObject<LastName, Person>()));
  EXPECT_TRUE((isMemberOfObject<MiddleName, Person>()));
  EXPECT_TRUE((isMemberOfObject<PersonAge, Person>()));
}
