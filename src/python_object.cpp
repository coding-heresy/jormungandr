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

#include "jmg/python_object.h"

namespace jmg::python
{

PythonObject::PythonObject() : obj_(Py_None) { Py_INCREF(obj_); }

PythonObject::~PythonObject() {
  if (obj_) { Py_XDECREF(obj_); }
}

PythonObject::PythonObject(const PythonObject& src) : obj_(src.obj_) {
  Py_XINCREF(obj_);
}

PythonObject::PythonObject(PythonObject&& src) : obj_(src.obj_) {
  Py_XINCREF(obj_);
  src.obj_ = nullptr;
}

PythonObject& PythonObject::operator=(const PythonObject& src) {
  if (this != &src) {
    Py_XDECREF(obj_);
    obj_ = src.obj_;
    Py_XINCREF(obj_);
  }
  return *this;
}

PythonObject& PythonObject::operator=(PythonObject&& src) {
  if (this != &src) {
    Py_XDECREF(obj_);
    obj_ = src.obj_;
    src.obj_ = nullptr;
  }
  return *this;
}

PyObject* PythonObject::operator*() const { return obj_; }

PyObject* PythonObject::release() && {
  auto* rslt = obj_;
  obj_ = nullptr;
  return rslt;
}

bool PythonObject::operator==(PythonObject rhs) const {
  return equal(obj_, rhs.obj_);
}

bool PythonObject::operator!=(PythonObject rhs) const {
  return !(*this == rhs);
}

bool PythonObject::operator==(PyObject* rhs) const { return equal(obj_, rhs); }

bool PythonObject::operator!=(PyObject* rhs) const { return !(*this == rhs); }

} // namespace jmg::python
