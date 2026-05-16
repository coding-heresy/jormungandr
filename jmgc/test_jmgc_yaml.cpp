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

#include "test_jmgc_any.h"

#include <sstream>

#include <yaml-cpp/yaml.h>

#include "jmg/util.h"
#include "jmg/yaml/yaml.h"

/**
 * test code specific to CBE test
 */

using namespace jmg;
using namespace std;
using namespace std::string_literals;

namespace jmg::test_jmgc
{

namespace
{

const auto boolVal = []() -> string {
  ostringstream strm;
  strm << boolalpha << TestValues::kBoolean;
  return strm.str();
}();

const auto kYamlStr = str_cat("boolean: ",
                              boolVal,
                              "\n",
                              // 32 bit integers
                              "int_32: ",
                              TestValues::kInt32,
                              "\n",
                              "uint_32: ",
                              TestValues::kUInt32,
                              "\n",
                              "sfixed_32: ",
                              TestValues::kSFixed32,
                              "\n",
                              "fixed_32: ",
                              TestValues::kFixed32,
                              "\n",
                              // 64 bit integers
                              "int_64: ",
                              TestValues::kInt64,
                              "\n",
                              "uint_64: ",
                              TestValues::kUInt64,
                              "\n",
                              "sfixed_64: ",
                              TestValues::kSFixed64,
                              "\n",
                              "fixed_64: ",
                              TestValues::kFixed64,
                              "\n",
                              // floating point
                              "flt: ",
                              TestValues::kFlt,
                              "\n",
                              "dbl: ",
                              TestValues::kDbl,
                              "\n",
                              // strings
                              "str: \"",
                              TestValues::kStr,
                              "\"\n",
                              "bytes_str: \"",
                              TestValues::kBytesStr,
                              "\"\n",
                              // safe type
                              "int_id_fld: ",
                              unsafe(TestValues::kIntId),
                              "\n",
                              // enum
                              "active_state: ",
                              TestValues::kActiveState,
                              "\n",
                              // int array
                              "ints:\n- ",
                              str_join(TestValues::kInts, "\n- ]"),
                              "\n",
                              // str array
                              "strs:\n- \"",
                              str_join(TestValues::kStrs, "\"\n- \""),
                              "\"\n",
                              // inner message array
                              "inner_msgs:\n",
                              "- inner_int_32: ",
                              str_cat(TestValues::kInnerInts[0]),
                              "\n- inner_int_32: ",
                              str_cat(TestValues::kInnerInts[1]),
                              "\n  opt_inner_str: \"",
                              *(TestValues::kInnerOptStrs[1]),
                              "\"\n");

} // namespace

NonJmgTestMsg makeNonJmgTestMsg(const jmg::TimePoint tp) {
  // add the argument time point to the end of the YAML data
  const auto yaml_str =
    str_cat(kYamlStr, "time_stamp: ", epoch_duration_from(tp).count(), "\n");
  return YAML::Load(yaml_str);
}

} // namespace jmg::test_jmgc
