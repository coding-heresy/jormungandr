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

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <meta>
#include <numeric>
#include <ranges>
#include <string_view>
#include <vector>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <boost/numeric/conversion/cast.hpp>

#include "jmg/conversion.h"
#include "jmg/reflection_util.h"
#include "jmg/types.h"
#include "jmg/util.h"

namespace rflx = std::meta;
namespace rng = std::ranges;
namespace vws = std::views;

////////////////////
// useful macros
#if (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 12) || (PY_MAJOR_VERSION > 3)
// NOTE: use PyErr_GetRaisedException for python version >= 3.12
#define MAYBE_HANDLE_PYTHON_ERROR(...)                                  \
  do {                                                                  \
    if (PyErr_Occurred()) {                                             \
      auto* py_exception = PyErr_GetRaisedException();                  \
      JMG_ENFORCE(py_exception,                                         \
                  "a python error occurred exception object could not " \
                  "be retrieved");                                      \
      const auto exc_cleanup = Cleanup([&]() { Py_DECREF(py_exception); }); \
      auto* py_err_msg = PyObject_Str(py_exception);                    \
      JMG_ENFORCE(py_err_msg,                                           \
                  "unable to get string value for python exception");   \
      const auto msg_cleanup = Cleanup([&]() { Py_DECREF(py_err_msg); }); \
      const char* err_msg = PyUnicode_AsUTF8(py_err_msg);               \
      JMG_ENFORCE(err_msg,                                              \
                  "unable to get string value for python exception");   \
      JMG_THROW_EXCEPTION(std::runtime_error, "python exception [",     \
                          err_msg, "] occurred when ", __VA_ARGS__);    \
    }                                                                   \
  } while (0)
#else
// NOTE: use PyErr_Fetch for python version < 3.12
#define MAYBE_HANDLE_PYTHON_ERROR(...)                                  \
  do {                                                                  \
    if (PyErr_Occurred()) {                                             \
      PyObject* py_type = nullptr;                                      \
      PyObject* py_val = nullptr;                                       \
      PyObject* py_traceback = nullptr;                                 \
      PyErr_Fetch(&py_type, &py_val, &py_traceback);                    \
      const auto err_cleanup = Cleanup([&]() {                          \
        Py_XDECREF(py_type);                                            \
        Py_XDECREF(py_val);                                             \
        Py_XDECREF(py_traceback);                                       \
      });                                                               \
      JMG_ENFORCE(py_val,                                               \
                  "a python error occurred but could not be fetcted");  \
      auto* py_err_msg = PyObject_Str(py_val);                          \
      JMG_ENFORCE(py_err_msg,                                           \
                  "unable to get string value for python exception");   \
      const auto msg_cleanup = Cleanup([&]() { Py_DECREF(py_err_msg); }); \
      const char* err_msg = PyUnicode_AsUTF8(py_err_msg);               \
      JMG_ENFORCE(err_msg,                                              \
                  "unable to get string value for python exception");   \
      JMG_THROW_EXCEPTION(std::runtime_error, "python exception [",     \
                          std::string_view(err_msg), "] occurred when ", \
                          __VA_ARGS__);                                 \
    }                                                                   \
  } while (0)
#endif

