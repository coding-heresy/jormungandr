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

#include "reflexive.h"

using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace jmg
{

constexpr auto kReflexiveTestDocStr =
  "module for testing the python reflex library"sv;

struct ReflexiveFeatures
  : PythonModule<ReflexiveFeatures, kReflexiveTestDocStr> {};

constexpr auto kTestLifetimeDocStr =
  "class that logs lifetime events for testing with PythonReflex"sv;

/**
 * very simple class that logs to stdout in its constructor and destructor
 */
class TestLifetime
  : public PythonReflex<TestLifetime, ReflexiveFeatures, kTestLifetimeDocStr> {
public:
  TestLifetime() { cout << "constructor called" << endl; }
  ~TestLifetime() { cout << "destructor called" << endl; }
};

constexpr auto kTestClassDocStr =
  "class that exhibits various behaviors for testing with PythonReflex"sv;

class TestClass
  : public PythonReflex<TestClass, ReflexiveFeatures, kTestClassDocStr> {
public:
  // constructors
  TestClass() = default;
  explicit TestClass(const int int_val) : int_val_(int_val) {}
  explicit TestClass(const std::string str_val) : str_val_(str_val) {}

  // destructor
  ~TestClass() = default;

  ////////////////////
  // member functions with no arguments

  // member functions that return constants
  int returnsIntConst() const { return 20010911; }
  double returnsDblConst() const { return 42.0; }
  std::string returnsStrConst() const { return "foo"s; }
  void returnsVoid() const { cout << __PRETTY_FUNCTION__ << endl; }

  // member functions that return data member values
  int returnsIntVal() const {
    JMG_ENFORCE(int_val_, "trying to return int_val that was not set");
    return *int_val_;
  }
  std::string returnsStrVal() const {
    JMG_ENFORCE(str_val_, "trying to return str_val that was not set");
    return *str_val_;
  }

  ////////////////////
  // member functions with one argument

  int returnsIntArg(const int arg) const { return arg; }

private:
  std::optional<int> int_val_{};
  std::optional<std::string> str_val_{};
};

} // namespace jmg

JMG_DECLARE_MODULE(reflexive_features)
