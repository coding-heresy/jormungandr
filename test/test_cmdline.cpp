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
#include "jmg/safe_types.h"

using namespace jmg;
using namespace std;

using PosnParam1 = PosnParam<int, "int", "an integer positional param">;
using PosnParam2 = PosnParam<string, "str", "a string positional param">;
using OptPosnParam =
  PosnParam<string, "opt_str", "an optional string positional param", Optional>;
using NamedParam1 = NamedParam<double, "dbl", "a double named param", Required>;
using NamedParam2 = NamedFlag<"flag", "a flag">;
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

  using CmdLine = CmdLineArgs<PosnParam1>;
  EXPECT_TRUE(CmdLineArgsT<CmdLine>);
  EXPECT_FALSE(CmdLineArgsT<TmpObj>);
}

TEST(CmdLineParamTests, TestUsage) {
  std::array argv{"test_program"};
  // NOTE: the declaration of CmdLine is very tightly coupled with the
  // string used for comparison inside the catch statement
  using CmdLine =
    CmdLineArgs<NamedParam1, NamedParam2, PosnParam1, PosnParam2, OptPosnParam>;
  try {
    const auto cmdline = CmdLine(argv.size(), argv.data());
    JMG_TEST_UNREACHED;
  }
  catch (const exception& exc) {
    const auto posn = std::string_view(exc.what())
                        .find("usage: test_program -dbl <double> "
                              "-flag <int (int)> <str (std::string)> "
                              "[opt_str (std::string)]");
    EXPECT_NE(string::npos, posn);
    return;
  }
  JMG_TEST_UNREACHED;
}

TEST(CmdLineParamTests, TestTrivialCommandLine) {
  std::array argv{"test_program"};
  using CmdLine = CmdLineArgs<>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
}

////////////////////////////////////////////////////////////////////////////////
// Handling of command lines that match the specified parameters
////////////////////////////////////////////////////////////////////////////////

TEST(CmdLineParamTests, TestNamedFlagSet) {
  std::array argv{"test_program", "-flag"};
  using CmdLine = CmdLineArgs<NamedParam2>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
  const auto flag_value = get<NamedParam2>(cmdline);
  EXPECT_TRUE(flag_value);
}

TEST(CmdLineParamTests, TestNamedFlagNotSet) {
  std::array argv{"test_program"};
  using CmdLine = CmdLineArgs<NamedParam2>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
  const auto flag_value = get<NamedParam2>(cmdline);
  EXPECT_FALSE(flag_value);
}