namespace jmg
{

////////////////////
// useful constants and types

constexpr auto kMethodListTerminator =
  PyMethodDef{nullptr, nullptr, 0, nullptr};
using MethodDefs = std::vector<PyMethodDef>;
using ClassDefEntry = std::tuple<std::string_view, PyTypeObject*>;
using ClassDefs = std::vector<ClassDefEntry>;
constexpr auto kAnyAccess = rflx::access_context::unchecked();
constexpr auto kPublicAccess = rflx::access_context::unprivileged();
constexpr int64_t kFailInt = -1LL;
constexpr int kPyErr = -1;
constexpr int kPySuccess = 0;
constexpr auto kTrueStr = std::string_view("True");
constexpr auto kFalseStr = std::string_view("False");

////////////////////
// type wrappers

/**
 * wrapper class for a C++ class used by python
 */
template<ClassT T>
struct PythonObjWrapper {
  PyObject_HEAD std::optional<T> instance;
};

/**
 * wrapper class that supports iteration over C++ objects using
 * standard python constructs
 */
template <std::ranges::random_access_range Container>
struct PythonItrState {
  PyObject_HEAD
  using value_type = std::ranges::range_value_t<Container>;
  using iterator = std::ranges::iterator_t<Container>;
  PyObject* iterable;
  iterator current;
  iterator end;
  PythonItrState(PyObject* i, iterator&& c, iterator&& e)
    : iterable(i), current(c), end(e) {
    Py_INCREF(iterable);
  }
  ~PythonItrState() {
    Py_XDECREF(iterable);
  }
};

/**
 * metaclass associated with a class used to access static member
 * functions and data members
 */
struct PythonMetaclassWrapper {
  PyHeapTypeObject base;
};

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

namespace detail
{
template<typename T>
struct PythonObjWrapperT : std::false_type {};
template<typename T>
struct PythonObjWrapperT<PythonObjWrapper<T>> : std::true_type {};
} // namespace detail

template<typename T>
concept PythonObjWrapperT = detail::PythonObjWrapperT<T>::value;

namespace detail
{

////////////////////
// Python-specific classes

class PythonString {
public:
  template <typename... Args>
  PythonString(Args&&... args)
    : cpp_str_(str_cat(std::forward<Args>(args)...))
    , py_str_(PyUnicode_FromStringAndSize(cpp_str_.data(), cpp_str_.size()))
  {
    JMG_ENFORCE(py_str_, "unable to create python string");
  }
  ~PythonString() { Py_DECREF(py_str_); }
  PyObject* operator*() const { return py_str_; }
private:
  std::string cpp_str_;
  PyObject* py_str_;
};

////////////////////
// Python-specific functions

/**
 * get a pointer to the C++ type wrapped in a python object
 *
 * TODO(bd) modify this to return a non-const ref
 */
template<typename T>
decltype(auto) get_cpp_instance(PyObject* self,
                                const std::source_location location =
                                std::source_location::current()) {
  auto* py_obj = reinterpret_cast<T*>(self);
  JMG_ENFORCE(py_obj, "python object is null");
  JMG_ENFORCE_USING(RuntimePythonTypeError, py_obj->instance,
                    "C++ instance was not properly initialized");
  return *(py_obj->instance);
}

/**
 * convert a c++ type to a python type
 *
 * TODO(bd) integrate with return type overloading?
 */
template<typename T>
PyObject* to_python(const T& value) {
  if constexpr (SameAsDecayedT<PyObject*, T>) {
    // If it's already a raw PyObject*, pass it through directly
    return value;
  }
  else if constexpr (SameAsDecayedT<bool, T>) {
    if (value) { Py_RETURN_TRUE; }
    Py_RETURN_FALSE;
  }
  else if constexpr (IntegralT<T>) {
    if constexpr (SignedT<T>) {
      return PyLong_FromLongLong(static_cast<int64_t>(value));
    }
    else { return PyLong_FromUnsignedLongLong(static_cast<uint64_t>(value)); }
  }
  else if constexpr (FloatingPointT<T>) {
    return PyFloat_FromDouble(static_cast<double>(value));
  }
  else if constexpr (StdStringLikeT<T>) {
    return PyUnicode_FromStringAndSize(value.data(), value.size());
  }
  else if constexpr (CStyleStringT<T>) {
    if (!value) { Py_RETURN_NONE; }
    return PyUnicode_FromString(value);
  }
  JMG_ENFORCE_USING(std::logic_error, false, "TODO(bd) return non-primitive");
}

template<typename T>
T from_python(PyObject* py_obj) {
  if constexpr (SameAsDecayedT<bool, T>) {
    return (PyObject_IsTrue(py_obj) > 0);
  }
  else if constexpr (IntegralT<T>) {
    // TODO(bd) allow implicit conversion between python floating
    // point and c++ integer?
    JMG_ENFORCE(PyLong_Check(py_obj),
                "unable to convert python object to an integer value");
    if constexpr (SignedT<T>) {
      const int64_t val = PyLong_AsLongLong(py_obj);
      if (kFailInt == val) {
        MAYBE_HANDLE_PYTHON_ERROR("retrieving signed integer from python");
      }
      // TODO(bd) add safe integer type conversions to jmg::from
      return boost::numeric_cast<T>(val);
    }
    else {
      const uint64_t val = PyLong_AsUnsignedLongLong(py_obj);
      if ((uint64_t)kFailInt == val) {
        MAYBE_HANDLE_PYTHON_ERROR("retrieving unsigned integer from python");
      }
      // TODO(bd) add safe integer type conversions to jmg::from
      return boost::numeric_cast<T>(val);
    }
  }
  else if constexpr (FloatingPointT<T>) {
    JMG_ENFORCE(PyFloat_Check(py_obj) || PyLong_Check(py_obj),
                "unable to convert python object to a floating point value");
    return static_cast<T>(PyFloat_AsDouble(py_obj));
  }
  // TODO(bd) handle string_view or char*?
  else if constexpr (SameAsDecayedT<std::string, T>) {
    const char* val = PyUnicode_AsUTF8(py_obj);
    if (!val) {
      MAYBE_HANDLE_PYTHON_ERROR("retrieving string from python");
      // TODO(bd) probably not correct, but assume that nullptr
      // returned from PyUnicode_AsUTF8() with no python error set
      // indicates empty string
      return std::string();
    }
    return std::string(val);
  }
  JMG_ENFORCE_USING(std::logic_error, false, "TODO(bd) return non-primitive");
}

/**
 * TODO(bd) this currently does not work due to some weird compiler
 * issue, see related TODO comments associated with attribute name
 * handling
 */
std::string_view get_python_attr_name(PyObject* name) {
  const char* attr_name = PyUnicode_AsUTF8(name);
  if (!attr_name) { throw RuntimePythonErrorNoCppMsg(); }
  return std::string_view(attr_name);
}

/**
 * compile time-only reflection metafunction that returns the python
 * function parameter format string for a type
 *
 * TODO(bd) convert the code to strict type checking in order to avoid
 * ambiguity when choosing constructors?
 *
 * TODO(bd) figure out how to handle optional parameters
 *
 * TODO(bd) handle std::string and std::string_view arguments using
 * 's#' format that supports zero copy for std::string_view and avoids
 * an error for std::string if the python string contains one or more
 * NULL bytes
 *
 * TODO(bd) handle user-defined types
 */
template<typename T>
consteval char get_python_format_char() {
  if constexpr (SameAsDecayedT<int, T>) { return 'i'; }
  else if constexpr (SameAsDecayedT<long, T>) { return 'l'; }
  else if constexpr (SameAsDecayedT<float, T>) { return 'f'; }
  else if constexpr (SameAsDecayedT<double, T>) { return 'd'; }
  else if constexpr (SameAsDecayedT<bool, T>) {
    return 'p'; // boolean predicate
  }
  else if constexpr (StringLikeT<T>) {
    return 's';
  }
  JMG_THROW_EXCEPTION(std::runtime_error,
                      "encountered unexpected/unsupported C++ type when "
                      "getting python type format character");
}

/**
 * compile time-only reflection metafunction which constructs a format
 * string that will allow the PyArg_ParseTuple function to convert the
 * python object containing function arguments into a C++ tuple that
 * can be used with std::apply to execute a C++ function
 */
template<rflx::info Fcn>
consteval auto make_python_method_params_fmt_str() {
  static constexpr auto params =
    std::define_static_array(rflx::parameters_of(Fcn));
  // always needs null terminator
  std::array<char, params.size() + 1> rslt;
  size_t idx = 0;
  template for (constexpr auto param : params) {
    using ParamType = typename[:rflx::type_of(param):];
    rslt[idx++] = get_python_format_char<ParamType>();
  }
  rslt[idx] = '\0';
  return rslt;
}

/**
 * invoke a C++ function called via a python method whose arguments
 * have already been converted into a tuple and return a value usable
 * by python, allowing exceptions to pass through under the
 * expectation that they will be handled by the caller
 */
template<rflx::info FcnInfo, TupleT ArgsTpl>
PyObject* invoke_from_python(ArgsTpl args_tpl) {
  auto fcn = &[:FcnInfo:];
  if constexpr (rflx::return_type_of(FcnInfo) == ^^void) {
    std::apply(fcn, args_tpl);
    Py_RETURN_NONE;
  }
  else {
    auto cpp_rslt = std::apply(fcn, args_tpl);
    return to_python(std::forward<decltype(cpp_rslt)>(cpp_rslt));
  }
}

/**
 * return a value indicating failure using a type that is appropriate
 * for the function that is failing
 */
template<typename T>
consteval auto fail_return() {
  if constexpr (SameAsDecayedT<int, T>) {
    return kPyErr;
  }
  else if constexpr (SameAsDecayedT<Py_ssize_t, T>) {
    return static_cast<Py_ssize_t>(0);
  }
  else if constexpr (std::is_pointer_v<T>) {
    return static_cast<T>(nullptr);
  }
  else {
    JMG_NOT_EXHAUSTIVE(T, "unknown/unsupported return type");
  }
}

/**
 * invoke any C++ function called from python, returning the result
 * (if any) in a form usable by python and automatically converting
 * any C++ exceptions that occur to python exceptions
 */
template<typename Fcn>
auto sinking_invoke_from_python(Fcn&& fcn) {
  using Rslt = decltype(fcn());
  try {
    return fcn();
  }
  catch (const RuntimePythonErrorNoCppMsg& e) {
    // no need to set a python error string
    return fail_return<Rslt>();
  }
  catch (const RuntimePythonTypeError& e) {
    PyErr_SetString(PyExc_TypeError, e.what());
    return fail_return<Rslt>();
  }
  catch (const std::invalid_argument& e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return fail_return<Rslt>();
  }
  catch (const std::out_of_range& e) {
    PyErr_SetString(PyExc_IndexError, e.what());
    return fail_return<Rslt>();
  }
  catch (const std::runtime_error& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return fail_return<Rslt>();
  }
  catch (const std::exception& e) {
    PyErr_SetString(PyExc_Exception, e.what());
    return fail_return<Rslt>();
  }
  catch (...) {
    auto err_msg = str_cat("unknown/unexpected c++ exception of type [",
                           current_exception_type_name(), "] occurred.");
    PyErr_SetString(PyExc_SystemError, err_msg.c_str());
    return fail_return<Rslt>();
  }
}

/**
 * class used to generate the tuple type needed to store the result of
 * converting python arguments to a form usable in C++
 */
template<rflx::info Fcn>
class FcnArgsConverter {
  /**
   * special handling of tuple storage for string types
   */
  template<typename T>
  struct TgtStorageType {
    using type = std::conditional_t<StdStringLikeT<T>, const char*, T>;
  };

