/** -*- mode: c++ -*-
 *
 * Copyright (C) 2024 Brian Davis
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
#include <concepts>
#include <span>
#include <string_view>

#include "jmg/conversion.h"
#include "jmg/object.h"
#include "jmg/preprocessor.h"
#include "jmg/safe_types.h"
#include "jmg/util.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// type tag to differentiate command line parameter field types from
// other fields declared e.g. directly by jmg::FieldDef
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
struct CmdLineParamTag {};

// TODO: add support for separating named and positional parameters or
// remove this constant
static constexpr auto kPosDelimit = std::string_view("--");
} // namespace detail

/**
 * concept for command line parameters
 *
 * NOTE: using std::is_base_of_v instead of std::derived_from due to
 * the fact that CmdLineParamTag is expected to be a private base
 * class
 */
template<typename T>
concept CmdLineParamT = std::is_base_of_v<detail::CmdLineParamTag, T>;

////////////////////////////////////////////////////////////////////////////////
// command line parameter field types
////////////////////////////////////////////////////////////////////////////////

// using a macro instead of a base class cuts down on the boilerplate
#define PARAM_BOILERPLATE                                                 \
  static constexpr auto name = kName.value;                               \
  static constexpr auto desc = kDesc.value;                               \
  static_assert(name[0] != '-', "parameter name may not begin with '-'"); \
  static_assert(std::same_as<T, std::string> || !StringLikeT<T>,          \
                "std::string is the only string-like type allowed for "   \
                "command line parameters");                               \
  using type = T

/**
 * command line positional parameter
 *
 * NOTE: positional parameters are not allowed to be boolean
 */
template<NonBoolT T,
         StrLiteral kName,
         StrLiteral kDesc,
         TypeFlagT IsRequired = Required>
struct PosnParam : public FieldDef<T, kName, IsRequired>,
                   private detail::CmdLineParamTag {
  PARAM_BOILERPLATE;
};

/**
 * command line named parameter
 */
template<typename T, StrLiteral kName, StrLiteral kDesc, TypeFlagT IsRequired>
struct NamedParam : public FieldDef<T, kName, IsRequired>,
                    private detail::CmdLineParamTag {
  PARAM_BOILERPLATE;
  static_assert(!std::same_as<T, bool> || IsRequired{}(),
                "named boolean parameters must be required");
};

template<StrLiteral kName, StrLiteral kDesc>
struct NamedFlag : public NamedParam<bool, kName, kDesc, Required> {};

#undef PARAM_BOILERPLATE

////////////////////////////////////////////////////////////////////////////////
// concepts
////////////////////////////////////////////////////////////////////////////////

namespace detail
{
template<typename T>
struct IsPosnParam : std::false_type {};
template<NonBoolT T, StrLiteral kName, StrLiteral kDesc, TypeFlagT IsRequired>
struct IsPosnParam<PosnParam<T, kName, kDesc, IsRequired>> : std::true_type {};

template<typename T>
struct IsNamedParam : std::false_type {};
template<typename T, StrLiteral kName, StrLiteral kDesc, TypeFlagT IsRequired>
struct IsNamedParam<NamedParam<T, kName, kDesc, IsRequired>> : std::true_type {
};
// NOTE: a NamedFlag is a type of NamedParam
template<StrLiteral kName, StrLiteral kDesc>
struct IsNamedParam<NamedFlag<kName, kDesc>> : std::true_type {};
} // namespace detail

template<typename T>
concept PosnParamT = detail::IsPosnParam<std::remove_cvref_t<T>>{}();

template<typename T>
concept NamedParamT = detail::IsNamedParam<std::remove_cvref_t<T>>{}();

////////////////////////////////////////////////////////////////////////////////
// specific exception for command line processing errors that should
// result in usage output
////////////////////////////////////////////////////////////////////////////////

JMG_DEFINE_RUNTIME_EXCEPTION(CmdLineError);

/**
 * macro for enforcing some predicate about a command line parse by
 * throwing CmdLineError with embedded usage
 */
#define JMG_ENFORCE_CMDLINE(predicate, ...)     \
  do {                                          \
    if (JMG_UNLIKELY(!(predicate))) {           \
      throw CmdLineError(str_cat(__VA_ARGS__)); \
    }                                           \
  } while (0)

////////////////////////////////////////////////////////////////////////////////
// parsing/handling implementation
////////////////////////////////////////////////////////////////////////////////

/**
 * command line argument handling class that is configured at compile
 * time using field definitions
 */