TEST(CmdLineParamTests, TestRequiredNamedValue) {
  std::array argv{"test_program", "-dbl", "42"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
  const auto dbl = get<NamedParam1>(cmdline);
  EXPECT_DOUBLE_EQ(42.0, dbl);
}

TEST(CmdLineParamTests, TestOptionalNamedValue) {
  std::array argv{"test_program", "-opt_int", "20010911"};
  using CmdLine = CmdLineArgs<NamedParam3>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
  const auto opt_int = try_get<NamedParam3>(cmdline);
  EXPECT_TRUE(opt_int.has_value());
  EXPECT_EQ(20010911, *opt_int);
}

TEST(CmdLineParamTests, TestMissingOptionalNamedValue) {
  std::array argv{"test_program"};
  using CmdLine = CmdLineArgs<NamedParam3>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
  const auto opt_int = try_get<NamedParam3>(cmdline);
  EXPECT_FALSE(opt_int.has_value());
}

TEST(CmdLineParamTests, TestRequiredIntPositionalValue) {
  std::array argv{"test_program", "-1"};
  using CmdLine = CmdLineArgs<PosnParam1>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
  const auto posn_int = get<PosnParam1>(cmdline);
  EXPECT_EQ(-1, posn_int);
}

TEST(CmdLineParamTests, TestRequiredStrPositionalValue) {
  std::array argv{"test_program", "foo"};
  using CmdLine = CmdLineArgs<PosnParam2>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
  const auto str = get<PosnParam2>(cmdline);
  EXPECT_EQ("foo"s, str);
}

TEST(CmdLineParamTests, TestOptionalStrPositionalValue) {
  std::array argv{"test_program", "foo"};
  using CmdLine = CmdLineArgs<OptPosnParam>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
  const auto str = try_get<OptPosnParam>(cmdline);
  EXPECT_TRUE(str.has_value());
  EXPECT_EQ("foo"s, *str);
}

TEST(CmdLineParamTests, TestMissingOptionalStrPositionalValue) {
  std::array argv{"test_program"};
  using CmdLine = CmdLineArgs<OptPosnParam>;
  const auto cmdline = CmdLine(argv.size(), argv.data());
  const auto str = try_get<OptPosnParam>(cmdline);
  EXPECT_FALSE(str.has_value());
}

////////////////////////////////////////////////////////////////////////////////
// Handling of command lines that do not match the specified parameters
////////////////////////////////////////////////////////////////////////////////

TEST(CmdLineParamTests, TestFailedConversionOfRequiredNamedParameterValue) {
  std::array argv{"test_program", "-dbl", "foo"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  EXPECT_THROW((CmdLine(argv.size(), argv.data())), runtime_error);
}

TEST(CmdLineParamTests, TestFailedConversionOfOptionalNamedParameterValue) {
  std::array argv{"test_program", "-opt_int", "bar"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  EXPECT_THROW((CmdLine(argv.size(), argv.data())), runtime_error);
}

// NOTE: it would normally not be appropriate to include the contents
// of the error message associated with an exception in a test but in
// this case the messages are important because they guide the user of
// a command line program to the correct arguments

#define EXPECT_CMDLINE_ERROR(cmd, errMsgMatch)                       \
  do {                                                               \
    try {                                                            \
      const auto cmdline = (cmd);                                    \
      JMG_TEST_UNREACHED;                                            \
    }                                                                \
    catch (const CmdLineError& exc) {                                \
      EXPECT_NE(string::npos, string(exc.what()).find(errMsgMatch)); \
      return;                                                        \
    }                                                                \
    catch (...) {                                                    \
      JMG_TEST_UNREACHED;                                            \
    }                                                                \
    JMG_TEST_UNREACHED;                                              \
  } while (0)

TEST(CmdLineParamTests, TestMissingRequiredNamedValue) {
  std::array argv{"test_program"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())),
                       "unable to find required named argument [dbl]");
}

TEST(CmdLineParamTests, TestRequiredNamedParameterWithMissingValue) {
  std::array argv{"test_program", "-dbl"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  const auto errFragment = "named argument [dbl] is the last argument and is "
                           "missing its required value"s;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())), errFragment);
}

TEST(CmdLineParamTests, TestMultipleMatchesForRequiredNamedParameter) {
  std::array argv{"test_program", "-dbl", "42", "-dbl", "24"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  const auto errFragment = "multiple matches for named argument [dbl]"s;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())), errFragment);
}

TEST(CmdLineParamTests, TestUnusualMultipleMatchesForRequiredNamedParameter) {
  std::array argv{"test_program", "-dbl", "-dbl", "24"};
  using CmdLine = CmdLineArgs<NamedParam1>;
  const auto errFragment = "multiple matches for named argument [dbl]"s;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())), errFragment);
}

TEST(CmdLineParamTests, TestMultipleMatchesForOptionalNamedParameter) {
  std::array argv{"test_program", "-opt_int", "20010911", "-opt_int",
                  "20010911"};
  using CmdLine = CmdLineArgs<NamedParam3>;
  const auto errFragment = "multiple matches for named argument [opt_int]"s;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())), errFragment);
}

TEST(CmdLineParamTests, TestUnusualMultipleMatchesForOptionalNamedParameter) {
  std::array argv{"test_program", "-opt_int", "-opt_int", "20010911"};
  using CmdLine = CmdLineArgs<NamedParam3>;
  const auto errFragment = "multiple matches for named argument [opt_int]"s;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())), errFragment);
}