  /**
   * alias for special storage type
   */
  template<typename T>
  using TgtStorageTypeT = typename TgtStorageType<T>::type;

  /**
   * dummy function that is never called and is only used as a
   * convenient way to generate the tuple type
   */
  static consteval rflx::info meta_make_tuple() {
    constexpr auto params = std::define_static_array(rflx::parameters_of(Fcn));
    return [&]<size_t... kIdxs>(std::index_sequence<kIdxs...>) {
      return ^^std::tuple<TgtStorageTypeT<typename[:rflx::type_of(params.data()[kIdxs]):]>...>;
    }(std::make_index_sequence<params.size()>{});
  }

  /**
   * alias for tuple-ized parameters type
   */
  using ArgsTpl = typename[:meta_make_tuple():];

public:
  /**
   * convert python arguments into a form usable by C++
   */
  static auto convert(PyObject* args, const bool is_searching) {
    // construct a format string that will allow python to convert the
    // function arguments into a tuple
    constexpr auto fmt_array =
      detail::make_python_method_params_fmt_str<Fcn>();
    const char* fmt = fmt_array.data();

    // determine the type of the tuple that holds the argument
    using Rslt = std::optional<ArgsTpl>;
    Rslt rslt{};

    // don't bother trying to parse the arguments if the sizes don't match
    if (PyTuple_Size(args) != std::tuple_size_v<ArgsTpl>) {
      if (!is_searching) {
        const auto err_msg =
          PythonString("size mismatch on method arguments, expected [",
                       std::tuple_size_v<ArgsTpl>, "] but got [",
                       PyTuple_Size(args), "]");
        PyErr_SetObject(PyExc_TypeError, *err_msg);
      }
      return rslt;
    }

    // convert the arguments provided by python into a tuple that can be
    // used to execute the member function with std::apply
    ArgsTpl parsed_args;
    auto is_parsed = [&]<size_t... Is>(std::index_sequence<Is...>) {
      return PyArg_ParseTuple(args, fmt, &std::get<Is>(parsed_args)...);
    }(std::make_index_sequence<std::tuple_size_v<ArgsTpl>>{});
    if (is_parsed) { rslt = std::move(parsed_args); }
    else {
      // clear the global python error state set by the failed parse to
      // allow further matches to be attempted
      if (is_searching) {
        PyErr_Clear();
      }
    }
    return rslt;
  }
};

/**
 * convert python arguments into a form usable by the function whose
 * type info was used to instantiate this function template
 */
template<rflx::info Fcn>
auto parse_python_args(PyObject* args, const bool is_searching = false) {
  return FcnArgsConverter<Fcn>::convert(args, is_searching);
}

/**
 * invoke a C++ non-static member function called via a python method,
 * returning the result (if any) in a form usable by python and
 * automatically converting any C++ exceptions that occur to python
 * exceptions
 */
template<rflx::info MbrFcn, typename CppObj>
PyObject* sinking_invoke_from_python(CppObj* cpp_obj, PyObject* args) {
  auto dispatch = [&]() -> PyObject* {
    auto parsed_args = parse_python_args<MbrFcn>(args);
    if (!parsed_args) {
      // python error state was set by PyArg_ParseTuple
      return nullptr;
    }

    auto mem_fcn_args =
      std::tuple_cat(std::make_tuple(cpp_obj),
                     std::move(*parsed_args));

    return invoke_from_python<MbrFcn>(std::move(mem_fcn_args));
  };
  return detail::sinking_invoke_from_python(dispatch);
}

/**
 * return true if the parameter is a public member function, false otherwise
 */
template<rflx::info Mbr>
consteval bool is_public_member_function() {
  if constexpr (rflx::is_function(Mbr)
                && !rflx::is_constructor(Mbr)
                && !rflx::is_destructor(Mbr)
                && rflx::has_identifier(Mbr)) {
    return true;
  }
  else { return false; }
}

/**
 * lazily generate wrappers that will allow C++ member functions to be
 * called from python
 */
template<ClassT T>
class LazyPythonMethodFactory {
public:
  static constinit inline auto methods = []() {
    using CppClass = typename T::CppType;
    static constexpr auto mbrs =
      std::define_static_array(rflx::members_of(^^CppClass, kPublicAccess));

    std::array<PyMethodDef, mbrs.size() + 1> rslt{};
    size_t idx = 0;
    template for (constexpr auto mbr : mbrs) {
      if constexpr (is_public_member_function<mbr>()) {
        const auto* snake_case_name = SnakeCaseIdOwner<mbr>().c_str();
        if constexpr (!rflx::is_static_member(mbr)) {
          auto method =
            reinterpret_cast<PyCFunction>(&T::template callMemberFcn<mbr>);
          rslt[idx++] = PyMethodDef{.ml_name = snake_case_name,
                                    .ml_meth = method,
                                    .ml_flags = METH_VARARGS,
                                    .ml_doc = "TODO(bd) some doc string"};
        }
        else {
          auto method =
            reinterpret_cast<PyCFunction>(&T::template callStaticMemberFcn<mbr>);
          rslt[idx++] = PyMethodDef{.ml_name = snake_case_name,
                                    .ml_meth = method,
                                    .ml_flags = METH_VARARGS | METH_STATIC,
                                    .ml_doc = "TODO(bd) some doc string"};
        }
      }
    }
    rslt[mbrs.size()] = kMethodListTerminator;
    return rslt;
  }();
};

} // namespace detail

/**
 * simple interface whose purpose is to permit member functions related to
 * python interfaces to remain private
 */
struct ModuleFactory {
  virtual PyObject* make() = 0;
};

/**
 * global registry of python modules used by the reflexive as part of the
 * protocol for automatic creation of python bindings at static time ,either
 * during program startup or shared object library loading
 */
class ModuleRegistry {
  using Registry = Dict<std::string,
                        std::reference_wrapper<ModuleFactory>,
                        "module name",
                        "module registry">;
  static Registry& registry() {
    static Registry registry{};
    return registry;
  }

public:
  /**
   * register a module factory by its python name
   */
  static void registerFactory(const std::string_view name,
                              ModuleFactory& factory) {
    registry().emplace_uniq(std::string(name), std::ref(factory));
  }