template<CmdLineParamT... Params>
class CmdLineArgs : public ObjectDef<Params...> {
public:
  CmdLineArgs() = delete;
  CmdLineArgs(const int argc, const char* argv[]) {
    JMG_ENFORCE_USING(
      std::logic_error, argc >= 1,
      "internal error, argument vector must have at least 1 element");
    try {
      program_ = argv[0];
      const auto args = std::span(argv + 1, argc - 1);
      auto matches = std::vector<bool>(args.size());

      auto processParam = [&]<typename T>() {
        using ValueType = typename T::type;
        JMG_ENFORCE_USING(std::logic_error, args.size() == matches.size(),
                          "internal error, size of arguments span [",
                          args.size(),
                          "] does not match size of matching flags span [",
                          matches.size(), "]");
        constexpr auto param_idx = meta::find_index<ParamList, T>{}();
        const auto is_required = RequiredField<T>;
        if constexpr (NamedParamT<T>) {
          ////////////////////
          // handling of named parameters

          // function that will return true if its argument matches
          // the named parameter
          auto matchNamedParam = [](const std::string_view str) {
            return (('-' == str[0]) && (T::name == str.substr(1)));
          };

          const auto entry = std::ranges::find_if(args, matchNamedParam);
          if (entry == args.end()) {
            // parameter was not found
            if constexpr (!std::same_as<typename T::type, bool>) {
              JMG_ENFORCE_CMDLINE(!is_required,
                                  "unable to find required named argument [",
                                  T::name, "]");
            }
            // parameter is not required or parameter type was
            // boolean, no further action (because boolean/flag
            // parameter types are defaulted to 'false')
            return;
          }
          const auto arg_idx = std::distance(args.begin(), entry);
          JMG_ENFORCE_CMDLINE(!matches[arg_idx], "multiple matches for value [",
                              T::name, "]");
          matches[arg_idx] = true;

          {
            // ensure that only one instance of this named parameter
            // is present
            const auto next = std::next(entry);
            if (next != args.end()) {
              const auto check_dups =
                std::find_if(next, args.end(), matchNamedParam);
              JMG_ENFORCE_CMDLINE(check_dups == args.end(),
                                  "multiple matches for named argument [",
                                  T::name, "]");
            }
          }

          if constexpr (std::same_as<ValueType, bool>) {
            // boolean named parameters don't have an associated value
            std::get<param_idx>(values_) = true;

            return;
          }
          else {
            // other other types of named parameters will consume the next
            // entry in the argument span
            JMG_ENFORCE_CMDLINE(
              (arg_idx + 1) < args.size(), "named argument [", T::name,
              "] is the last argument and is missing its required value");
            JMG_ENFORCE_CMDLINE(
              !matches[arg_idx + 1], "value [", args[arg_idx + 1],
              "] for named argument [", T::name,
              "] was previously consumed for some other parameter");
            matches[arg_idx + 1] = true;

            captureArg<param_idx, ValueType>(args, arg_idx + 1);
          }
        }
        else {
          ////////////////////
          // handling of positional parameters

          // find first unmatched argument
          const auto entry =
            std::ranges::find_if(matches, [](const bool val) { return !val; });
          if (entry == matches.end()) {
            JMG_ENFORCE_CMDLINE(!is_required,
                                "unable to find required positional argument [",
                                T::name, "]");
            // parameter is not required, no further action
            return;
          }
          const auto arg_idx = std::distance(matches.begin(), entry);
          *entry = true;

          captureArg<param_idx, ValueType>(args, arg_idx);
        }
      };
      // TODO: find some way to get rid of the annoying .template
      // syntax here if possible
      (processParam.template operator()<Params>(), ...);

      const auto unmatched = std::ranges::find(matches, false);
      JMG_ENFORCE_CMDLINE(matches.end() == unmatched, "command line argument [",
                          args[std::distance(matches.begin(), unmatched)],
                          "] did not match any declared parameter");
    }
    // append the usage information to the error message of all
    // exceptions
    catch (const CmdLineError& exc) {
      throw CmdLineError(usage(exc.what()));
    }
    catch (const std::logic_error& exc) {
      throw std::logic_error(usage(exc.what()));
    }
    catch (const std::exception& exc) {
      throw std::runtime_error(usage(exc.what()));
    }
  }

  std::string usage() const { return usage(""); }

  /**
   * delegate for jmg::get()
   */
  template<RequiredField Param>
  typename Param::type get() const {
    constexpr auto idx = meta::find_index<ParamList, Param>{}();
    return std::get<idx>(values_);
  }

  /**
   * delegate for jmg::try_get()
   */
  template<OptionalField Param>
  typename std::optional<typename Param::type> try_get() const {
    // NOTE: non-required fields are stored in values_ already wrapped
    // in std::optional so the implementation of try_get() is the same
    // as the implementation of get() in this case
    constexpr auto idx = meta::find_index<ParamList, Param>{}();
    return std::get<idx>(values_);
  }

