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

#include "jmg/cmdline.h"

using namespace jmg;
using namespace std;

using PosnParam1 = PosnParam<int, "int", "an integer positional param">;
using PosnParam2 = PosnParam<string, "str", "a string positional param">;
using OptPosnParam =
  PosnParam<string, "opt_str", "an optional string positional param", Optional>;
using NamedParam1 = NamedParam<double, "dbl", "a double named param", Required>;
using NamedParam2 = NamedParam<bool, "flag", "a flag", Required>;
using NamedParam3 =
  NamedParam<unsigned, "opt_int", "an optional integer named parameter", Optional>;

TEST(CmdLineParamTests, TestConcepts) {
  // CmdLineParamT
  EXPECT_FALSE(CmdLineParamT<int>);
  EXPECT_TRUE(CmdLineParamT<PosnParam1>);
  EXPECT_TRUE(CmdLineParamT<PosnParam2>);
  EXPECT_TRUE(CmdLineParamT<NamedParam1>);
  EXPECT_TRUE(CmdLineParamT<NamedParam2>);

  // PosnParamT
  EXPECT_FALSE(PosnParamT<string>);
  EXPECT_TRUE(PosnParamT<PosnParam1>);
  EXPECT_TRUE(PosnParamT<PosnParam2>);
  EXPECT_FALSE(PosnParamT<NamedParam1>);
  EXPECT_FALSE(PosnParamT<NamedParam2>);

  // NamedParamT
  EXPECT_FALSE(NamedParamT<double>);
  EXPECT_FALSE(NamedParamT<PosnParam1>);
  EXPECT_FALSE(NamedParamT<PosnParam2>);
  EXPECT_TRUE(NamedParamT<NamedParam1>);
  EXPECT_TRUE(NamedParamT<NamedParam2>);

  // command line params can be used to build an object
  using TmpObj = ObjectDef<PosnParam1, PosnParam2, NamedParam1, NamedParam2>;
  EXPECT_TRUE(ObjectDefT<TmpObj>);
}

TEST(CmdLineParamTests, TestUsage) {
  const char* argv[] = {"test_program"};
  // NOTE: the declaration of CmdLine is very tightly coupled with the
  // string used for comparison inside the catch statement
  using CmdLine =
    CmdLineArgs<NamedParam1, NamedParam2, PosnParam1, PosnParam2, OptPosnParam>;
  try {
    const auto cmdline = CmdLine(1, argv);
  }
  catch (const exception& exc) {
    const auto posn = std::string_view(exc.what())
                        .find("usage: test_program -dbl <double> "
                              "-flag <int (int)> <str (std::string)> "
                              "[opt_str (std::string)]");
    EXPECT_NE(string::npos, posn);
    return;
  }
  // should never reach here
  EXPECT_TRUE(false);
}

TEST(CmdLineParamTests, TestTrivialCommandLine) {
  const char* argv[] = {"test_program"};
  using CmdLine = CmdLineArgs<>;
  const auto cmdline = CmdLine(1, argv);
}

TEST(CmdLineParamTests, TestNamedFlag) {
  const char* argv[] = {"test_program", "-flag"};
  using CmdLine = CmdLineArgs<NamedParam2>;
  const auto cmdline = CmdLine(2, argv);
  const auto flag_value = get<NamedParam2>(cmdline);
  EXPECT_TRUE(flag_value);
}

TEST(CmdLineParamTests, TestRequiredNamedValue) {
  const char* argv[] = {"test_program", "-dbl", "42"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  const auto cmdline = CmdLine(3, argv);
  const auto dbl = get<NamedParam1>(cmdline);
  EXPECT_DOUBLE_EQ(42.0, dbl);
}

TEST(CmdLineParamTests, TestRequiredNamedParameterWithMissingValue) {
  const char* argv[] = {"test_program", "-dbl"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  auto cmdline = std::unique_ptr<CmdLine>();
  EXPECT_ANY_THROW((cmdline = std::make_unique<CmdLine>(2, argv)));
}

TEST(CmdLineParamTests, TestMissingRequiredNamedValue) {
  const char* argv[] = {"test_program"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  auto cmdline = unique_ptr<CmdLine>();
  EXPECT_ANY_THROW((cmdline = std::make_unique<CmdLine>(1, argv)));
}

TEST(CmdLineParamTests, TestOptionalNamedValue) {
  const char* argv[] = {"test_program", "-opt_int", "20010911"};
  using CmdLine = CmdLineArgs<NamedParam3>;
  const auto cmdline = CmdLine(3, argv);
  const auto opt_int = try_get<NamedParam3>(cmdline);
  EXPECT_TRUE(opt_int.has_value());
  EXPECT_EQ(20010911, *opt_int);
}

TEST(CmdLineParamTests, TestMissingOptionalNamedValue) {
  const char* argv[] = {"test_program"};
  using CmdLine = CmdLineArgs<NamedParam3>;
  const auto cmdline = CmdLine(1, argv);
  const auto opt_int = try_get<NamedParam3>(cmdline);
  EXPECT_FALSE(opt_int.has_value());
}

TEST(CmdLineParamTests, TestRequiredIntPositionalValue) {
  const char* argv[] = {"test_program", "-1"};
  using CmdLine = CmdLineArgs<PosnParam1>;
  const auto cmdline = CmdLine(2, argv);
  const auto posn_int = get<PosnParam1>(cmdline);
  EXPECT_EQ(-1, posn_int);
}

TEST(CmdLineParamTests, TestRequiredStrPositionalValue) {
  const char* argv[] = {"test_program", "foo"};
  using CmdLine = CmdLineArgs<PosnParam2>;
  const auto cmdline = CmdLine(2, argv);
  const auto str = get<PosnParam2>(cmdline);
  EXPECT_EQ("foo"s, str);
}

TEST(CmdLineParamTests, TestOptionalStrPositionalValue) {
  const char* argv[] = {"test_program", "foo"};
  using CmdLine = CmdLineArgs<OptPosnParam>;
  const auto cmdline = CmdLine(2, argv);
  const auto str = try_get<OptPosnParam>(cmdline);
  EXPECT_TRUE(str.has_value());
  EXPECT_EQ("foo"s, *str);
}

TEST(CmdLineParamTests, TestMissingOptionalStrPositionalValue) {
  const char* argv[] = {"test_program"};
  using CmdLine = CmdLineArgs<OptPosnParam>;
  const auto cmdline = CmdLine(1, argv);
  const auto str = try_get<OptPosnParam>(cmdline);
  EXPECT_FALSE(str.has_value());
}

TEST(CmdLineParamTests, TestMissingRequiredPositionalValue) {
  const char* argv[] = {"test_program"};
  using CmdLine = CmdLineArgs<PosnParam1>;
  auto cmdline = unique_ptr<CmdLine>();
  EXPECT_ANY_THROW((cmdline = std::make_unique<CmdLine>(1, argv)));
}
