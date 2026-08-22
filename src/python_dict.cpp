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

#include <string_view>
#include <tuple>

#include "jmg/python_dict.h"

#include "jmg/preprocessor.h"
#include "jmg/python_util.h"
#include "jmg/util.h"

using namespace std;
using namespace std::string_view_literals;

namespace jmg::python
{

PythonDict::PythonDict() : dict_(PyDict_New()) {
  JMG_ENFORCE(dict_, "unable to create new python dictionary");
}

PythonDict::PythonDict(PyObject& dict, const source_location src_lctn)
  : dict_(&dict) {
  JMG_ENFORCE_AT_SRC_USING(RuntimePythonTypeError, PyDict_Check(dict_),
                           src_lctn, "python object was not a dict"sv);
  Py_XINCREF(dict_);
}

PythonDict::~PythonDict() { Py_XDECREF(dict_); }

PythonDict::PythonDict(const PythonDict& src) {
  dict_ = src.dict_;
  Py_XINCREF(dict_);
}

PythonDict& PythonDict::operator=(const PythonDict& src) {
  if (&src != this) {
    Py_XDECREF(dict_);
    dict_ = src.dict_;
    Py_XINCREF(dict_);
  }
  return *this;
}

PythonDict::iterator::iterator(PyObject* dict, Py_ssize_t idx)
  : dict_(dict), idx_(idx) {
  JMG_ENFORCE_USING(RuntimePythonTypeError, dict,
                    "attempted to create a python dictionary iterator from a "
                    "nullptr dictionary");
  advance();
}

void PythonDict::iterator::advance() {
  if (!PyDict_Next(dict_, &idx_, &key_, &val_)) {
    key_ = nullptr;
    val_ = nullptr;
  }
}

PythonDict::iterator& PythonDict::iterator::operator++() {
  advance();
  return *this;
}

PythonDict::iterator PythonDict::iterator::operator++(int) {
  iterator rslt = *this;
  advance();
  return rslt;
}

bool PythonDict::iterator::operator==(const iterator& rhs) const {
  return equal(key_, rhs.key_) && (idx_ == rhs.idx_);
}

bool PythonDict::iterator::operator!=(const iterator& rhs) const {
  return !(*this == rhs);
}

PythonDict::iterator::reference PythonDict::iterator::operator*() const {
  current_ = make_pair(key_, val_);
  return current_;
}

PythonDict::iterator PythonDict::begin() const { return iterator(dict_, 0); }

PythonDict::iterator PythonDict::end() const {
  return iterator(dict_, numeric_limits<Py_ssize_t>::max());
}

size_t PythonDict::size() const {
  const auto rc = PyDict_Size(dict_);
  JMG_ENFORCE_PYTHON_SUCCESS(rc >= 0);
  return static_cast<size_t>(rc);
}

bool PythonDict::contains(PyObject* key) const {
  const auto rc = PyDict_Contains(dict_, key);
  JMG_ENFORCE_PYTHON_SUCCESS(kPyErr != rc);
  return 1 == rc;
}

bool PythonDict::contains(const PythonObject key) const {
  return contains(*key);
}

PyObject* PythonDict::at(PyObject* key) const {
  auto* rslt = PyDict_GetItemWithError(dict_, key);
  if (!rslt) { JMG_ENFORCE_PYTHON_SUCCESS(!PyErr_Occurred()); }
  return rslt;
}

PythonObject PythonDict::at(const PythonObject key) const {
  return PythonObject(at(*key));
}

void PythonDict::insert_or_assign(PyObject* key, PyObject* val) {
  PyDict_SetItem(dict_, key, val);
  JMG_ENFORCE_PYTHON_SUCCESS(!PyErr_Occurred());
}

void PythonDict::insert_or_assign(PythonObject key, PythonObject val) {
  insert_or_assign(*key, *val);
}

PythonDict::iterator PythonDict::find(PyObject* key) const {
  // NOTE: there is apparently no single function to e.g. find the
  // index of an item in a python dict, so iterating over all the
  // items is the best that can be done
  for (auto itr = begin(); itr != end(); ++itr) {
    if (equal(key_of(*itr), key)) { return itr; }
  }
  return end();
}

PythonDict::iterator PythonDict::find(const PythonObject key) const {
  return find(*key);
}

} // namespace jmg::python
