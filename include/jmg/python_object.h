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

#include <string>
#include <string_view>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <boost/numeric/conversion/cast.hpp>

#include "jmg/meta.h"
#include "jmg/preprocessor.h"
#include "jmg/python_util.h"

namespace jmg::python
{

////////////////////
// useful macros

// TODO(bd) only support python 3.14 and above?
#if (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 12) || (PY_MAJOR_VERSION > 3)
// NOTE: use PyErr_GetRaisedException for python version >= 3.12
#define MAYBE_HANDLE_PYTHON_ERROR(...)                                      \
  do {                                                                      \
    if (PyErr_Occurred()) {                                                 \
      PyObject* py_exception = PyErr_GetRaisedException();                  \
      JMG_ENFORCE(py_exception,                                             \
                  "a python error occurred but the raised exception "       \
                  "object could not be retrieved");                         \
      const auto exc_cleanup = Cleanup([&]() { Py_DECREF(py_exception); }); \
      PyObject* py_err_msg = PyObject_Str(py_exception);                    \
      JMG_ENFORCE(py_err_msg,                                               \
                  "unable to get string value for python exception");       \
      const auto msg_cleanup = Cleanup([&]() { Py_DECREF(py_err_msg); });   \
      const char* err_msg = PyUnicode_AsUTF8(py_err_msg);                   \
      JMG_ENFORCE(err_msg,                                                  \
                  "unable to convert python string to C++ string when "     \
                  "handling python exception");                             \
      JMG_RUNTIME_ERROR("python exception [", std::string_view(err_msg),    \
                        "] occurred when ", __VA_ARGS__);                   \
    }                                                                       \
  } while (0)
#else
// NOTE: use PyErr_Fetch for python version < 3.12
#define MAYBE_HANDLE_PYTHON_ERROR(...)                                         \
  do {                                                                         \
    if (PyErr_Occurred()) {                                                    \
      PyObject* py_type = nullptr;                                             \
      PyObject* py_val = nullptr;                                              \
      PyObject* py_traceback = nullptr;                                        \
      PyErr_Fetch(&py_type, &py_val, &py_traceback);                           \
      const auto err_cleanup = Cleanup([&]() {                                 \
        Py_XDECREF(py_type);                                                   \
        Py_XDECREF(py_val);                                                    \
        Py_XDECREF(py_traceback);                                              \
      });                                                                      \
      JMG_ENFORCE(py_val, "a python error occurred but could not be fetcted"); \
      auto* py_err_msg = PyObject_Str(py_val);                                 \
      JMG_ENFORCE(py_err_msg,                                                  \
                  "unable to get string value for python exception");          \
      const auto msg_cleanup = Cleanup([&]() { Py_DECREF(py_err_msg); });      \
      const char* err_msg = PyUnicode_AsUTF8(py_err_msg);                      \
      JMG_ENFORCE(err_msg, "unable to get string value for python exception"); \
      JMG_THROW_EXCEPTION(std::runtime_error, "python exception [",            \
                          std::string_view(err_msg), "] occurred when ",       \
                          __VA_ARGS__);                                        \
    }                                                                          \
  } while (0)
#endif

/**
 * C++ wrapper for a python object
 */
class PythonObject {
  static constexpr int64_t kFailInt = -1LL;

public:
  ~PythonObject();

  PythonObject(const PythonObject& src);
  PythonObject(PythonObject&& src) = default;
  PythonObject& operator=(const PythonObject& src);
  PythonObject& operator=(PythonObject&& src);

