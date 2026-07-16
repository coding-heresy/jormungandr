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

#include "jmg/reflection_util.h"
#include "jmg/types.h"
#include "jmg/util.h"

namespace rflx = std::meta;
namespace rng = std::ranges;
namespace vws = std::views;

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

template<ClassT T>
struct PythonObjWrapper {
  PyObject_HEAD //
    T* instance;
};

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
// Python-specific functions

/**
 * convert a c++ type to a python type
 *
 * TODO(bd) integrate with return type overloading?
 */
template<typename T>
PyObject* to_python(T&& value) {
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

/**
 * compile time-only reflection metafunction that returns the python
 * function parameter format string for a type
 *
 * TODO(bd) convert the code to strict type checking in order to avoid
 * ambiguity when choosing constructors?
 *
 * TODO(bd) figure out how to handle optional parameters
 */
template<rflx::info T>
consteval char get_python_format_char() {
  constexpr auto type_ref = T;
  if constexpr (type_ref == ^^int) { return 'i'; }
  else if constexpr (type_ref == ^^long) { return 'l'; }
  else if constexpr (type_ref == ^^float) { return 'f'; }
  else if constexpr (type_ref == ^^double) { return 'd'; }
  else if constexpr (type_ref == ^^bool) {
    return 'p'; // boolean predicate
  }
  else if constexpr ((type_ref == ^^const char*) || (type_ref == ^^char*)) {
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
  constexpr auto sz = [&]() {
    size_t rslt = 1; // always needs null terminator
    template for (constexpr auto _ : params) { ++rslt; }
    return rslt;
  }();
  std::array<char, sz> rslt;
  size_t idx = 0;
  template for (constexpr auto param : params) {
    rslt[idx++] = get_python_format_char<rflx::type_of(param)>();
  }
  rslt[idx] = '\0';
  return rslt;
}

/**
 * function template that invokes a C++ function called via a python
 * method whose arguments have already been converted into a tuple,
 * allowing exceptions to pass through under the expectation that they
 * will be handled by the caller
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

template<typename T>
consteval auto failReturn() {
  if constexpr (SameAsDecayedT<int, T>) {
    return -1;
  }
  else if constexpr (std::is_pointer_v<T>) {
    return static_cast<T>(nullptr);
  }
  else {
    JMG_NOT_EXHAUSTIVE(T, "unknown/unsupported return type");
  }
}

template<typename Fcn>
auto sinking_invoke_from_python(Fcn&& fcn) {
  using Rslt = decltype(fcn());
  try {
    return fcn();
  }
  catch (const std::invalid_argument& e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return failReturn<Rslt>();
  }
  catch (const std::out_of_range& e) {
    PyErr_SetString(PyExc_IndexError, e.what());
    return failReturn<Rslt>();
  }
  catch (const std::runtime_error& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return failReturn<Rslt>();
  }
  catch (const std::exception& e) {
    PyErr_SetString(PyExc_Exception, e.what());
    return failReturn<Rslt>();
  }
  catch (...) {
    auto err_msg = str_cat("unknown/unexpected c++ exception of type [",
                           current_exception_type_name(), "] occurred.");
    PyErr_SetString(PyExc_SystemError, err_msg.c_str());
    return failReturn<Rslt>();
  }
}

template<rflx::info Fcn>
consteval rflx::info make_fcn_params_tuple() {
  constexpr auto params = std::define_static_array(rflx::parameters_of(Fcn));
  return [&]<size_t... kIdxs>(std::index_sequence<kIdxs...>) {
    return ^^std::tuple<typename[:std::meta::type_of(params.data()[kIdxs]):]...>;
  }(std::make_index_sequence<params.size()>{});
}

template<rflx::info Fcn>
auto parse_python_args(PyObject* args) {
  // construct a format string that will allow python to convert the
  // function arguments into a tuple
  constexpr auto fmt_array =
    detail::make_python_method_params_fmt_str<Fcn>();
  const char* fmt = fmt_array.data();

  // determine the type of the tuple that holds the argument
  using ArgTpl = typename[:make_fcn_params_tuple<Fcn>():];
  using Rslt = std::optional<ArgTpl>;
  Rslt rslt{};

  // don't bother trying to parse the arguments if the sizes don't match
  if (PyTuple_Size(args) != std::tuple_size_v<ArgTpl>) {
    return rslt;
  }

  // convert the arguments provided by python into a tuple that can be
  // used to execute the member function with std::apply
  ArgTpl parsed_args;
  auto is_parsed = [&]<size_t... Is>(std::index_sequence<Is...>) {
    return PyArg_ParseTuple(args, fmt, &std::get<Is>(parsed_args)...);
  }(std::make_index_sequence<std::tuple_size_v<ArgTpl>>{});
  if (is_parsed) { rslt = std::move(parsed_args); }
  else {
    // clear the global python error state set by the failed parse to
    // allow further matches to be attempted
    PyErr_Clear();
  }
  return rslt;
}

/**
 * function template that invokes a C++ non-static member function
 * called via a python method, automatically sinking any exceptions
 * that occur and converting them to python exceptions
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

template<ClassT T>
class LazyPythonMethodFactory {
public:
  static constinit inline auto methods = []() {
    static constexpr auto mbrs =
      std::define_static_array(rflx::members_of(^^T, kPublicAccess));
    constexpr auto sz = []() {
      size_t sz = 0;
      template for (constexpr auto mbr : mbrs) {
        if constexpr (rflx::is_function(mbr)
                      && !rflx::is_constructor(mbr)
                      && !rflx::is_destructor(mbr)
                      && rflx::has_identifier(mbr)
                      && !rflx::is_static_member(mbr)) {
          ++sz;
        }
      }
      return sz;
    }();

    std::array<PyMethodDef, sz + 1> rslt{};
    size_t idx = 0;
    template for (constexpr auto mbr : mbrs) {
      if constexpr (rflx::is_function(mbr)
                    && !rflx::is_constructor(mbr) && !rflx::is_destructor(mbr)
                    && rflx::has_identifier(mbr)
                    && !rflx::is_static_member(mbr)) {
        constexpr auto name = rflx::identifier_of(mbr);
        auto method =
          reinterpret_cast<PyCFunction>(&T::template pythonMethodDispatch<mbr>);
        rslt[idx++] = PyMethodDef{.ml_name =
                                    SnakeCaseIdOwner<mbr>().c_str(),
                                  .ml_meth = method,
                                  .ml_flags = METH_VARARGS,
                                  .ml_doc = "TODO(bd) some doc string"};
      }
    }
    rslt[sz] = kMethodListTerminator;
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
template<typename Derived, const std::string_view& kDocStr>
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

    // add sentinel null entry
    staticMethods().push_back(kMethodListTerminator);

    static PyModuleDef def = {PyModuleDef_HEAD_INIT, name(),
                              docStr().data(), -1, staticMethods().data()};
    PyObject* module = PyModule_Create(&def);
    if (!module) { return nullptr; }

    for (auto& [class_name, py_object] : classes()) {
      if (PyType_Ready(py_object) < 0) {
        Py_DECREF(module);
        return nullptr;
      }

      Py_INCREF(py_object);
      PyModule_AddObject(module, class_name.data(),
                         reinterpret_cast<PyObject*>(py_object));
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
 * mixin class template that uses reflection to create a python class
 * from a C++ class
 */
template<typename Derived, typename TgtModule, const std::string_view& kDocStr>
class PythonReflex {
private:
  friend detail::LazyPythonMethodFactory<Derived>;

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
   * count the number of public non-static member functions that
   * should be registered as python methods
   */
  static constexpr size_t methodsSz() {
    size_t sz = 0;
    template for (constexpr auto mbr :
                  rflx::members_of(^^Derived, kAnyAccess)) {
      if constexpr (rflx::is_function(mbr) && rflx::is_public(mbr)
                    && !rflx::is_static_member(mbr)) {
        ++sz;
      }
    }
    return sz;
  }

  /**
   * wrapper for internal python representation of the class
   */
  using PyObj = PythonObjWrapper<Derived>;

  /**
   * memory deallocator
   */
  static void destroy(PyObject* self) {
    auto* py_obj = reinterpret_cast<PyObj*>(self);
    if (py_obj->instance) {
      delete py_obj->instance;
      py_obj->instance = nullptr;
    }
    Py_TYPE(self)->tp_free(self);
  }

  /**
   * python object initializer
   */
  static PyObject* initialize(PyTypeObject* python_type,
                              PyObject* args,
                              PyObject* kwds) {
    auto* py_obj =
      reinterpret_cast<PyObj*>(python_type->tp_alloc(python_type, 0));
    // explicitly clear the c++ instance pointer to avoid calling
    // delete on garbage memory at a later stage of initialization
    if (py_obj) {
      py_obj->instance = nullptr;
    }
    return reinterpret_cast<PyObject*>(py_obj);
  }

  /**
   * c++ object initializer/constructor
   */
  static int construct(PyObject* self, PyObject* args, PyObject* kwds) {
    if (kwds && (PyDict_Size(kwds) > 0)) {
      // TODO(bd) support keyword arguments for constructors?
      PyErr_SetString(PyExc_TypeError,
                      "keyword arguments for constructors are not yet supported");
      return -1;
    }

    auto dispatch = [&]() -> int {
      JMG_ENFORCE(!kwds || (0 == PyDict_Size(kwds)),
                  "keyword arguments for constructors are not supported");
      auto* py_obj = reinterpret_cast<PyObj*>(self);
      JMG_ENFORCE(py_obj, "python object is null");

      // prevent re-initialization memory leaks
      if (py_obj->instance) {
        delete py_obj->instance;
        py_obj->instance = nullptr;
      }

      static constexpr auto mbrs =
        std::define_static_array(rflx::members_of(^^Derived, kPublicAccess));

      template for (constexpr rflx::info mbr : mbrs) {
        // TODO(bd) support copy constructor?
        if constexpr (rflx::is_constructor(mbr)
                      && !rflx::is_copy_constructor(mbr)
                      && !rflx::is_move_constructor(mbr)
                      && !rflx::is_deleted(mbr)) {
          auto parsed_args = detail::parse_python_args<mbr>(args);
          if (parsed_args) {
            std::apply([&](auto... arg_vals) {
              py_obj->instance = new Derived(arg_vals...);
            }, *parsed_args);
            return 0;
          }
        }
      }
      PyErr_SetString(PyExc_TypeError,
                      "no matching C++ constructor signature found.");
      return -1;
    };
    return detail::sinking_invoke_from_python(dispatch);
  }

  /**
   * generic static member function that implements calling a non-static public
   * member function of the derived class from python
   */
  template<rflx::info MbrFcn>
  static PyObject* pythonMethodDispatch(PyObject* self, PyObject* args) {
    auto* py_obj = reinterpret_cast<PyObj*>(self);
    if (!py_obj) {
      PyErr_SetString(PyExc_RuntimeError, "python object is null");
      return nullptr;
    }
    auto* cpp_obj = py_obj->instance;
    if (!cpp_obj) {
      PyErr_SetString(PyExc_RuntimeError, "c++ instance is null");
      return nullptr;
    }

    return detail::sinking_invoke_from_python<MbrFcn>(cpp_obj, args);
  }

  static PyMethodDef* nonStaticMethods() {
    return const_cast<PyMethodDef*>(
      detail::LazyPythonMethodFactory<Derived>::methods.data());
  }

  /**
   * static registration of the class with the target module
   */
  static inline bool PY_is_created_ = []() {
    static const auto class_name =
      PascalCaseIdOwner<^^Derived>::c_str();
    static const auto tp_name = str_cat(TgtModule::name(), ".", class_name);

    static auto py_type = [&]() -> PyTypeObject {
      PyTypeObject rslt = {PyVarObject_HEAD_INIT(nullptr, 0)};
      // populate required fields
      rslt.tp_name = tp_name.data();
      rslt.tp_basicsize = sizeof(PyObj);
      rslt.tp_dealloc = PythonReflex::destroy;
      rslt.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
      rslt.tp_doc = docStr().data();
      rslt.tp_methods = PythonReflex::nonStaticMethods();
      rslt.tp_init = PythonReflex::construct;
#if 1
      rslt.tp_new = PythonReflex::initialize;
#else
      rslt.tp_new = PyType_GenericNew;
#endif
      return rslt;
    }();

    TgtModule::classes().push_back({std::string_view(class_name), &py_type});
    return true;
  }();

  template<bool&>
  struct ForceCreation {};
  inline static ForceCreation<PY_is_created_> PY_force_creation_;
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
