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

#include "jmg/meta.h"
#include "jmg/safe_types.h"

using namespace jmg;
using namespace std;

JMG_SAFE_ID_32(TestId32);
JMG_SAFE_ID(TestIdStr, string);

TEST(SafeTypesTests, IdsAreComparable) {
  TestId32 val1{42};
  TestId32 val2{42};
  TestId32 val3{20010911};

  EXPECT_EQ(val1, val2);
  EXPECT_NE(val1, val3);
}

TEST(SafeTypesTests, RetrieveUnsafeType) {
  EXPECT_TRUE((is_same_v<uint32_t, UnsafeTypeFromT<TestId32>>));
  EXPECT_TRUE((is_same_v<string, UnsafeTypeFromT<TestIdStr>>));
}

TEST(SafeTypesTests, StreamOutput) {
  TestId32 id{42};
  ostringstream strm;
  strm << id;
  EXPECT_EQ("42"s, strm.str());
}