  /**
   * create a python object from a C++ object or store a python object pointer
   * and take an ownership share in it
   */
  template<typename T>
  explicit PythonObject(
    T&& src,
    const std::source_location location = std::source_location::current()) {
    if constexpr (SameAsDecayedT<PyObject*, T>) {
      JMG_ENFORCE_AT_SRC_USING(RuntimePythonTypeError, src, location,
                               "python object pointer was NULL");
      obj_ = src;
      Py_XINCREF(obj_);
    }
    else if constexpr (SameAsDecayedT<bool, T>) {
      obj_ = PyBool_FromLong(src ? 1 : 0);
    }
    else if constexpr (IntegralT<DecayT<T>>) {
      if constexpr (SignedT<T>) {
        if constexpr (sizeof(DecayT<T>) <= sizeof(long)) {
          obj_ = PyLong_FromLong(static_cast<long>(src));
        }
        else { obj_ = PyLong_FromLongLong(static_cast<long long>(src)); }
      }
      else {
        if constexpr (sizeof(DecayT<T>) <= sizeof(unsigned long)) {
          obj_ = PyLong_FromUnsignedLong(static_cast<unsigned long>(src));
        }
        else {
          obj_ =
            PyLong_FromUnsignedLongLong(static_cast<unsigned long long>(src));
        }
      }
    }
    else if constexpr (FloatingPointT<T>) {
      obj_ = PyFloat_FromDouble(static_cast<double>(src));
    }
    else if constexpr (StringLikeT<T>) {
      if constexpr (CStyleStringT<T>) { obj_ = PyUnicode_FromString(src); }
      else { obj_ = PyUnicode_FromStringAndSize(src.data(), src.size()); }
    }
    else {
      JMG_THROW_EXCEPTION(
        std::logic_error,
        "attempted to construct a python object for C++ type [",
        type_name_for<T>(), "]");
    }
  }

  /**
   * return the pointer to the wrapped python object
   */
  [[nodiscard]] PyObject* operator*() const;

  /**
   * return the pointer to the wrapped python object and release the ownership
   * share that the wrapper object holds in it
   */
  [[nodiscard]] PyObject* release() &&;

  /**
   * retrieve a C++ value from the value in the python object
   */
  template<typename T>
  T as(const std::source_location location =
         std::source_location::current()) const {
    if constexpr (SameAsDecayedT<bool, T>) {
      return (PyObject_IsTrue(obj_) > 0);
    }
    else if constexpr (IntegralT<T>) {
      // TODO(bd) allow implicit conversion between python floating
      // point and C++ integer?
      JMG_ENFORCE_AT_SRC_USING(
        RuntimePythonTypeError, PyLong_Check(obj_), location,
        "unable to convert python object to an integer value");
      if constexpr (SignedT<T>) {
        const int64_t val = PyLong_AsLongLong(obj_);
        if (kFailInt == val) {
          MAYBE_HANDLE_PYTHON_ERROR("retrieving signed integer from python");
        }
        // TODO(bd) add safe integer type conversions to jmg::from
        return boost::numeric_cast<T>(val);
      }
      else {
        const uint64_t val = PyLong_AsUnsignedLongLong(obj_);
        if ((uint64_t)kFailInt == val) {
          MAYBE_HANDLE_PYTHON_ERROR("retrieving unsigned integer from python");
        }
        // TODO(bd) add safe integer type conversions to jmg::from
        return boost::numeric_cast<T>(val);
      }
    }
    else if constexpr (FloatingPointT<T>) {
      JMG_ENFORCE(PyFloat_Check(obj_) || PyLong_Check(obj_),
                  "unable to convert python object to a floating point value");
      return static_cast<T>(PyFloat_AsDouble(obj_));
    }
    else if constexpr (StringLikeT<T>) {
      const char* val = PyUnicode_AsUTF8(obj_);
      if (!val) {
        MAYBE_HANDLE_PYTHON_ERROR("retrieving string from python");
        // TODO(bd) probably not correct, but assume that nullptr
        // returned from PyUnicode_AsUTF8() with no python error set
        // indicates empty string
        if constexpr (CStyleStringT<T>) { return nullptr; }
        else { return T(); }
      }
      if constexpr (CStyleStringT<T>) { return val; }
      else { return T(val); }
    }
    else {
      JMG_THROW_EXCEPTION_AT_SRC(std::logic_error, location,
                                 "TODO(bd) return non-primitive [",
                                 type_name_for<T>(), "]");
    }
  }

private:
  PyObject* obj_ = nullptr;
};

} // namespace jmg::python

#undef MAYBE_HANDLE_PYTHON_ERROR