  /**
   * usage message to log when invalid command line parameters are
   * received or help is requested
   */
  std::string usage(const std::string_view err) const {
    std::ostringstream strm;
    if (!err.empty()) { strm << "ERROR: " << err << "\n"; }
    strm << "usage: " << program_;
    (nameOf<Params>(strm, true), ...);
    (nameOf<Params>(strm, false), ...);
    strm << "\nwhere parameters are:";
    (describe<Params>(strm), ...);
    return strm.str();
  }

private:
  enum class ScanState { Opts, ReqdPosns, OptPosns };
  using ParamList = meta::list<Params...>;
  using Values = Tuplize<meta::transform<ParamList, Optionalize>>;

  /**
   * convert an element of the argument array to the appropriate type
   * and store it in the appropriate position in the set of values
   */
  template<size_t kParamIdx, typename T>
  void captureArg(std::span<const char*>(args), size_t arg_idx) {
    if constexpr (SameAsDecayedT<std::string, T>) {
      std::get<kParamIdx>(values_) = args[arg_idx];
    }
    else if constexpr (SafeT<T>) {
      UnsafeTypeFromT<T> val = from(std::string_view(args[arg_idx]));
      std::get<kParamIdx>(values_) = T(val);
    }
    else {
      T val = from(std::string_view(args[arg_idx]));
      std::get<kParamIdx>(values_) = val;
    }
  }

  /**
   * construct the portion of the usage command line for one declared
   * parameter
   */
  template<FieldDefT Fld>
  static void nameOf(std::ostream& strm, const bool named) {
    using namespace std::string_literals;
    constexpr bool is_opt = OptionalField<Fld>;
    if (named) {
      if constexpr (NamedParamT<Fld>) {
        strm << " ";
        if (is_opt) { strm << "["; }
        strm << "-" << Fld::name;
        if constexpr (!std::same_as<typename Fld::type, bool>) {
          strm << " <" << type_name_for<typename Fld::type>() << ">";
        }
        if (is_opt) { strm << "]"; }
      }
    }
    else {
      if constexpr (PosnParamT<Fld>) {
        strm << " ";
        strm << (is_opt ? "["s : "<"s);
        strm << Fld::name << " (" << type_name_for<typename Fld::type>() << ")";
        strm << (is_opt ? "]"s : ">"s);
      }
    }
  }

  /**
   * construct the usage description for one declared parameter
   */
  template<FieldDefT Fld>
  static void describe(std::ostream& strm) {
    using Type = typename Fld::type;
    strm << "\n  ";
    if constexpr (NamedParamT<Fld>) {
      strm << "-" << Fld::name;
      if constexpr (!std::same_as<Type, bool>) {
        strm << " <" << type_name_for<Type>() << ">";
      }
      strm << ": " << Fld::desc;
    }
    else {
      strm << "<" << Fld::name << " (" << type_name_for<Type>()
           << ")>: " << Fld::desc;
    }
  }

  /**
   * metafunction that validates a single type parameter given the
   * current state of scanning the list of type parameters
   */
  template<CmdLineParamT T, bool kValidateNamed>
  static constexpr bool validateParam(ScanState& state) {
    if constexpr (NamedParamT<T>) {
      if (ScanState::Opts != state) { return !kValidateNamed; }
    }
    else if constexpr (PosnParamT<T>) {
      if constexpr (RequiredField<T>) {
        if (ScanState::Opts == state) {
          // first required positional parameter
          state = ScanState::ReqdPosns;
        }
        else if (ScanState::OptPosns == state) {
          // previously saw an optional positional parameter, invert
          // the polarity of kValidateNamed to get the correct return
          // value
          return kValidateNamed;
        }
      }
      else {
        if (ScanState::OptPosns != state) {
          // first optional positional parameter
          state = ScanState::OptPosns;
        }
      }
    }
    return true;
  }

  /**
   * metafunction that validates all type parameters in the pack for
   * correct placement of either named or positional parameters
   */
  template<bool kValidateNamed, CmdLineParamT... Ts>
  static constexpr bool validate() {
    ScanState state = ScanState::Opts;
    return (validateParam<Ts, kValidateNamed>(state) && ...);
  }

  // statically validate the correct ordering of named and positional
  // parameters in the parameter pack that the class was specialized
  // with
  static_assert(validate<true, Params...>(),
                "some named parameter was declared after the first positional "
                "parameter");
  static_assert(validate<false, Params...>(),
                "some required positional parameter was declared after the "
                "first optional positional parameter");

  std::string program_;
  Values values_;
};

/**
 * concept for command line arguments
 */
template<typename T>
concept CmdLineArgsT = TemplateSpecializationOfT<T, CmdLineArgs>;

} // namespace jmg
