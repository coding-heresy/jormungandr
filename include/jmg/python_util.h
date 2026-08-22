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

#include <concepts>
#include <utility>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "jmg/preprocessor.h"

namespace jmg::python
{

////////////////////
// exception types

/**
 * class that C++ wrapper code should throw as an exception in cases
 * where a python error message has already been set
 */
struct RuntimePythonErrorNoCppMsg {};

/**
 * std::runtime_error-derived class that C++ wrapper code should throw
 * in cases where a python type error (which does not map cleanly to
 * any existing C++ standard library exception type) should be
 * returned
 */
JMG_DEFINE_RUNTIME_EXCEPTION(RuntimePythonTypeError);

/**
 * enforcement macro used when a call to a CPython API function has
 * already set the python exception type and message
 *
 * TODO(bd) use PyErr_Occurred everywhere
 */
#define JMG_ENFORCE_PYTHON_SUCCESS(pred)                 \
  do {                                                   \
    if (!(pred)) { throw RuntimePythonErrorNoCppMsg(); } \
  } while (0)

////////////////////
// utility constants and types

constexpr int kPyErr = -1;
constexpr int kPySuccess = 0;

////////////////////
// utility operators and functions

[[nodiscard]] inline bool equal(PyObject* lhs, PyObject* rhs) {
  if (lhs != rhs) {
    if (!lhs || !rhs) { return false; }
    const auto rc = PyObject_RichCompareBool(lhs, rhs, Py_EQ);
    JMG_ENFORCE_PYTHON_SUCCESS(kPyErr != rc);
    return 1 == rc;
  }
  return true;
}

} // namespace jmg::python