  /**
   * look up a module factory by its python name
   */
  static ModuleFactory& lookupFactory(const std::string_view name) {
    return registry().find_required(std::string(name)).get();
  }
};

/**
 * mixin class template that declares a python module
 */
template<typename Derived,
         const std::string_view& kDocStr,
         typename... Wraps>
class PythonModule {
  static_assert(!kDocStr.empty(), "module doc string may not be empty");

public:
  /**
   * generate the python module name from the derived class name
   *
   * NOTE: by convention, python module names use snake_case
   */
  static const char* name() {
    static constexpr auto owner = SnakeCaseIdOwner<^^Derived>();
    return owner.c_str();
  }

  /**
   * access to the list of methods associated with the module
   */
  static MethodDefs& staticMethods() {
    static MethodDefs defs{};
    return defs;
  }

  /**
   * access to the list of classes associated with the module
   */
  static ClassDefs& classes() {
    static ClassDefs defs{};
    return defs;
  }

private:
  /**
   * convert the non-type template parameter with the module doc
   * string into a form that can be passed to the Python API
   */
  static const std::string& docStr() {
    static std::string docStr{};
    if (docStr.empty()) { docStr = std::string(kDocStr); }
    return docStr;
  }

  /**
   * function that creates the python object for the module
   */
  static PyObject* make() {
    static bool is_created = false;
    if (is_created) { return nullptr; }

    auto processWrap = [&]<typename T>() {
      auto [class_name, py_obj] = T::make(std::string_view(name()));
      auto tpl = std::make_tuple(std::string_view(class_name), py_obj);
      classes().push_back(std::move(tpl));
    };
    (processWrap.template operator()<Wraps>(), ...);

    // add sentinel null entry
    staticMethods().push_back(kMethodListTerminator);

    static PyModuleDef def = {PyModuleDef_HEAD_INIT, name(),
                              docStr().data(), -1, staticMethods().data()};
    PyObject* module = PyModule_Create(&def);
    if (!module) { return nullptr; }

    for (auto& [class_name, py_obj] : classes()) {
      if (PyType_Ready(py_obj) < 0) {
        Py_DECREF(module);
        return nullptr;
      }

      Py_INCREF(py_obj);
      PyModule_AddObject(module, class_name.data(),
                         reinterpret_cast<PyObject*>(py_obj));
    }

    is_created = true;
    return module;
  }

