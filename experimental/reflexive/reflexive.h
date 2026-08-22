// clang-format Language: Cpp
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
 * framework for using c++26 static reflection to automatically
 * generate python interfaces for c++ code without using multi-stage
 * builds
 *
 * A note on terminology: due to the need for careful type handling
 * when passing python function arguments down to c++ functions and
 * the fact that this framework is focused on calling c++ from python,
 * the term "arg" or "argument" will be used to denote a type that
 * corresponds to the type that must be used when calling
 * PyArg_ParseTuple on a python arguments object, while the term
 * "param" or "parameter" will refer to the type that must be used in
 * a `std::tuple` that will be passed to `std::apply` to invoke a c++
 * function. As an example of why this is important, a c++ function
 * taking a parameter of type `std::string` must use `const char*` in
 * the call to PyArg_ParseTuple, and that pointer must be converted to
 * a `std::string` in a separate tuple before it can be used to call
 * the associated c++ function.
 *
 * TODO(bd) sort out whether to support multiple versions of python or
 * only the one that the library was built with
 *
 * TODO(bd) use PyErr_Occurred for all error handling?
 */

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

#include "jmg/conversion.h"
#include "jmg/python_object.h"
#include "jmg/python_util.h"
#include "jmg/reflection_util.h"
#include "jmg/types.h"
#include "jmg/util.h"

namespace rflx = std::meta;
namespace rng = std::ranges;
namespace vws = std::views;

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
                  "unable to convert python string to c++ string when "     \
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

namespace jmg::python
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
constexpr auto kTrueStr = std::string_view("True");
constexpr auto kFalseStr = std::string_view("False");

////////////////////
// type wrappers

/**
 * wrapper class for a C++ class used by python
 */
template<ClassT T>
struct PythonObjWrapper {
  PyObject_HEAD alignas(alignof(std::optional<T>)) std::optional<T> instance;
  using Self = PythonObjWrapper<T>;

  static void destroy(PyObject* self) {
    auto* py_obj = reinterpret_cast<Self*>(self);
    py_obj->instance.~optional<T>();
    Py_TYPE(self)->tp_free(self);
  }

  static PyObject* allocate(PyTypeObject* python_type,
                            PyObject* args,
                            PyObject* kwargs) {
    auto* py_obj =
      reinterpret_cast<Self*>(python_type->tp_alloc(python_type, 0));
    if (!py_obj) { return nullptr; }
    // use placement new to construct the std::optional that owns the
    // memory
    new (&(py_obj->instance)) std::optional<T>(std::nullopt);
    return reinterpret_cast<PyObject*>(py_obj);
  }
};

/**
 * wrapper class that supports iteration over C++ objects using
 * standard python constructs
 */
template<std::ranges::random_access_range Container>
struct PythonItrState {
  PyObject_HEAD using value_type = std::ranges::range_value_t<Container>;
  using iterator = std::ranges::iterator_t<Container>;
  PyObject* iterable;
  iterator current;
  iterator end;
  PythonItrState(PyObject* i, iterator&& c, iterator&& e)
    : iterable(i), current(c), end(e) {
    Py_INCREF(iterable);
  }
  ~PythonItrState() { Py_XDECREF(iterable); }
};

/**
 * metaclass associated with a class used to access static member
 * functions and data members
 */
