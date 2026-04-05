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

#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>
#include <yaml-cpp/yaml.h>

#include "jmg/types.h"
#include "jmg/yaml/yaml.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// Jormungandr field definitions at various levels
//
// NOTE: these fields effectively define the 'syntax' of a YAML file that
// describes a legal definition of a set of Jormungandr/JMG objects
////////////////////////////////////////////////////////////////////////////////

using Name = StringField<"name", Required>;
using File = StringField<"file", Required>;
using Type = StringField<"type", Required>;
using SubType = StringField<"subtype", Optional>;
using Concept = StringField<"concept", Optional>;
using CbeId = FieldDef<uint32_t, "cbe_id", Optional>;
using ProtobufPackage = StringField<"protobuf_package", Optional>;
using ProtobufId = FieldDef<uint32_t, "protobuf_id", Optional>;
using RequiredFlag = FieldDef<bool, "required", Optional>;

// enumeration
// TODO(bd) should EnumValue be optional?
using EnumValue = FieldDef<int64_t, "value", Required>;
using EnumUlType = StringField<"underlying_type", Optional>;
using Enumeration = yaml::Object<Name, EnumValue>;
using Enumerations = yaml::ArrayField<Enumeration>;
using EnumValues = FieldDef<Enumerations, "values", Optional>;

// attributes of a package
using ImportFile = yaml::Object<File>;
using ImportFiles = yaml::ArrayField<ImportFile>;
using Imports = FieldDef<ImportFiles, "imports", Optional>;
using ProtobufImports = FieldDef<ImportFiles, "protobuf_imports", Optional>;
using PkgDef = yaml::Object<Name, Imports, ProtobufPackage, ProtobufImports>;

// objects in the 'types' section
using TypeDef =
  yaml::Object<Name, Type, SubType, Concept, EnumUlType, EnumValues>;

// objects in the 'groups' and 'objects' sections
using ObjGrpFld =
  yaml::Object<Name, Type, SubType, CbeId, ProtobufId, RequiredFlag>;

} // namespace jmg

namespace jmgc
{

////////////////////////////////////////////////////////////////////////////////
// support code
////////////////////////////////////////////////////////////////////////////////

/**
 * function template that simplifies very annoying non/std::expected value
 * retrieval
 */
template<template<typename...> typename Expected,
         typename T,
         typename E,
         typename... DescParts>
decltype(auto) getJ2ValueFrom(Expected<T, E>&& expected,
                              DescParts... desc_parts) {
  JMG_ENFORCE(expected.has_value(),
              jmg::str_cat(std::forward<DescParts>(desc_parts)...),
              " failed: ", expected.error().ToString());
  return expected.value();
}

////////////////////////////////////////////////////////////////////////////////
// spec builder
////////////////////////////////////////////////////////////////////////////////

/**
 * pure virtual base class for specification (AKA "spec") objects that
 * generate a specific type of encoding wrapper or external IDL format
 */
class JmgcYamlSpecIfc {
public:
  /**
   * process 'package' section
   */
  virtual void processPkg(const YAML::Node& jmg_pkg) = 0;

  /**
   * process type definition
   */
  virtual void processType(const YAML::Node& jmg_type) = 0;

  /**
   * process object field definition
   */
  virtual void processObjFld(const std::string_view obj_name,
                             const YAML::Node& jmg_fld) = 0;

  /**
   * base file name to emit output to
   */
  virtual std::string tgtFileName() const = 0;

  /**
   * no more definitions to process, emit output
   *
   * TODO(bd) replace const with && to allow emission to be destructive?
   */
  virtual void emit(std::ostream& strm) const = 0;
};

using JmgcYamlSpecPtr = std::unique_ptr<JmgcYamlSpecIfc>;

/**
 * class JmgYamlSpec
 */
class JmgYamlSpec : public JmgcYamlSpecIfc {
protected:
  using JmgObjGrpFldPtr = std::shared_ptr<jmg::ObjGrpFld>;
  using JmgObjGrpFlds = std::vector<JmgObjGrpFldPtr>;
  using J2TmplPtr = std::unique_ptr<jinja2::Template>;

public:
  void processPkg(const YAML::Node& jmg_pkg) override;

  void processType(const YAML::Node& jmg_type) override;

  void processObjFld(const std::string_view obj_name,
                     const YAML::Node& jmg_fld) override;