  /**
   * module factory that allows the make() member function to remain private
   */
  struct Factory : public ModuleFactory {
    PyObject* make() override {
      return PythonModule::make();
    }
  };

  friend Factory;

  /**
   * Myers singleton for the factory class
   */
  static Factory& factory() {
    static Factory factory;
    return factory;
  }

  /**
   * automatic registration with the module registry at static time
   */
  static inline bool is_registered_ = []() {
    ModuleRegistry::registerFactory(name(), factory());
    return true;
  }();

  template<bool&>
  struct ForceRegistration {};
  inline static ForceRegistration<is_registered_> force_registration_;
};

/**
 * Class template that uses reflection to create a python class from a
 * C++ class
 */
template<typename CppClass, const std::string_view& kDocStr>
class PythonReflex {
private:
  using type = PythonReflex<CppClass, kDocStr>;
  using CppType = CppClass;
  friend detail::LazyPythonMethodFactory<type>;

  /**
   * convert the non-type template parameter with the class doc
   * string into a form that can be passed to the Python API
   */
  static const std::string& docStr() {
    static std::string docStr{};
    if (docStr.empty()) { docStr = std::string(kDocStr); }
    return docStr;
  }

  /**
   * wrapper for internal python representation of the class
   */
  using PyObj = PythonObjWrapper<CppClass>;

  /**
   * memory deallocator
   */
  static void destroy(PyObject* self) {
    auto* py_obj = reinterpret_cast<PyObj*>(self);
    if (py_obj->instance) {
      py_obj->instance = std::nullopt;
    }
    Py_TYPE(self)->tp_free(self);
  }

  /**
   * python object allocator
   */
  static PyObject* allocate(PyTypeObject* python_type,
                            PyObject* args,
                            PyObject* kwds) {
    auto* py_obj =
      reinterpret_cast<PyObj*>(python_type->tp_alloc(python_type, 0));
    if (!py_obj) { return nullptr; }
    new (&(py_obj->instance)) std::optional<CppType>(std::nullopt);
    return reinterpret_cast<PyObject*>(py_obj);
  }

  /**
   * c++ object initializer/constructor
   */
  static int construct(PyObject* self, PyObject* args, PyObject* kwds) {
    return detail::sinking_invoke_from_python([&]() -> int {
      if (kwds && (PyDict_Size(kwds) > 0)) {
        // TODO(bd) support keyword arguments for constructors?
        PyErr_SetString(PyExc_TypeError,
                        "keyword arguments for constructors are not yet "
                        "supported");
        return kPyErr;
      }

      auto* py_obj = reinterpret_cast<PyObj*>(self);
      JMG_ENFORCE_USING(std::invalid_argument, py_obj,
                        "python object is null");

      static constexpr auto mbrs =
        std::define_static_array(rflx::members_of(^^CppClass, kPublicAccess));

      template for (constexpr rflx::info mbr : mbrs) {
        // TODO(bd) support copy constructor?
        if constexpr (rflx::is_constructor(mbr)
                      && !rflx::is_copy_constructor(mbr)
                      && !rflx::is_move_constructor(mbr)
                      && !rflx::is_deleted(mbr)) {
          auto parsed_args =
            detail::parse_python_args<mbr>(args, true /* is_searching */);
          if (parsed_args) {
            std::apply([&](auto... arg_vals) {
              py_obj->instance.emplace(std::forward<decltype(arg_vals)>(arg_vals)...);
            }, *parsed_args);
            return kPySuccess;
          }
        }
      }
      PyErr_SetString(PyExc_TypeError,
                      "no matching C++ constructor signature found");
      return kPyErr;
    });
  }