struct PythonMetaclassWrapper {
  PyHeapTypeObject base;
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
 * get a pointer to the C++ type wrapped in a python object
 *
 * TODO(bd) modify this to return a non-const ref
 */
template<typename T>
decltype(auto) getCppInstance(
  PyObject* self,
  const std::source_location location = std::source_location::current()) {
  JMG_ENFORCE(self, "python object is null");
  auto* py_obj = reinterpret_cast<T*>(self);
  JMG_ENFORCE_USING(RuntimePythonTypeError, py_obj->instance,
                    "C++ instance was not properly initialized");
  return *(py_obj->instance);
}

/**
 * return a value indicating failure using a type that is appropriate
 * for the function that is failing
 */
template<typename T>
consteval auto failReturn() {
  if constexpr (SameAsDecayedT<int, T>) { return kPyErr; }
  else if constexpr (SameAsDecayedT<Py_ssize_t, T>) {
    return static_cast<Py_ssize_t>(0);
  }
  else if constexpr (std::is_pointer_v<T>) { return static_cast<T>(nullptr); }
  else { JMG_NOT_EXHAUSTIVE(T, "unknown/unsupported return type"); }
}

/**
 * invoke any C++ function called from python, returning the result
 * (if any) in a form usable by python and automatically converting
 * any C++ exceptions that occur to python exceptions
 */
template<typename Fcn>
auto sinkingInvoke(Fcn&& fcn) {
  using Rslt = decltype(fcn());
  try {
    return fcn();
  }
  catch (const RuntimePythonErrorNoCppMsg& e) {
    // no need to set a python error string
    return failReturn<Rslt>();
  }
  catch (const RuntimePythonTypeError& e) {
    PyErr_SetString(PyExc_TypeError, e.what());
    return failReturn<Rslt>();
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

template<rflx::info FcnMeta>
class FcnInvoker {
  // reflection metadata for the parameters
  static constexpr auto kParamsMeta =
    std::define_static_array(rflx::parameters_of(FcnMeta));
  // total number of parameters
  static constexpr auto kParamsSz = kParamsMeta.size();
  // total number of non-default parameters
  static constexpr auto kNonDefaultParamsSz = []() consteval -> size_t {
    size_t counter = 0;
    for (auto param : kParamsMeta) {
      if (std::meta::has_default_argument(param)) { break; }
      ++counter;
    }
    return counter;
  }();
  // total number of default parameters
  static constexpr auto kDefaultParamsSz = kParamsSz - kNonDefaultParamsSz;

  /**
   * return the python method argument format string for a type
   *
   * TODO(bd) handle std::string and std::string_view arguments using
   * 's#' format that supports zero copy for std::string_view and
   * avoids an error for std::string if the python string contains one
   * or more NULL bytes
   *
   * TODO(bd) handle user-defined types
   */
  template<typename T>
  static consteval char fmt() {
    if constexpr (SameAsDecayedT<int, T>) { return 'i'; }
    else if constexpr (SameAsDecayedT<long, T>) { return 'l'; }
    else if constexpr (SameAsDecayedT<float, T>) { return 'f'; }
    else if constexpr (SameAsDecayedT<double, T>) { return 'd'; }
    else if constexpr (SameAsDecayedT<bool, T>) {
      return 'p'; // boolean predicate
    }
    else if constexpr (StringLikeT<T>) { return 's'; }
    JMG_THROW_EXCEPTION(std::runtime_error,
                        "encountered unexpected/unsupported C++ type when "
                        "getting python type format character");
  }

  // format string used for python parsing
  static constexpr auto kArgsFmtStr = []() consteval {
    std::array<char, kNonDefaultParamsSz + (2 * kDefaultParamsSz) + 1> rslt{};
    size_t idx = 0;
    template for (constexpr auto param : kParamsMeta) {
      using ParamType = typename[:rflx::type_of(param):];
      // all defaulted params must be preceded by '| so that
      // PyArg_ParseTupleAndKeywords can parse them correctly
      if (rflx::has_default_argument(param)) { rslt[idx++] = '|'; }
      rslt[idx++] = fmt<ParamType>();
    }
    rslt[idx] = '\0';
    return rslt;
  }();

  // keyword argument list used for python parsing
  static constexpr auto kKwargNames = []() consteval {
    std::array<const char*, kParamsSz + 1> rslt{};
    size_t idx = 0;
    template for (constexpr auto param : kParamsMeta) {
      if constexpr (rflx::has_identifier(param)) {
        rslt[idx++] = rflx::identifier_of(param).data();
      }
      else {
        // TODO(bd) investigate this case further?
        // NOTE: this covers the case where the function signature
        // declaration contains parameters with no names e.g. due to
        // backward compatibility requirements, seems like python will
        // need to provide them as positional arguments even though
        // the code likely doesn't use them
        rslt[idx++] = "";
      }
    }
    return rslt;
  }();

  using ParamsTpl = FcnParamsTupleForFcnMetaT<FcnMeta>;

  /**
   * special handling of tuple storage for non-primitive types
   */
  template<typename T>
  struct TgtStorageType {
    // TODO(bd) add more types
    using type = std::conditional_t<StdStringLikeT<T>, const char*, T>;
  };

  /**
   * alias for special storage type
   */
  template<typename T>
  using TgtStorageTypeT = typename TgtStorageType<T>::type;

  /**
   * dummy function that is never called and is only used as a
   * convenient way to generate the tuple type corresponding the
   * positional arguments of a c++ function converted to a form that
   * can be used with PyArg_ParseTuple
   *
   * NOTE: `const char*` must be used in place of `std::string` here
   * to avoid memory corruption
   *
   * TODO(bd) handle non-primitive types other than std::string?
   */
  static consteval rflx::info meta_make_arg_tuple() {
    return [&]<size_t... kIdxs>(std::index_sequence<kIdxs...>) {
      return ^^std::tuple<
        TgtStorageTypeT<typename[:rflx::type_of(kParamsMeta.data()[kIdxs]):]>...>;
    }(std::make_index_sequence<kParamsSz>{});
  }

  /**
   * alias for tuple-ized arguments type
   */
  using ArgsTpl = typename[:meta_make_arg_tuple():];

  /**
   * transform a single python argument type into the corresponding
   * c++ parameter type
   */
  template<typename Tgt, typename Src>
  static Tgt xform(const Src src) {
    if constexpr (ArithmeticT<Tgt>) {
      static_assert(ArithmeticT<Src>,
                    "attempted to transform non-arithmetic into arithmetic");
      return Tgt(src);
    }
    if constexpr (BoolT<Tgt>) {
      static_assert(BoolT<Src>, "attempted to transform non-bool into bool");
      return src;
    }
    if constexpr (StringLikeT<Tgt>) {
      // NOTE: the type of a python string argument should always be
      // parsed as `const char*`
      static_assert(CStyleStringT<Src>,
                    "attempted to transform non-string into string");
      return Tgt(src);
    }
    else {
      JMG_THROW_EXCEPTION(std::logic_error,
                          "unsupported transformation target type [",
                          type_name_for<Tgt>(), "]");
    }
  }

  /**
   * transform the tuple of parsed python arguments into the tuple
   * containing the c++ function parameters
   */
  static constexpr ParamsTpl xformTpl(const ArgsTpl& src) {
    return [&]<size_t... kIdxs>(std::index_sequence<kIdxs...>) {
      return ParamsTpl{
        xform<std::tuple_element_t<kIdxs, ParamsTpl>>(std::get<kIdxs>(src))...};
    }(std::make_index_sequence<kParamsSz>{});
  }

  /**
   * apply the function identified by the static reflection non-type
   * template parameter of the owning class to the argument tuple
   * constructed from python method arguments
   */
  template<TupleT ArgsTpl>
  static PyObject* applyFcn(ArgsTpl args_tpl) {
    auto fcn = &[:FcnMeta:];
    if constexpr (rflx::return_type_of(FcnMeta) == ^^void) {
      std::apply(fcn, args_tpl);
      Py_RETURN_NONE;
    }
    else { return PythonObject(std::apply(fcn, args_tpl)).release(); }
  }

public:
  /**
   * invoke a C++ function using arguments provided by python
   */
  template<typename T = std::monostate, bool kIsConstructor = false>
  // TODO(bd) constrain T to be a class type or std::monostate
  static auto invoke(PyObject* args,
                     PyObject* kwargs,
                     std::optional<T>* instance = nullptr) {
    // check for constructors, which require special handling for
    // overloads

    // initial sanity check of python arguments
    if constexpr (kIsConstructor) {
      if (kwargs && !PyDict_Check(kwargs)) {
        PyErr_SetString(PyExc_TypeError,
                        "keyword arguments object was not a valid dictionary");
        return kPyErr;
      }
      if (!instance) {
        PyErr_SetString(PyExc_TypeError,
                        "no target instance was provided for constructor");
        return kPyErr;
      }
    }
    else {
      if (kwargs) {
        JMG_ENFORCE_USING(
          RuntimePythonTypeError, PyDict_Check(kwargs),
          "keyword arguments object was not a valid dictionary");
      }
    }

    ArgsTpl parsed_args{};
    const auto is_parsing_successful = std::apply(
      [&](auto&... elements) {
        return PyArg_ParseTupleAndKeywords(
          args, kwargs, kArgsFmtStr.data(),
          const_cast<char**>(kKwargNames.data()), (&elements)...);
      },
      parsed_args);

    if constexpr (kIsConstructor) {
      if (!is_parsing_successful) { return kPyErr; }
      try {
        std::apply(
          [&](auto&&... elements) {
            instance->emplace(std::forward<decltype(elements)>(elements)...);
          },
          std::move(parsed_args));
        // clear any previously set python exception
        PyErr_Clear();
        return kPySuccess;
      }
      catch (const std::exception& e) {
        const auto err_msg =
          str_cat("caught exception when constructing object: ", e.what());
        PyErr_SetString(PyExc_TypeError, err_msg.c_str());
      }
      catch (...) {
        const auto err_msg =
          str_cat("caught unexpected exception type [",
                  current_exception_type_name(), "] when constructing object");
        PyErr_SetString(PyExc_TypeError, err_msg.c_str());
      }
      return kPyErr;
    }
    else {
      JMG_ENFORCE_PYTHON_SUCCESS(is_parsing_successful);
      if constexpr (SameAsDecayedT<std::monostate, T>) {
        // no c++ instance was provided, this is a static member
        // function or free subprogram call
        JMG_ENFORCE_USING(RuntimePythonTypeError, !instance,
                          "a c++ object was provided when attempting to "
                          "execute a c++ static member function");
        return applyFcn(std::move(parsed_args));
      }
      else {
        JMG_ENFORCE_USING(RuntimePythonTypeError,
                          instance && instance->has_value(),
                          "no c++ object was provided when attempting to "
                          "execute a c++ member function");
        // c++ instance was provided, this is a member function call
        auto& cpp_obj = instance->value();
        auto mbr_fcn_args =
          std::tuple_cat(std::make_tuple(&cpp_obj), std::move(parsed_args));
        return applyFcn(std::move(mbr_fcn_args));
      }
    }
  }

  /**
   * construct a C++ object using arguments provided by python
   */
  template<typename T>
  static auto construct(PyObject* args,
                        PyObject* kwargs,
                        std::optional<T>& instance) {
    // delegate to invoke()
    return invoke<T, true /* kIsConstructor */>(args, kwargs, &instance);
  }
};

/**
 * return true if the parameter is a public member function, false otherwise
 */
template<rflx::info Mbr>
consteval bool is_public_member_function() {
  if constexpr (rflx::is_function(Mbr) && !rflx::is_constructor(Mbr)
                && !rflx::is_destructor(Mbr) && rflx::has_identifier(Mbr)) {
    return true;
  }
  else { return false; }
}

#define JMG_RECAST_AS_PYCFCN(method) \
  reinterpret_cast<PyCFunction>(reinterpret_cast<void (*)()>(method))

#define JMG_DEF_PY_METHOD(method, flags)               \
  PyMethodDef{.ml_name = snake_case_name,              \
              .ml_meth = JMG_RECAST_AS_PYCFCN(method), \
              .ml_flags = (flags),                     \
              .ml_doc = "TODO(bd) some doc string"}

/**
 * lazily generate wrappers that will allow C++ member functions to be
 * called from python
 */
template<ClassT T>
class LazyPythonMethodFactory {
public:
  static const inline auto methods = []() {
    using CppClass = typename T::CppType;
    static constexpr auto mbrs =
      std::define_static_array(rflx::members_of(^^CppClass, kPublicAccess));

    std::array<PyMethodDef, mbrs.size() + 1> rslt{};
    size_t idx = 0;
    template for (constexpr auto mbr : mbrs) {
      if constexpr (is_public_member_function<mbr>()) {
        const auto* snake_case_name = SnakeCaseIdOwner<mbr>().c_str();
        if constexpr (!rflx::is_static_member(mbr)) {
          rslt[idx++] = JMG_DEF_PY_METHOD(&T::template callMemberFcn<mbr>,
                                          METH_VARARGS | METH_KEYWORDS);
        }
        else {
          rslt[idx++] =
            JMG_DEF_PY_METHOD(&T::template callStaticMemberFcn<mbr>,
                              METH_VARARGS | METH_KEYWORDS | METH_STATIC);
        }
      }
    }
    rslt[mbrs.size()] = kMethodListTerminator;
    return rslt;
  }();
};

#undef JMG_DEF_PY_METHOD

#undef JMG_RECAST_AS_PYCFCN

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
 *
 * TODO(bd) prevent iterator types from being wrapped since they are
 * handled explicitly by the wrapper generated for a container
 */
template<typename Derived, const std::string_view& kDocStr, typename... Wraps>
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

    static PyModuleDef def = {PyModuleDef_HEAD_INIT, name(), docStr().data(),
                              -1, staticMethods().data()};
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
    PyObject* make() override { return PythonModule::make(); }
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
  using PyObj = PythonObjWrapper<CppType>;

  /**
   * c++ object initializer/constructor
   */
  static int construct(PyObject* self, PyObject* args, PyObject* kwargs) {
    return detail::sinkingInvoke([&]() -> int {
      auto* py_obj = reinterpret_cast<PyObj*>(self);
      JMG_ENFORCE_USING(std::invalid_argument, py_obj, "python object is null");
      auto& tgt = py_obj->instance;
      using Tgt = DecayT<decltype(*tgt)>;

      static constexpr auto mbrs =
        std::define_static_array(rflx::members_of(^^CppClass, kPublicAccess));

      // compile-time unrolled loop over the constructors, invoking
      // each one until one succeeds or the list is exhausted
      template for (constexpr rflx::info mbr : mbrs) {
        // TODO(bd) support copy constructor?
        if constexpr (rflx::is_constructor(mbr)
                      && !rflx::is_copy_constructor(mbr)
                      && !rflx::is_move_constructor(mbr)
                      && !rflx::is_deleted(mbr)) {
          using Invoker = detail::FcnInvoker<mbr>;
          const auto rslt = Invoker::template construct<Tgt>(args, kwargs, tgt);
          if (kPySuccess == rslt) { return kPySuccess; }
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
  static PyObject* callMemberFcn(PyObject* self,
                                 PyObject* args,
                                 PyObject* kwargs) {
    return detail::sinkingInvoke([&]() -> PyObject* {
      JMG_ENFORCE(self, "python object is null");
      auto* py_obj = reinterpret_cast<PyObj*>(self);
      using Invoker = detail::FcnInvoker<MbrFcn>;
      return Invoker::template invoke<CppClass>(args, kwargs,
                                                &(py_obj->instance));
    });
  }

  /**
   * generic static member function that implements calling a non-static public
   * member function of the derived class from python
   */
  template<rflx::info MbrFcn>
  static PyObject* callStaticMemberFcn(PyObject* self,
                                       PyObject* args,
                                       PyObject* kwargs) {
    return detail::sinkingInvoke([&]() -> PyObject* {
      using Invoker = detail::FcnInvoker<MbrFcn>;
      return Invoker::template invoke<std::monostate>(args, kwargs);
    });
  }

  /**
   * static member function that implements retrieval of C++ public
   * data members as python attributes
   */
  static PyObject* getDataMember(PyObject* self, PyObject* name) {
    return detail::sinkingInvoke([&]() -> PyObject* {
      auto& cpp_obj = detail::getCppInstance<PyObj>(self);

      // TODO(bd) figure out why attempting to factor out a function
      // to get a string_view for the attribute name results in the
      // compiler believing that the variable used to store the name
      // is not used
      const char* raw_attr_name = PyUnicode_AsUTF8(name);
      if (!raw_attr_name) { return nullptr; }
      std::string_view attr_name(raw_attr_name);

      static constexpr auto mbrs = std::define_static_array(
        rflx::nonstatic_data_members_of(^^CppClass, kPublicAccess));

      template for (constexpr rflx::info mbr : mbrs) {
        if (std::string_view(SnakeCaseIdOwner<mbr>::c_str()) == attr_name) {
          // TODO(bd) figure out why 2 lines are necessary here
          auto rslt = PythonObject(cpp_obj.[:mbr:]);
          return std::move(rslt).release();
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
    return detail::sinkingInvoke([&]() -> int {
      if (!value) {
        PyErr_SetString(PyExc_TypeError,
                        "illegal attempt to delete an attribute of a wrapped "
                        "C++ class");
        return kPyErr;
      }
      auto& cpp_obj = detail::getCppInstance<PyObj>(self);

      // TODO(bd) figure out why attempting to factor out a function
      // to get a string_view for the attribute name results in the
      // compiler believing that the variable used to store the name
      // is not used
      const auto name_obj = PythonObject(name);
      const auto attr_name = name_obj.as<std::string_view>();
      JMG_ENFORCE_USING(RuntimePythonTypeError, !attr_name.empty(),
                        "encountered empty attribute name from python when "
                        "setting c++ data member");
      const auto val_obj = PythonObject(value);

      static constexpr auto mbrs = std::define_static_array(
        rflx::nonstatic_data_members_of(^^CppClass, kPublicAccess));

      template for (constexpr rflx::info mbr : mbrs) {
        if (std::string_view(SnakeCaseIdOwner<mbr>::c_str()) == attr_name) {
          using MbrType = DecayT<decltype(cpp_obj.[:mbr:])>;
          cpp_obj.[:mbr:] = val_obj.as<MbrType>();
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
    return detail::sinkingInvoke([&]() -> PyObject* {
      // TODO(bd) figure out why attempting to factor out a function
      // to get a string_view for the attribute name results in the
      // compiler believing that the variable used to store the name
      // is not used
      const auto name_obj = PythonObject(name);
      const auto attr_name = name_obj.as<std::string_view>();
      JMG_ENFORCE_USING(RuntimePythonTypeError, !attr_name.empty(),
                        "encountered empty attribute name from python when "
                        "getting a static c++ data member");

      // TODO(bd) create a dictionary of static data members at static
      // initialization time
      static constexpr auto mbrs =
        std::define_static_array(rflx::static_data_members_of(^^CppClass,
                                                              kPublicAccess));

      template for (constexpr rflx::info mbr : mbrs) {
        if (SnakeCaseIdOwner<mbr>::c_str() == attr_name) {
          // TODO(bd) figure out why 2 lines are necessary here
          auto rslt = PythonObject([:mbr:]);
          return std::move(rslt).release();
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
    return detail::sinkingInvoke([&]() -> int {
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
      const auto name_obj = PythonObject(name);
      const auto attr_name = name_obj.as<std::string_view>();
      JMG_ENFORCE_USING(RuntimePythonTypeError, !attr_name.empty(),
                        "encountered empty attribute name from python when "
                        "setting a static c++ data member");
      const auto val_obj = PythonObject(value);

      static constexpr auto mbrs =
        std::define_static_array(rflx::static_data_members_of(^^CppClass,
                                                              kPublicAccess));

      // TODO(bd) create a dictionary of static data members at static
      // initialization time
      template for (constexpr rflx::info mbr : mbrs) {
        if (SnakeCaseIdOwner<mbr>::c_str() == attr_name) {
          using MbrType = DecayT<decltype([:mbr:])>;
          [:mbr:] = val_obj.as<MbrType>();
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
    return detail::sinkingInvoke([&]() -> PyObject* {
      if constexpr (!rng::random_access_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError, "object is not iterable");
      }
      else {
        using ItrState = PythonItrState<CppType>;
        using ItrWrapper = PythonObjWrapper<ItrState>;
        auto& cpp_obj = detail::getCppInstance<PyObj>(self);
        auto* itr_obj =
          reinterpret_cast<ItrWrapper*>(PyType_GenericAlloc(itrProxy(), 0));
        JMG_ENFORCE_PYTHON_SUCCESS(itr_obj);
        new (&(itr_obj->instance)) std::optional<ItrState>(std::nullopt);
        itr_obj->instance.emplace(self, rng::begin(cpp_obj), rng::end(cpp_obj));
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
    return detail::sinkingInvoke([&]() -> PyObject* {
      if constexpr (!rng::random_access_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError, "object is not iterable");
      }
      else {
        using ItrState = PythonItrState<CppType>;
        using ItrWrapper = PythonObjWrapper<ItrState>;
        auto& itr_state = detail::getCppInstance<ItrWrapper>(self);
        if (itr_state.end == itr_state.current) { return nullptr; }
        PyObject* item = PythonObject(*(itr_state.current)).release();
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
    return detail::sinkingInvoke([&]() -> Py_ssize_t {
      if constexpr (!rng::sized_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError, "object has no len()");
      }
      else {
        auto& cpp_obj = detail::getCppInstance<PyObj>(self);
        return cpp_obj.size();
      }
    });
  }

  static PyObject* sequenceItem(PyObject* self, Py_ssize_t idx) {
    namespace rng = std::ranges;
    return detail::sinkingInvoke([&]() -> PyObject* {
      // TODO(bd) is random_access_range specific enough here?
      if constexpr (!rng::random_access_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError,
                            "object is not subscriptable");
      }
      else {
        auto& cpp_obj = detail::getCppInstance<PyObj>(self);
        const auto sz = static_cast<Py_ssize_t>(cpp_obj.size());

        // support pythonic negative indexing (e.g., v[-1])
        if (idx < 0) { idx += sz; }
        JMG_ENFORCE_USING(std::out_of_range, (idx >= 0) && (idx < sz),
                          "index [", idx, "] out of range [0..", sz, "]");
        return PythonObject(cpp_obj[idx]).release();
      }
    });
  }

  static int sequenceSetItem(PyObject* self, Py_ssize_t idx, PyObject* value) {
    namespace rng = std::ranges;
    return detail::sinkingInvoke([&]() -> int {
      // TODO(bd) is random_access_range specific enough here?
      if constexpr (!rng::random_access_range<CppType>) {
        // NOTE: should never happen
        JMG_THROW_EXCEPTION(RuntimePythonTypeError,
                            "object is not subscriptable");
      }
      else {
        auto& cpp_obj = detail::getCppInstance<PyObj>(self);
        const auto sz = static_cast<Py_ssize_t>(cpp_obj.size());

        // support pythonic negative indexing (e.g., v[-1])
        if (idx < 0) { idx += sz; }
        JMG_ENFORCE_USING(std::out_of_range, (idx >= 0) && (idx < sz),
                          "index [", idx, "] out of range [0..", sz, "]");
        const auto py_value = PythonObject(value);
        using ItemType = CppType::value_type;
        cpp_obj[idx] = py_value.as<ItemType>();
        return kPySuccess;
      }
    });
  }

  static PyObject* strRepr(PyObject* self) {
    namespace rng = std::ranges;
    namespace vws = std::views;
    using namespace std::string_view_literals;
    return detail::sinkingInvoke([&]() -> PyObject* {
      auto& cpp_obj = detail::getCppInstance<PyObj>(self);

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
          auto joined = val | vws::transform(recurse) | vws::join_with(","sv);
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
        return PythonObject(strm.str()).release();
      }

      // TODO(bd) what is the correct output if the container is
      // iterable and has public data members?
      strm << "{"sv;
      static constexpr auto mbrs = std::define_static_array(
        rflx::nonstatic_data_members_of(^^CppClass, kPublicAccess));
      [[maybe_unused]] bool first = true;
      template for (constexpr rflx::info mbr : mbrs) {
        if (first) { first = false; }
        else { strm << ","sv; }
        strm << SnakeCaseIdOwner<mbr>::c_str() << "="sv;
        auto& val = cpp_obj.[:mbr:];
        strm << str_for(val);
      }
      strm << "}"sv;

      return PythonObject(strm.str()).release();
    });
  }

  static PyMethodDef* allMethods() {
    const auto& methods = detail::LazyPythonMethodFactory<type>::methods;
    return const_cast<PyMethodDef*>(methods.data());
  }

  /**
   * holder for a pointer to an iterator state wrapper object if one
   * is created along with the main c++ wrapper object
   */
  static PyTypeObject*& itrProxy() {
    static PyTypeObject* instance = nullptr;
    return instance;
  }

public:
  /**
   * factory function for the reflexive wrapper class
   */
  static auto make(const std::string_view module_name) {
    static const auto class_name = PascalCaseIdOwner<^^CppClass>::c_str();

    // create the custom metaclass
    static PyTypeObject* metaclass_ptr = nullptr;
    if (!metaclass_ptr) {
      static PyTypeObject metaclass = {PyVarObject_HEAD_INIT(nullptr, 0)};
      // populate required fields
      static const auto tp_name = str_cat(module_name, ".", class_name, "Meta");
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
    static PyTypeObject* itr_ptr = nullptr;
    if (!type_ptr) {
      static PyTypeObject rslt = {PyVarObject_HEAD_INIT(nullptr, 0)};
      // populate required fields
      static const auto tp_name = str_cat(module_name, ".", class_name);
      rslt.tp_name = tp_name.data();
      rslt.tp_basicsize = sizeof(PyObj);
      rslt.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
      rslt.tp_doc = docStr().data();
      rslt.tp_new = PyObj::allocate;
      rslt.tp_init = construct;
      rslt.tp_dealloc = PyObj::destroy;
      rslt.tp_methods = allMethods();
      rslt.tp_getattro = PythonReflex::getDataMember;
      rslt.tp_setattro = PythonReflex::setDataMember;
      rslt.tp_repr = PythonReflex::strRepr;
      rslt.tp_str = PythonReflex::strRepr;
      static PySequenceMethods seq_hooks{};
      if constexpr (std::ranges::random_access_range<CppType>) {
        rslt.tp_iter = PythonReflex::iterator;

        // also generate a python wrapper for the iterator state proxy
        using ItrState = PythonItrState<CppType>;
        using ItrWrapper = PythonObjWrapper<ItrState>;
        static PyTypeObject itr_proxy = {PyVarObject_HEAD_INIT(nullptr, 0)};
        static const auto itr_tp_name =
          str_cat(module_name, ".", class_name, ".", "iterator");
        itr_proxy.tp_name = itr_tp_name.data();
        itr_proxy.tp_basicsize = sizeof(ItrWrapper);
        // prevent iterator proxies from being subclassed on the python side
        itr_proxy.tp_flags = Py_TPFLAGS_DEFAULT;
        itr_proxy.tp_doc = "c++ iterator proxy";
        // iterator is initialized by the parent
        itr_proxy.tp_new = nullptr;
        itr_proxy.tp_init = nullptr;
        itr_proxy.tp_dealloc = ItrWrapper::destroy;
        // only provide enable python iteration for types that support it
        itr_proxy.tp_iternext = PythonReflex::nextItem;
        itr_ptr = &itr_proxy;
        JMG_ENFORCE(
          PyType_Ready(itr_ptr) >= 0,
          "unable to initialize the python iterator proxy for class [",
          class_name, "]");
        itrProxy() = itr_ptr;

        // the main object should also have sequence hooks set

        // TODO(bd) is random_access_range specific enough to ensure
        // that this actually works?
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

} // namespace jmg::python

#undef MAYBE_HANDLE_PYTHON_ERROR

/**
 * macro that defines the init function for a module
 *
 * NOTE: the name of this function must match the python name of the module
 * being declared
 */
#define JMG_DECLARE_MODULE(python_module_name)                          \
  PyMODINIT_FUNC PyInit_##python_module_name(void) {                    \
    static constexpr auto name = std::string_view(#python_module_name); \
    return jmg::python::ModuleRegistry::lookupFactory(name).make();     \
  }
