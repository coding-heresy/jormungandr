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

#include <string>
#include <tuple>

#include "jmg/util.h"

using namespace jmg;
using namespace std;
using namespace std::string_literals;

TEST(GeneralUtilitiesTest, TestStreamTupleOut) {
  const auto tp = make_tuple(20010911, 42.0, "foo"s);
  ostringstream strm;
  strm << tp;
  EXPECT_EQ("20010911, 42, foo"s, strm.str());
}

TEST(GeneralUtilitiesTest, TestStreamOctetOut) {
  auto value = uint8_t(1);
  std::array<char, 9> expected;
  expected.fill('0');
  expected[8] = '\0';
  for (size_t counter = 0; counter < 8; ++counter) {
    if (counter > 0) { expected[7 - (counter - 1)] = '0'; }
    expected[7 - counter] = '1';
    ostringstream strm;
    strm << Octet(value);
    EXPECT_EQ(strm.str(), string(expected.data()));
    value <<= 1;
  }
}

TEST(GeneralUtilitiesTest, TestGetFromArgs) {
  const auto int_val = 20010911;
  const auto dbl_val = 42.0;
  const auto str_val = "foo"s;

  // order of arguments doesn't matter
  EXPECT_EQ(20010911, getFromArgs<int>(int_val, dbl_val, str_val));
  EXPECT_EQ(20010911, getFromArgs<int>(dbl_val, str_val, int_val));
  EXPECT_EQ(20010911, getFromArgs<int>(str_val, int_val, dbl_val));
}