  /**
   * generic static member function that implements calling a non-static public
   * member function of the derived class from python
   */
  template<rflx::info MbrFcn>
  static PyObject* callMemberFcn(PyObject* self, PyObject* args) {
    return detail::sinking_invoke_from_python([&]() -> PyObject* {
      auto& cpp_obj = detail::get_cpp_instance<PyObj>(self);
      auto parsed_args = detail::parse_python_args<MbrFcn>(args);
      if (!parsed_args) { return nullptr; }
      auto mbr_fcn_args = std::tuple_cat(std::make_tuple(&cpp_obj),
                                         std::move(*parsed_args));
      return detail::invoke_from_python<MbrFcn>(std::move(mbr_fcn_args));
    });
  }

  /**
   * generic static member function that implements calling a non-static public
   * member function of the derived class from python
   */
  template<rflx::info MbrFcn>
  static PyObject* callStaticMemberFcn(PyObject* self, PyObject* args) {
    return detail::sinking_invoke_from_python([&]() -> PyObject* {
      auto parsed_args = detail::parse_python_args<MbrFcn>(args);
      if (!parsed_args) { return nullptr; }
      return detail::invoke_from_python<MbrFcn>(std::move(*parsed_args));
    });
  }

  /**
   * static member function that implements retrieval of C++ public
   * data members as python attributes
   */
  static PyObject* getDataMember(PyObject* self, PyObject* name) {
    return detail::sinking_invoke_from_python([&]() -> PyObject* {
      auto& cpp_obj = detail::get_cpp_instance<PyObj>(self);

      // TODO(bd) figure out why attempting to factor out a function
      // to get a string_view for the attribute name results in the
      // compiler believing that the variable used to store the name
      // is not used
      const char* raw_attr_name = PyUnicode_AsUTF8(name);
      if (!raw_attr_name) { return nullptr; }
      std::string_view attr_name(raw_attr_name);

      static constexpr auto mbrs =
        std::define_static_array(rflx::nonstatic_data_members_of(^^CppClass,
                                                                 kPublicAccess));

      template for (constexpr rflx::info mbr : mbrs) {
        if (std::string_view(SnakeCaseIdOwner<mbr>::c_str()) == attr_name) {
          return detail::to_python(cpp_obj.[:mbr:]);
        }
      }
      return PyObject_GenericGetAttr(self, name);
    });
  }

  /**
   * static member function that implements update of C++ public data
   * members as python attributes
   */
  static int setDataMember(PyObject* self, PyObject* name, PyObject* value) {
    return detail::sinking_invoke_from_python([&]() -> int {
      if (!value) {
        PyErr_SetString(PyExc_TypeError,
                        "illegal attempt to delete an attribute of a wrapped "
                        "C++ class");
        return kPyErr;
      }
      auto& cpp_obj = detail::get_cpp_instance<PyObj>(self);

      // TODO(bd) figure out why attempting to factor out a function
      // to get a string_view for the attribute name results in the
      // compiler believing that the variable used to store the name
      // is not used
      const char* raw_attr_name = PyUnicode_AsUTF8(name);
      if (!raw_attr_name) { return kPyErr; }
      std::string_view attr_name(raw_attr_name);

      static constexpr auto mbrs =
        std::define_static_array(rflx::nonstatic_data_members_of(^^CppClass,
                                                                 kPublicAccess));

      template for (constexpr rflx::info mbr : mbrs) {
        if (std::string_view(SnakeCaseIdOwner<mbr>::c_str()) == attr_name) {
          using MbrType = DecayT<decltype(cpp_obj.[:mbr:])>;
          cpp_obj.[:mbr:] = detail::from_python<MbrType>(value);
          return kPySuccess;
        }
      }
      return PyObject_GenericSetAttr(self, name, value);
    });
  }

  /**
   * static member function that implements retrieval of C++ public
   * static data members as python class attributes
   */
  static PyObject* getStaticDataMember(PyObject* metaclass, PyObject* name) {
    return detail::sinking_invoke_from_python([&]() -> PyObject* {

      // TODO(bd) figure out why attempting to factor out a function
      // to get a string_view for the attribute name results in the
      // compiler believing that the variable used to store the name
      // is not used
      const char* raw_attr_name = PyUnicode_AsUTF8(name);
      if (!raw_attr_name) { return nullptr; }
      std::string_view attr_name(raw_attr_name);

      static constexpr auto mbrs =
        std::define_static_array(rflx::static_data_members_of(^^CppClass,
                                                              kPublicAccess));

      template for (constexpr rflx::info mbr : mbrs) {
        if (std::string_view(SnakeCaseIdOwner<mbr>::c_str()) == attr_name) {
          return detail::to_python([:mbr:]);
        }
      }
      return PyType_Type.tp_getattro(metaclass, name);
    });
  }

