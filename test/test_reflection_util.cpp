/** -*- mode: c++ -*-
 *
 * Copyright (C) 2026 Brian Davis
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

#include "jmg/reflection_util.h"

#include <string>
#include <string_view>
#include <tuple>

#include <gtest/gtest.h>

#include "jmg/meta.h"

using namespace jmg;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace
{

void fcnName(int fcnArg) { ignore = fcnArg; }

void fcn_name(int fcn_arg, std::string str_arg) {
  ignore = fcn_arg;
  ignore = str_arg;
}

class StandardName {};

class non_standard_name {};

class Really_Bad_Name {};

// TODO(bd) should cases involving identifiers like this work?
// class NO_THANK_YOU {};

} // namespace

#define JMG_CHECK_ID(style, src, tgt) \
  EXPECT_EQ(string_view(style##CaseIdOwner<^^src>::c_str()), string_view(tgt))

TEST(ReflectionUtilTests, TestSnakeCase) {
  JMG_CHECK_ID(Snake, fcnName, "fcn_name");
  JMG_CHECK_ID(Snake, fcn_name, "fcn_name");
  JMG_CHECK_ID(Snake, StandardName, "standard_name");
  JMG_CHECK_ID(Snake, non_standard_name, "non_standard_name");
  JMG_CHECK_ID(Snake, Really_Bad_Name, "really_bad_name");
}

TEST(ReflectionUtilTests, TestPascalCase) {
  JMG_CHECK_ID(Pascal, fcnName, "FcnName");
  JMG_CHECK_ID(Pascal, fcn_name, "FcnName");
  JMG_CHECK_ID(Pascal, StandardName, "StandardName");
  JMG_CHECK_ID(Pascal, non_standard_name, "NonStandardName");
  JMG_CHECK_ID(Pascal, Really_Bad_Name, "ReallyBadName");
  // TODO(bd) should the following case work?
  // JMG_CHECK_ID(Pascal, NO_THANK_YOU, "NoThankYou"sv);
}

TEST(ReflectionUtilTests, TestCamelCase) {
  JMG_CHECK_ID(Camel, fcnName, "fcnName");
  JMG_CHECK_ID(Camel, fcn_name, "fcnName");
  JMG_CHECK_ID(Camel, StandardName, "standardName");
  JMG_CHECK_ID(Camel, non_standard_name, "nonStandardName");
  JMG_CHECK_ID(Camel, Really_Bad_Name, "reallyBadName");
  // TODO(bd) should the following case work?
  // JMG_CHECK_ID(Camel, NO_THANK_YOU, "noThankYou");
}

TEST(ReflectionUtilTests, TestFcnParamsTuple) {
  EXPECT_TRUE((SameAsDecayedT<tuple<int>, FcnParamsTupleForFcnMetaT<^^fcnName>>));
  EXPECT_TRUE((SameAsDecayedT<tuple<int>, FcnParamsTupleForFcnPtrT<fcnName>>));
  EXPECT_TRUE((SameAsDecayedT<tuple<int, std::string>,
                              FcnParamsTupleForFcnMetaT<^^fcn_name>>));
  EXPECT_TRUE((SameAsDecayedT<tuple<int, std::string>,
                              FcnParamsTupleForFcnPtrT<fcn_name>>));
}

// TODO(bd) the following should not compile:
//   SnakeCaseIdOwner<^^NO_THANK_YOU>
//   PascalCaseIdOwner<^^broken_name_>
//   CamelCaseIdOwner<^^broken_name_>
