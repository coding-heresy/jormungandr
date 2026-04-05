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

#include "jmgc_spec.h"

#include <ranges>

#include "jmg/conversion.h"
#include "jmg/file_util.h"

using namespace YAML;
using namespace jmg;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace fs = std::filesystem;
namespace j2 = jinja2;
namespace vws = std::views;

namespace
{

////////////////////////////////////////////////////////////////////////////////
// support constants
////////////////////////////////////////////////////////////////////////////////

const auto kKeyConcept = "key"s;
const auto kArithmeticConcept = "arithmetic"s;

using J2ConceptTranslations =
  Dict<string, vector<string>, "JMG IDL concept", "strong type library concept">;
const auto kJ2ConceptTranslations = J2ConceptTranslations{
  {kKeyConcept, {"st::equality_comparable"s, "st::hashable"s, "st::orderable"s}},
  {kArithmeticConcept, {"st::arithmetic"s}}};

} // namespace

namespace jmgc
{

////////////////////////////////////////////////////////////////////////////////
// support types
////////////////////////////////////////////////////////////////////////////////

using PrimitiveTypeTranslations =
  Dict<string,
       string,
       "translations from JMG IDL primitive types to C++ types",
       "JMG IDL primitive type">;

////////////////////////////////////////////////////////////////////////////////
// JmgYamlSpec implementation constants
////////////////////////////////////////////////////////////////////////////////

const JmgYamlSpec::PrimitiveTypeTranslations
  JmgYamlSpec::kPrimitiveTypeTranslations = {
    {"bool"s, "bool"s},           {"dbl"s, "double"s},
    {"flt"s, "float"s},           {"i8"s, "int8_t"s},
    {"i16"s, "int16_t"s},         {"i32"s, "int32_t"s},
    {"i64"s, "int64_t"s},         {"str"s, "std::string"s},
    {"timepoint"s, "TimePoint"s}, {"u8"s, "uint8_t"s},
    {"u16"s, "uint16_t"s},        {"u32"s, "uint32_t"s},
    {"u64"s, "uint64_t"s}};

////////////////////
// jinja2 templates

const string JmgYamlSpec::kPkgTmpl = R"(
{% import "type.tmpl" as type_tmpl -%}
{% import "object.tmpl" as obj_tmpl -%}
////////////////////////////////////////////////////////////////////////////////
// WARNING: this file is generated automatically by jmgc and should not be
// edited manually
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "jmg/safe_types.h"
#include "jmg/{{ tgt_encoding }}/{{ tgt_encoding }}.h"

{% if src_hdrs is defined -%}
{% for src_hdr in src_hdrs -%}
#include "{{ src_hdr }}"
{% endfor -%}

{% endif -%}
namespace {{ pkg_namespace }}
{

////////////////////////////////////////////////////////////////////////////////
// types
////////////////////////////////////////////////////////////////////////////////

{% for type_def in type_defs -%}
{{ type_tmpl.render_type_def(type_def) -}}
{%- if not loop.last -%}

{% endif -%}
{% endfor -%}

////////////////////////////////////////////////////////////////////////////////
// fields and objects
////////////////////////////////////////////////////////////////////////////////

{% for obj_def in obj_defs -%}
{{ obj_tmpl.render_obj(obj_def, tgt_encoding) }}
{% endfor -%}
} // namespace {{ pkg_namespace }}
)"s;