  /**
   * static member function that implements update of C++ public
   * static data members as python class attributes
   */
  static int setStaticDataMember(PyObject* metaclass,
                                 PyObject* name,
                                 PyObject* value) {
    return detail::sinking_invoke_from_python([&]() -> int {
      if (!value) {
        PyErr_SetString(PyExc_TypeError,
                        "illegal attempt to delete an attribute of a wrapped "
                        "C++ metaclass");
        return kPyErr;
      }

      // TODO(bd) figure out why attempting to factor out a function
      // to get a string_view for the attribute name results in the
      // compiler believing that the variable used to store the name
      // is not used
      const char* raw_attr_name = PyUnicode_AsUTF8(name);
      if (!raw_attr_name) { return kPyErr; }
      std::string_view attr_name(raw_attr_name);

      static constexpr auto mbrs =
        std::define_static_array(rflx::static_data_members_of(^^CppClass,
                                                              kPublicAccess));

      template for (constexpr rflx::info mbr : mbrs) {
        if (std::string_view(SnakeCaseIdOwner<mbr>::c_str()) == attr_name) {
          using MbrType = DecayT<decltype([:mbr:])>;
          [:mbr:] = detail::from_python<MbrType>(value);
          return kPySuccess;
        }
      }
      return PyType_Type.tp_setattro(metaclass, name, value);
    });
  }

  /**
   * static member function that creates an iterator for an iterable
   * C++ object
   */
  static PyObject* iterator(PyObject* self) {
    namespace rng = std::ranges;
    return detail::sinking_invoke_from_python([&]() -> PyObject* {
      if constexpr (!rng::random_access_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError, "object is not iterable");
      }
      else {
        using ItrState = PythonItrState<CppType>;
        using ItrWrapper = PythonObjWrapper<ItrState>;
        auto& cpp_obj = detail::get_cpp_instance<PyObj>(self);
        auto* itr_obj =
          reinterpret_cast<ItrWrapper*>(PyType_GenericAlloc(Py_TYPE(self), 0));
        if (!itr_obj) { throw RuntimePythonErrorNoCppMsg(); }
        new (&(itr_obj->instance)) std::optional<ItrState>(std::nullopt);
        itr_obj->instance.emplace(self,
                                  rng::begin(cpp_obj),
                                  rng::end(cpp_obj));
        return reinterpret_cast<PyObject*>(itr_obj);
      }
    });
  }

  /**
   * static member function that returns the next item from an
   * iterator for an iterable C++ object
   */
  static PyObject* nextItem(PyObject* self) {
    namespace rng = std::ranges;
    return detail::sinking_invoke_from_python([&]() -> PyObject* {
      if constexpr (!rng::random_access_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError, "object is not iterable");
      }
      else {
        using ItrState = PythonItrState<CppType>;
        using ItrWrapper = PythonObjWrapper<ItrState>;
        auto& itr_state = detail::get_cpp_instance<ItrWrapper>(self);
        if (itr_state.end == itr_state.current) { return nullptr; }

        using ValueType =
          typename ItrState::value_type;
        PyObject* item = detail::to_python<ValueType>(*(itr_state.current));
        ++(itr_state.current);
        return item;
      }
    });
  }

  /**
   * static member function that returns the size/length of an object
   */
  static Py_ssize_t sequenceSz(PyObject* self) {
    namespace rng = std::ranges;
    return detail::sinking_invoke_from_python([&]() -> Py_ssize_t {
      if constexpr (!rng::sized_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError, "object has no len()");
      }
      else {
        auto& cpp_obj = detail::get_cpp_instance<PyObj>(self);
        return cpp_obj.size();
      }
    });
  }

  static PyObject* sequenceItem(PyObject* self, Py_ssize_t idx) {
    namespace rng = std::ranges;
    return detail::sinking_invoke_from_python([&]() -> PyObject* {
      // TODO(bd) is random_access_range specific enough here?
      if constexpr (!rng::random_access_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError,
                            "object is not subscriptable");
      }
      else {
        auto& cpp_obj = detail::get_cpp_instance<PyObj>(self);
        const auto sz = static_cast<Py_ssize_t>(cpp_obj.size());

        // support pythonic negative indexing (e.g., v[-1])
        if (idx < 0) { idx += sz; }
        JMG_ENFORCE_USING(std::out_of_range, (idx >= 0) && (idx < sz),
                          "index [", idx, "] out of range [0..", sz, "]");
        using ItemType = CppType::value_type;
        return detail::to_python<ItemType>(cpp_obj[idx]);
      }
    });
  }

  static int sequenceSetItem(PyObject* self,
                                    Py_ssize_t idx,
                                    PyObject* value) {
    namespace rng = std::ranges;
    return detail::sinking_invoke_from_python([&]() -> int {
      // TODO(bd) is random_access_range specific enough here?
      if constexpr (!rng::random_access_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError,
                            "object is not subscriptable");
      }
      else {
        auto& cpp_obj = detail::get_cpp_instance<PyObj>(self);
        const auto sz = static_cast<Py_ssize_t>(cpp_obj.size());

        // support pythonic negative indexing (e.g., v[-1])
        if (idx < 0) { idx += sz; }
        JMG_ENFORCE_USING(std::out_of_range, (idx >= 0) && (idx < sz),
                          "index [", idx, "] out of range [0..", sz, "]");
        using ItemType = CppType::value_type;
        cpp_obj[idx] = detail::from_python<ItemType>(value);
        return kPySuccess;
      }
    });
  }

  static PyObject* strRepr(PyObject* self) {
    namespace rng = std::ranges;
    namespace vws = std::views;
    using namespace std::string_view_literals;
    return detail::sinking_invoke_from_python([&]() -> PyObject* {
      auto& cpp_obj = detail::get_cpp_instance<PyObj>(self);

      // NOTE: "deducing this" is not idiomatic here due to the caller
      // following python's nomenclature
      [[maybe_unused]] auto str_for = [](this auto& recurse,
                                         auto val) -> std::string {
        using ValType = DecayT<decltype(val)>;
        if constexpr (SameAsDecayedT<bool, ValType>) {
          return val ? kTrueStr : kFalseStr;
        }
        else if constexpr (ArithmeticT<ValType>) {
          return static_cast<std::string>(from(val));
        }
        else if constexpr (StringLikeT<ValType>) {
          return str_cat("'", val, "'");
        }
        else if constexpr (rng::range<ValType>) {
          auto joined =
            val | vws::transform(recurse) | vws::join_with(","sv);
          return str_cat("["sv, rng::to<std::string>(joined), "]"sv);
        }
        else {
          JMG_THROW_EXCEPTION(RuntimePythonTypeError,
                              "attempted to get the string representation for "
                              "type [",
                              type_name_for<ValType>(),
                              "] that is not representable");
        }
      };

      std::ostringstream strm;
      if constexpr (rng::range<CppType>) {
        strm << str_for(cpp_obj);
        return detail::to_python(strm.str());
      }

      // TODO(bd) what is the correct output if the container is
      // iterable and has public data members?
      strm << "{"sv;
      static constexpr auto mbrs =
        std::define_static_array(rflx::nonstatic_data_members_of(^^CppClass,
                                                                 kPublicAccess));
      [[maybe_unused]] bool first = true;
      template for (constexpr rflx::info mbr : mbrs) {
        if (first) { first = false; }
        else { strm << ","sv; }
        strm << SnakeCaseIdOwner<mbr>::c_str() << "="sv;
        auto& val = cpp_obj.[:mbr:];
        strm << str_for(val);
      }
      strm << "}"sv;

      return detail::to_python(strm.str());
    });
  }

  static PyMethodDef* allMethods() {
    const auto& methods = detail::LazyPythonMethodFactory<type>::methods;
    return const_cast<PyMethodDef*>(
      methods.data());
  }

