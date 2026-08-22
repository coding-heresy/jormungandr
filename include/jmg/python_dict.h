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

#include <source_location>
#include <string>
#include <string_view>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "jmg/python_object.h"

namespace jmg::python
{

/**
 * C++ wrapper for python dictionary
 *
 * NOTE: this is a view/proxy class
 *
 * TODO(bd) finish filling out the functionality of this class to
 * match the standards set for C++ dictionary-style data structures
 */
class PythonDict {
public:
  /**
   * iterator implementation
   */
  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using key_type = PyObject*;
    using mapped_type = PyObject*;
    using value_type = std::pair<key_type, mapped_type>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    iterator() = default;

    void advance();

    iterator& operator++();

    iterator operator++(int);

    bool operator==(const iterator& o) const;

    bool operator!=(const iterator& o) const;

    reference operator*() const;

  private:
    friend class PythonDict;

    iterator(PyObject* dict, Py_ssize_t idx);

    PyObject* dict_;
    Py_ssize_t idx_ = 0;
    PyObject* key_ = nullptr;
    PyObject* val_ = nullptr;
    mutable value_type current_;
  };

  PythonDict();

  explicit PythonDict(
    PyObject& dict,
    std::source_location src_lctn = std::source_location::current());

  ~PythonDict();

  /**
   * NOTE: shallow copy
   */
  PythonDict(const PythonDict& src);

  /**
   * NOTE: shallow copy
   */
  PythonDict& operator=(const PythonDict& src);

  /**
   * no move, copy is cheap
   */
  JMG_NON_MOVABLE(PythonDict);

  iterator begin() const;

  iterator end() const;

  size_t size() const;

  bool contains(PyObject* key) const;

  bool contains(PythonObject key) const;

  PyObject* at(PyObject* key) const;

  PythonObject at(PythonObject key) const;

  void insert_or_assign(PyObject* key, PyObject* val);

  void insert_or_assign(PythonObject key, PythonObject val);

  iterator find(PyObject* key) const;

  iterator find(PythonObject key) const;

private:
  PyObject* dict_ = nullptr;
};

} // namespace jmg::python