const string JmgYamlSpec::kTypeTmpl = R"(
{% macro render_type_def(type_def) -%}
{% if type_def.type == 'enum' -%}
enum class {{ type_def.name }} : {{ type_def.underlying_type }} {
{% for enumeration in type_def.values -%}
  {{ enumeration.name }} = {{ enumeration.value }}{% if not loop.last %},{% endif %}
{% endfor -%}
};
{% else -%}
{# if JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS -#}
{# using {{ type_def.name }} = jmg::SafeType< -#}
JMG_NEW_SAFE_TYPE({{ type_def.name }}, {{ type_def.type }},
{% for trait in type_def.traits -%}
  {{ trait }}{% if not loop.last %},{% endif %}
{% endfor -%}
{# if JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS -#}
{# >; -#}
);
{% endif -%}
{% endmacro -%}
)"s;

const string JmgYamlSpec::kObjTmpl = R"(
{% macro render_obj(obj_def, tgt_encoding) -%}
////////////////////
// {{ obj_def.name }}

{% for field in obj_def.fields -%}
using {{ field.name }} = {% if field.type == 'str' -%}
jmg::{{ tgt_encoding }}::StringField<{%- elif field.type == 'array' -%}
jmg::{{ tgt_encoding }}::ArrayField<{{ field.subtype }}, {% else -%}
jmg::{{ tgt_encoding }}::Field<{{ field.type }}, {% endif -%}
"{{ field.field_name }}", {{ field.required }}
{%- if field.field_id is defined %}, {{ field.field_id }}{% endif -%}
>;

{% endfor -%}
using {{ obj_def.name }} = jmg::{{ tgt_encoding }}::Object<
{%- if obj_def.src_obj is defined %}{{ obj_def.src_obj }},{% endif %}
{% for field in obj_def.fields -%}
  {{ field.name }}{% if not loop.last %},{% endif %}
{% endfor -%}
>;
{% endmacro -%}
)"s;

////////////////////////////////////////////////////////////////////////////////
// JmgYamlSpec member functions
////////////////////////////////////////////////////////////////////////////////

/**
 * translate the JMG IDL type name to the correct C++ type
 */
string_view JmgYamlSpec::translateType(const string_view jmg_idl_type) const {
  if (const auto entry = kPrimitiveTypeTranslations.find(jmg_idl_type);
      kPrimitiveTypeTranslations.end() != entry) {
    return value_of(*entry);
  }
  // no translation
  return jmg_idl_type;
}

void JmgYamlSpec::processPkg(const Node& jmg_pkg) {
  JMG_ENFORCE_USING(logic_error, !pkg_,
                    "more than 1 [package] section encountered");
  pkg_ = make_unique<PkgDef>(jmg_pkg);

  // update values for template substitution
  pkg_values_["tgt_encoding"] = encodingName();
  pkg_values_["pkg_namespace"] = jmg::get<Name>(*pkg_);
  enrichJ2Pkg(pkg_values_, *pkg_);
}

void JmgYamlSpec::processType(const Node& jmg_type) {
  // TODO(bd) enforce constraints on the type name
  auto type_def = TypeDef(jmg_type);

  // enforce valid underlying type for safe types
  const auto type_name = jmg::get<Name>(type_def);
  const auto ul_type = jmg::get<Type>(type_def);
  if (kEnum != ul_type) {
    JMG_ENFORCE(isPrimitiveTypeValid(ul_type), "invalid type [", ul_type,
                "] provided for safe type [", type_name, "]");
  }

  // update values for template substitution
  j2::ValuesMap values;
  const auto name = string(jmg::get<Name>(type_def));
  values["name"] = name;
  const auto inner_type = jmg::get<Type>(type_def);
  if (kEnum == inner_type) {
    // generate jinja2 values for enum type
    values["type"] = inner_type;
    {
      const auto ul_type = jmg::try_get<EnumUlType>(type_def);
      JMG_ENFORCE(pred(ul_type),
                  "no underlying type specified for enum named [", name, "]");
      values["underlying_type"] = translateType(*ul_type);
    }
    {
      const auto enumerations = [&] {
        const auto rslt = jmg::try_get<EnumValues>(type_def);
        JMG_ENFORCE(
          pred(rslt), "type field [", type_name,
          "] was declared as an enum but no enumeration values were provided");
        JMG_ENFORCE(
          !(rslt->empty()), "type field [", type_name,
          "] was declared as an enum but the list of enumeration was empty");
        return rslt;
      }();
      j2::ValuesList enums;
      for (const auto& enumeration : *enumerations) {
        j2::ValuesMap j2_enum;
        j2_enum["name"] = jmg::get<Name>(enumeration);
        j2_enum["value"] =
          static_cast<string>(from(jmg::get<EnumValue>(enumeration)));
        enums.push_back(std::move(j2_enum));
      }
      values["values"] = std::move(enums);
    }
  }
  else {
    // generate jinja2 values for safe type
    JMG_ENFORCE(!pred(jmg::try_get<EnumUlType>(type_def)),
                "underlying type is not valid for safe type named [", name,
                "]");
    JMG_ENFORCE(!pred(jmg::try_get<EnumValues>(type_def)),
                "enumeration values are not valid for safe type named [", name,
                "]");
    values["type"] = translateType(inner_type);
    const auto safe_concept = jmg::try_get<Concept>(type_def);
    if (safe_concept) {
      const auto& traits = kJ2ConceptTranslations.find_required(*safe_concept);
      j2::ValuesList trait_values;
      for (const auto& trait : traits) { trait_values.push_back(trait); }
      values["traits"] = std::move(trait_values);
    }
  }
  enrichJ2Type(values, type_def);
  type_names_.insert_uniq(string(type_name));
  type_def_values_.push_back(std::move(values));
}

void JmgYamlSpec::processObjFld(const string_view obj_name,
                                const Node& jmg_fld) {
  // TODO(bd) enforce constraints on the object name
  auto ptr = make_shared<ObjGrpFld>(jmg_fld);

  // enforce valid wrapped type for field
  const auto fld_name = jmg::get<Name>(*ptr);
  const auto type_name = jmg::get<Type>(*ptr);
  JMG_ENFORCE(isTypeValid(*ptr), "field [", fld_name, "] of object [", obj_name,
              "] does not contain a valid type");

  // lookup the entry for the object
  {
    auto& j2_obj = [&] -> j2::ValuesMap& {
      auto entry = obj_def_values_.find(obj_name);
      if (obj_def_values_.end() == entry) {
        JMG_ENFORCE_USING(logic_error, !type_names_.contains(obj_name),
                          "internal error: object [", obj_name,
                          "] was in the dictionary of object definitions but "
                          "not in the set of declared type names");
        type_names_.insert_uniq(string(obj_name));
        obj_names_.push_back(string(obj_name));
        auto& new_entry =
          obj_def_values_.emplace_uniq(string(obj_name), j2::ValuesMap());
        auto& obj = value_of(new_entry);
        obj["name"] = string(obj_name);
        obj["fields"] = j2::ValuesList();
        enrichJ2Obj(obj, obj_name);
        return obj;
      }
      return value_of(*entry);
    }();
    {
      j2::ValuesMap j2_fld;
      j2_fld["name"] = snakeCaseToCamelCase(fld_name);
      j2_fld["type"] = (("str"sv == type_name) || ("array"sv == type_name))
                         ? string(type_name)
                         : string(translateType(type_name));
      {
        const auto sub_type = jmg::try_get<SubType>(*ptr);
        if (sub_type) {
          JMG_ENFORCE("array"sv == type_name,
                      "subtype is not allowed for non-array field [", fld_name,
                      "]");
          j2_fld["subtype"] = string(translateType(*sub_type));
        }
        else {
          JMG_ENFORCE("array"sv != type_name,
                      "no subtype specified for array field [", fld_name, "]");
        }
      }
      if (const auto required_flag = jmg::try_get<RequiredFlag>(*ptr);
          required_flag) {
        j2_fld["required"] =
          *required_flag ? "jmg::Required"s : "jmg::Optional"s;
      }
      else {
        // default value is 'required'
        j2_fld["required"] = "jmg::Required"s;
      }
      j2_fld["field_name"] = camelCaseToSnakeCase(fld_name);
      enrichJ2Fld(j2_fld, *ptr);
      j2_obj["fields"].asList().push_back(std::move(j2_fld));
    }
  }
}

void JmgYamlSpec::emit(ostream& strm) {
  j2::ValuesMap all_values = pkg_values_;
  all_values["type_defs"] = type_def_values_;
  {
    j2::ValuesList j2_obj_defs;
    for (const auto& obj_name : obj_names_) {
      auto& j2_obj = obj_def_values_.find_required(obj_name);
      j2_obj_defs.push_back(j2_obj);
    }
    all_values["obj_defs"] = j2_obj_defs;
  }
  strm << getJ2ValueFrom(pkgTmpl().RenderAsString(all_values));
}

string JmgYamlSpec::tgtFileName() const {
  JMG_ENFORCE_USING(
    logic_error, pred(pkg_),
    "requested target file name before input file was processed");
  return str_cat(snakeCaseToCamelCase(jmg::get<Name>(*pkg_)), ".h");
}

jinja2::Template& JmgYamlSpec::pkgTmpl() {
  if (!pkg_tmpl_) {
    tmpl_store_ = [&] {
      j2::MemoryFileSystem rslt;
      rslt.AddFile("pkg.tmpl", pkgTmplData());
      rslt.AddFile("type.tmpl", typeTmplData());
      rslt.AddFile("object.tmpl", objTmplData());
      return rslt;
    }();
    tmpl_env_ = [&] {
      auto rslt = std::make_unique<j2::TemplateEnv>();
      rslt->AddFilesystemHandler(""s, tmpl_store_);
      return rslt;
    }();
    // TODO(bd) use non/std::expected wrapper to get better error message in the
    // exception if LoadTemplate fails
    pkg_tmpl_ = std::make_unique<j2::Template>(
      getJ2ValueFrom(tmpl_env_->LoadTemplate("pkg.tmpl"),
                     "loading main package jinja2 template"));
  }
  return *pkg_tmpl_;
}

bool JmgYamlSpec::isPrimitiveTypeValid(const string_view type_name) {
  return kPrimitiveTypeTranslations.contains(type_name);
}

bool JmgYamlSpec::isTypeValid(const ObjGrpFld& fld) {
  auto type_name = jmg::get<Type>(fld);
  if (kArray == type_name) {
    const auto sub_type = jmg::try_get<SubType>(fld);
    JMG_ENFORCE(pred(sub_type), "no subtype provided for array field [",
                jmg::get<Name>(fld));
    type_name = *sub_type;
  }
  return kPrimitiveTypeTranslations.contains(type_name)
         || type_names_.contains(type_name);
}

string JmgYamlSpec::pkgTmplData() const { return string(kPkgTmpl); }

string JmgYamlSpec::typeTmplData() const { return string(kTypeTmpl); }

string JmgYamlSpec::objTmplData() const { return string(kObjTmpl); }

////////////////////////////////////////////////////////////////////////////////
// JmgcYamlSpecMgr implementation
////////////////////////////////////////////////////////////////////////////////

void JmgcYamlSpecMgr::add(JmgcYamlSpecPtr&& spec) {
  specs_.emplace_back(std::move(spec));
}

void JmgcYamlSpecMgr::processPkg(const Node& jmg_pkg) {
  for (auto& spec : specs_) { spec->processPkg(jmg_pkg); }
}

void JmgcYamlSpecMgr::processType(const Node& jmg_type) {
  for (auto& spec : specs_) { spec->processType(jmg_type); }
}

void JmgcYamlSpecMgr::processObjFld(const string_view obj_name,
                                    const Node& jmg_fld) {
  for (auto& spec : specs_) { spec->processObjFld(obj_name, jmg_fld); }
}

void JmgcYamlSpecMgr::emit(ostream& strm) {
  for (auto& spec : specs_) {
    cerr << "base target output file would be [" << spec->tgtFileName()
         << "]\n";
    spec->emit(strm);
  }
}

void JmgcYamlSpecMgr::emit(const fs::path& tgt_directory) {
  const auto tgt_dir_str = tgt_directory.native();
  JMG_ENFORCE(fs::is_directory(tgt_directory),
              "attempted to write output to non-directory [", tgt_dir_str, "]");
  for (auto& spec : specs_) {
    const auto tgt_file_path = [&] {
      if (tgt_dir_str.ends_with("/")) {
        return str_cat(tgt_dir_str, spec->tgtFileName());
      }
      else { return str_cat(tgt_dir_str, "/", spec->tgtFileName()); }
    }();
    auto strm = open_file<ofstream>(tgt_file_path);
    cerr << "writing definitions to file [" << tgt_file_path << "]\n";
    spec->emit(strm);
  }
}

} // namespace jmgc