public:

  /**
   * factory function for the reflexive wrapper class
   */
  static auto make(const std::string_view module_name) {
    static const auto class_name =
      PascalCaseIdOwner<^^CppClass>::c_str();

    // create the custom metaclass
    static PyTypeObject* metaclass_ptr = nullptr;
    if (!metaclass_ptr) {
      static PyTypeObject metaclass = {PyVarObject_HEAD_INIT(nullptr, 0)};
      // populate required fields
      static const auto tp_name =
        str_cat(module_name, ".", class_name, "Meta");
      metaclass.tp_name = tp_name.data();
      metaclass.tp_basicsize = sizeof(PythonMetaclassWrapper);
      metaclass.tp_flags = Py_TPFLAGS_DEFAULT;
      metaclass.tp_base = &PyType_Type;
      metaclass.tp_getattro = PythonReflex::getStaticDataMember;
      metaclass.tp_setattro = PythonReflex::setStaticDataMember;
      metaclass_ptr = &metaclass;
      JMG_ENFORCE(PyType_Ready(metaclass_ptr) >= 0,
                  "unable to initialize the python metaclass for class [",
                  class_name, "]");
    }

    // create the standard instance type
    static PyTypeObject* type_ptr = nullptr;
    if (!type_ptr) {
      static PyTypeObject rslt = {PyVarObject_HEAD_INIT(nullptr, 0)};
      // populate required fields
      static const auto tp_name = str_cat(module_name, ".", class_name);
      rslt.tp_name = tp_name.data();
      rslt.tp_basicsize = sizeof(PyObj);
      rslt.tp_dealloc = PythonReflex::destroy;
      rslt.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
      rslt.tp_doc = docStr().data();
      rslt.tp_methods = allMethods();
      rslt.tp_init = construct;
      rslt.tp_new = allocate;
      rslt.tp_getattro = PythonReflex::getDataMember;
      rslt.tp_setattro = PythonReflex::setDataMember;
      rslt.tp_repr = PythonReflex::strRepr;
      rslt.tp_str = PythonReflex::strRepr;
      static PySequenceMethods seq_hooks{};
      if constexpr (std::ranges::random_access_range<CppType>) {
        // only provide enable python iteration for types that support it
        rslt.tp_iter = PythonReflex::iterator;
        rslt.tp_iternext = PythonReflex::nextItem;
        // TODO(bd) is random_access_range specific enough for item handling?
        seq_hooks.sq_item = PythonReflex::sequenceItem;
        seq_hooks.sq_ass_item = PythonReflex::sequenceSetItem;
        if constexpr (rng::sized_range<CppType>) {
          seq_hooks.sq_length = PythonReflex::sequenceSz;
        }
        rslt.tp_as_sequence = &seq_hooks;
      }
      type_ptr = &rslt;
      Py_SET_TYPE(type_ptr, metaclass_ptr);
    }
    return std::make_tuple(class_name, type_ptr);
  };
};

} // namespace jmg

/**
 * macro that defines the init function for a module
 *
 * NOTE: the name of this function must match the python name of the module
 * being declared
 */
#define JMG_DECLARE_MODULE(python_module_name)                          \
  PyMODINIT_FUNC PyInit_##python_module_name(void) {                    \
    static constexpr auto name = std::string_view(#python_module_name); \
    return jmg::ModuleRegistry::lookupFactory(name).make();             \
  }
