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
#pragma once

/**
 * Header file for a library of C++ code that will be wrapped by the
 * reflexive python library
 */

#include <optional>
#include <string>
#include <vector>

namespace jmg
{

/**
 * very simple class that logs to stdout in its constructor and destructor
 */
class TestLifetime {
public:
  TestLifetime();
  ~TestLifetime();
};

/**
 * test class that has various types of member function which exercise
 * python reflexive library functionality
 */
class TestClass {
public:
  // constructors
  TestClass() = default;
  explicit TestClass(const int int_val);
  explicit TestClass(const std::string str_val);

  // destructor
  ~TestClass() = default;

  ////////////////////
  // member functions with no arguments

  // member functions that return constants
  int returnsIntConst() const;
  double returnsDblConst() const;
  std::string returnsStrConst() const;
  void returnsVoid() const;

  // member functions that return data member values
  int returnsIntVal() const;
  std::string returnsStrVal() const;

  ////////////////////
  // member functions with one argument

  int returnsIntArg(const int arg) const;
  std::string returnsStdStringArg(const std::string& str) const;
  std::string returnsStdStringViewArg(std::string_view str) const;

  ////////////////////
  // static member functions with one argument

  static int staticReturnsIntArg(const int arg);
  static std::string staticReturnsStdStringArg(const std::string& str);
  static std::string staticReturnsStdStringViewArg(std::string_view str);

  ////////////////////
  // non-static data members

  int int_data_member = 19991231;
  std::string str_data_member = std::string("BLUB");
  std::string_view str_view_data_member = std::string_view("too far");

  ////////////////////
  // static data members
  static int static_int_data_member;
  static std::string static_str_data_member;
  static std::string_view static_str_view_data_member;

private:
  std::optional<int> int_val_{};
  std::optional<std::string> str_val_{};
};

/**
 * test class that demonstrates python handling of C++ types that
 * present an interface compatible with std::ranges::range
 */
class TestContainer {
  std::vector<int> data_ = {1, 2, 3, 4, 5};

public:
  using value_type = int;

  auto begin() const { return data_.begin(); }
  auto end() const { return data_.end(); }
  auto size() const { return data_.size(); }

  auto operator[](const size_t idx) const { return data_[idx]; }
  auto& operator[](const size_t idx) { return data_[idx]; }
};

} // namespace jmg