  virtual std::string tgtFileName() const;

  virtual std::string_view encodingHeaderFileName() const { return ""; }

  void emit(std::ostream& strm) const override;

protected:
  static constexpr auto kEnum = std::string_view("enum");
  static constexpr auto kString = std::string_view("str");
  static constexpr auto kArray = std::string_view("array");
  static constexpr auto kUnion = std::string_view("union");

  using TypeDefs =
    jmg::Dict<std::string, jmg::TypeDef, "declared types", "type name">;
  using PrimitiveTypeTranslations =
    jmg::Dict<std::string,
              std::string,
              "primitive type translations",
              "JMG IDL primitive type specifier">;
  using DeclaredFields =
    jmg::Dict<std::string, JmgObjGrpFldPtr, "declared fields", "field name">;
  using DeclaredObjects =
    jmg::Dict<std::string, JmgObjGrpFlds, "declared objects", "object name">;
  using ExtraTranslations =
    jmg::Dict<std::string, std::string, "extra type translations", "declared type">;

  static const PrimitiveTypeTranslations kPrimitiveTypeTranslations;

  jinja2::Template& pkgTmpl();

  jinja2::Template& pkgTmpl() const;

  bool isPrimitiveTypeValid(std::string_view type_name);

  bool isTypeValid(const jmg::ObjGrpFld& fld);

  virtual std::string_view encodingName() const { return ""; }

  virtual std::string pkgTmplData() const;

  virtual std::string typeTmplData() const;

  virtual std::string objTmplData() const;

  /**
   * member function that enriches a package definition
   */
  virtual void enrichJ2Pkg(jinja2::ValuesMap& j2_pkg,
                           const jmg::PkgDef& pkg_def) const {
    // no enrichment by default
  }

  /**
   * member function that enriches an object definition
   */
  virtual void enrichJ2Obj(jinja2::ValuesMap& j2_obj,
                           std::string_view obj_name) const {
    // no enrichment by default
  }

  /**
   * member function that enriches a field definition
   */
  virtual void enrichJ2Fld(jinja2::ValuesMap& j2_fld,
                           const jmg::ObjGrpFld& fld_def) const {
    // no enrichment by default
  }

  /**
   * member function that enriches a type definition
   */
  virtual void enrichJ2Type(jinja2::ValuesMap& j2_type,
                            const jmg::TypeDef& type_def) const {
    // no enrichment by default
  }

  virtual std::string_view translateType(std::string_view jmg_idl_type) const;

  std::unique_ptr<jmg::PkgDef> pkg_;
  TypeDefs type_defs_;
  std::vector<std::string> obj_names_;

  ////////////////////
  // jinja2

  jinja2::MemoryFileSystem tmpl_store_;
  // jinja2::TemplateEnv contains a mutex and cannot be copied or moved
  std::unique_ptr<jinja2::TemplateEnv> tmpl_env_;
  // jinja2 template for main package is constructed lazily to facilitate
  // runtime polymorphic override of the string contents of the templates
  std::unique_ptr<jinja2::Template> pkg_tmpl_;
  jinja2::ValuesMap pkg_values_;
  jinja2::ValuesList type_def_values_;
  jmg::Dict<std::string,
            jinja2::ValuesMap,
            "JMG object name",
            "jinja2 values for object">
    obj_def_values_;

private:
  static const std::string kPkgTmpl;
  static const std::string kTypeTmpl;
  static const std::string kObjTmpl;
};

/**
 * specification object that manages other specification objects
 */
class JmgcYamlSpecMgr : public JmgcYamlSpecIfc {
public:
  /**
   * add a new spec to the set managed by the object
   */
  void add(JmgcYamlSpecPtr&& spec);

  // TODO(bd) pass input line number to support better error messages

  void processPkg(const YAML::Node& jmg_pkg) override;

  void processType(const YAML::Node& jmg_type) override;

  void processObjFld(const std::string_view obj_name,
                     const YAML::Node& jmg_fld) override;

  std::string tgtFileName() const override {
    throw std::logic_error(
      "attempted to call tgtFileName() member function on spec manager object");
  }

  void emit(std::ostream& strm) const override;

  void emit(const std::filesystem::path& tgt_directory) const;

private:
  std::vector<JmgcYamlSpecPtr> specs_;
};

} // namespace jmgc
