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

#include "test_features.h"

#include <iostream>

#include "jmg/preprocessor.h"
#include "jmg/util.h"

using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace jmg::python
{

////////////////////
// TestLifetime class

TestLifetime::TestLifetime() { cout << "constructor called" << endl; }

TestLifetime::~TestLifetime() { cout << "destructor called" << endl; }

////////////////////
// TestClass class

TestClass::TestClass(const int int_val) : int_val_(int_val) {}

TestClass::TestClass(const string str_val) : str_val_(str_val) {}

int TestClass::returnsIntConst() const { return 20010911; }

double TestClass::returnsDblConst() const { return 42.0; }

string TestClass::returnsStrConst() const { return "foo"s; }

void TestClass::returnsVoid() const { cout << __PRETTY_FUNCTION__ << endl; }

int TestClass::returnsIntVal() const {
  JMG_ENFORCE(int_val_, "trying to return int_val that was not set");
  return *int_val_;
}

string TestClass::returnsStrVal() const {
  JMG_ENFORCE(str_val_, "trying to return str_val that was not set");
  return *str_val_;
}

int TestClass::returnsIntArg(const int arg) const { return arg; }

string TestClass::returnsStdStringArg(const string& str) const { return str; }

string TestClass::returnsStdStringViewArg(string_view str) const {
  return string(str);
}

int TestClass::addAndSet(const int arg1, const int arg2) {
  auto val = arg1 + arg2;
  int_val_ = arg1;
  return int_val_ ? *int_val_ + val : val;
}

int TestClass::addAndSetWithDefault(const int arg1, const int arg2) {
  auto val = arg1 + arg2;
  int_val_ = arg1;
  return int_val_ ? *int_val_ + val : val;
}

int TestClass::staticReturnsIntArg(const int arg) { return arg; }

string TestClass::staticReturnsStdStringArg(const string& str) { return str; }

string TestClass::staticReturnsStdStringViewArg(string_view str) {
  return string(str);
}

string TestClass::staticReturnsCatArgs(std::string_view str_arg, int int_arg) {
  return str_cat(str_arg, int_arg);
}

int TestClass::static_int_data_member = 20000101;
string TestClass::static_str_data_member = "blah"s;
string_view TestClass::static_str_view_data_member = "not far enough"sv;

} // namespace jmg::python