TEST(CmdLineParamTests, TestMissingRequiredPositionalValue) {
  std::array argv{"test_program"};
  using CmdLine = CmdLineArgs<PosnParam1>;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())),
                       "unable to find required positional argument [int]");
}

TEST(CmdLineParamTests,
     TestMissingRequiredPositionalValueAfterRequiredNamedValue) {
  std::array argv{"test_program", "-dbl", "42"};
  using CmdLine = CmdLineArgs<NamedParam1, PosnParam1>;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())),
                       "unable to find required positional argument [int]");
}

TEST(CmdLineParamTests,
     TestMissingRequiredPositionalValueAfterOptionalNamedValue) {
  std::array argv{"test_program", "-opt_int", "20010911"};
  using CmdLine = CmdLineArgs<NamedParam3, PosnParam1>;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())),
                       "unable to find required positional argument [int]");
}

TEST(CmdLineParamTests, TestExtraneousArgument) {
  std::array argv{"test_program", "foo", "bar"};
  using CmdLine = CmdLineArgs<PosnParam2>;
  EXPECT_CMDLINE_ERROR((CmdLine(argv.size(), argv.data())),
                       "command line argument [bar] did not match any declared "
                       "parameter");
}

TEST(CmdLineParamTests, TestOptionalsWithDefault) {
  using CmdLine = CmdLineArgs<PosnParam2, OptPosnParam>;
  {
    std::array argv{"test_program", "foo", "bar"};
    const auto cmdline = CmdLine(argv.size(), argv.data());
    EXPECT_EQ("bar"s, get<OptPosnParam>(cmdline, "blub"s));
  }
  {
    std::array argv{"test_program", "foo"};
    const auto cmdline = CmdLine(argv.size(), argv.data());
    EXPECT_EQ("blub"s, get<OptPosnParam>(cmdline, "blub"s));
  }
}

TEST(CmdLineParamTests, TestSafeTypes) {
  using TestIdStr = SafeIdStr<>;
  using TestId32 = SafeId32<>;

  using SafeName =
    NamedParam<TestIdStr, "str_id", "a safe string ID named param", Required>;
  using SafePosn =
    PosnParam<TestId32, "int_id", "a safe 32 bit integer ID positional param">;

  using CmdLine = CmdLineArgs<SafeName, SafePosn>;

  std::array argv{"test_program", "-str_id", "foo", "20010911"};
  const auto cmdline = CmdLine(argv.size(), argv.data());
  EXPECT_EQ(TestIdStr("foo"), jmg::get<SafeName>(cmdline));
  EXPECT_EQ(TestId32(20010911), jmg::get<SafePosn>(cmdline));
}

#undef EXPECT_CMDLINE_ERROR

// TODO(bd) initial cut of a test case that should not compile
// correctly because CmdLineArgs has been specialized with more than
// one field that has the same field name
//
// TEST(CmdLineParamTests, TestHandlingDuplicateParameterFieldNames) {
//   using DupPosnParam1 = PosnParam<int, "dup_name", "a parameter whose name
//   will be duplicated">; using DupPosnParam2 = PosnParam<double, "dup_name",
//   "a parameter whose name is a duplicate">; using CmdLine =
//   CmdLineArgs<DupPosnParam1, DupPosnParam2>; std::array argv{"test_program",
//   "20010911", "42.0"}; const auto cmdline = CmdLine(argv.size(),
//   argv.data());
// }

// TODO(bd) initial cut of a test case that should not compile
// correctly because an optional positional parameter is provided
// before a required positional parameter when CmdLineArgs is
// specialized
//
// TEST(CmdLineParamTests,
//      TestCompileFailsIfOptionalPositionalParamProvidedBeforeRequired) {
//   using OptPosnFlt =
//     PosnParam<float, "opt_posn_float",
//               "the second position param, which is optional", Optional>;
//   using PosnDbl = PosnParam<double, "posn_double",
//                             "the first position param, which is required">;
//   using CmdLine = CmdLineArgs<OptPosnFlt, PosnDbl>;
//   std::array argv{"test_program", "42.0"};
//   const auto cmdline = CmdLine(argv.size(), argv.data());
// }
