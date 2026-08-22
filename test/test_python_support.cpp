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

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <gtest/gtest.h>

#include "jmg/preprocessor.h"
#include "jmg/python_dict.h"
#include "jmg/python_object.h"
#include "jmg/python_util.h"
#include "jmg/test_util.h"

using namespace jmg;
using namespace jmg::python;
using namespace std;

class PythonSupportTests : public ::testing::Test {
protected:
  /**
   * initialize embedded CPython interpreter environment
   */
  static void SetUpTestSuite() {
    auto& cfg = python_cfg();
    const auto py_status = Py_InitializeFromConfig(&cfg);
    PyConfig_Clear(&cfg);
    JMG_ENFORCE(!PyStatus_Exception(py_status),
                "embedded python initialization failed: ", py_status.err_msg);
  }

  static void TearDownTestSuite() {
    if (Py_IsInitialized()) { Py_FinalizeEx(); }
  }
};

TEST_F(PythonSupportTests, TestPythonObjectCreationAndComparison) {
  const auto int_val1 = PythonObject(69);
  const auto int_val2 = PythonObject(20010911U);
  const auto int_val3 = PythonObject(-1);
  EXPECT_NE(int_val1, int_val2);
  EXPECT_NE(int_val1, int_val3);
  EXPECT_NE(int_val2, int_val3);
  EXPECT_EQ(int_val1, int_val1);
  EXPECT_EQ(int_val2, int_val2);
  EXPECT_EQ(int_val3, int_val3);

  const auto str_val1 = PythonObject("foo"sv);
  const auto str_val2 = PythonObject("bar"sv);
  EXPECT_NE(str_val1, str_val2);
  EXPECT_EQ(str_val1, str_val1);
  EXPECT_EQ(str_val2, str_val2);

  const auto flt_val1 = PythonObject(42.0);
  const auto flt_val2 = PythonObject(3.14159);
  EXPECT_NE(flt_val1, flt_val2);
  EXPECT_EQ(flt_val1, flt_val1);
  EXPECT_EQ(flt_val2, flt_val2);

  EXPECT_NE(int_val1, str_val1);
  EXPECT_NE(int_val1, flt_val1);
  EXPECT_NE(str_val1, flt_val1);

  const PythonObject none_val{};
  EXPECT_EQ(none_val, none_val);
  EXPECT_NE(none_val, int_val1);
  EXPECT_NE(none_val, str_val1);
  EXPECT_NE(none_val, flt_val1);
}

TEST_F(PythonSupportTests, TestPythonDictManipulation) {
  const auto key1 = PythonObject("foo"sv);
  const auto key2 = PythonObject("bar"sv);
  const auto val1 = PythonObject(69);
  const auto val2 = PythonObject(20010911);
  const auto missing_key = PythonObject("baz"sv);

  {
    // tests of raw python objects
    auto dict = PythonDict();
    EXPECT_EQ(0ULL, dict.size());
    dict.insert_or_assign(*key1, *val1);
    EXPECT_EQ(1ULL, dict.size());
    dict.insert_or_assign(*key2, *val2);
    EXPECT_EQ(2ULL, dict.size());

    EXPECT_TRUE(dict.contains(*key1));
    EXPECT_TRUE(dict.contains(*key2));

    EXPECT_FALSE(dict.contains(*missing_key));
  }

  {
    // tests of wrapped python objects
    auto dict = PythonDict();
    dict.insert_or_assign(key1, val1);
    dict.insert_or_assign(key2, val2);

    EXPECT_TRUE(dict.contains(key1));
    EXPECT_TRUE(dict.contains(key2));

    EXPECT_FALSE(dict.contains(missing_key));
  }
}
