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

#include "protoc_spec.h"

using namespace jmg;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace j2 = jinja2;
namespace vws = std::views;

namespace jmgc
{

// TODO(bd) only import timestamp.proto if the type is used
const string ProtocYamlSpec::kProtocPkgTmpl = R"(
{% import "type.tmpl" as type_tmpl -%}
{% import "object.tmpl" as obj_tmpl -%}
syntax = "proto2";

package {{ proto_pkg }};

import "google/protobuf/timestamp.proto";

{% for type_def in type_defs -%}
{{ type_tmpl.render_type_def(type_def) -}}
{% endfor -%}

{% for obj_def in obj_defs -%}
{{ obj_tmpl.render_obj(obj_def) }}
{%- if not loop.last -%}

{% endif -%}
{%- endfor -%}
)"s;

const string ProtocYamlSpec::kProtocTypeTmpl = R"(
{% macro render_type_def(type_def) -%}
{% if type_def.type == 'enum' -%}
enum {{ type_def.name }} {
{% for enumeration in type_def.values -%}
  {{ enumeration.name }} = {{ enumeration.value }};
{% endfor -%}
}
{% endif -%}
{% endmacro -%}
)"s;

const string ProtocYamlSpec::kProtocObjTmpl = R"(
{% macro render_obj(obj_def) -%}
message {{ obj_def.name }} {
{% for field in obj_def.fields -%}
  {% if field.type == 'array' %}repeated {{ field.subtype }}
{%- else %}{{ field.required }} {{ field.type }}{% endif -%}
 {{ field.name }} = {{ field.field_id }};
{% if not loop.last -%}

{% endif -%}
{% endfor -%}
}
{% endmacro -%}
)"s;

string ProtocYamlSpec::tgtFileName() const {
  JMG_ENFORCE_USING(
    logic_error, pred(pkg_),
    "requested target file name before input file was processed");
  return str_cat(snakeCaseToCamelCase(jmg::get<Name>(*pkg_)), ".proto");
}

string ProtocYamlSpec::pkgTmplData() const { return string(kProtocPkgTmpl); }

string ProtocYamlSpec::typeTmplData() const { return string(kProtocTypeTmpl); }

string ProtocYamlSpec::objTmplData() const { return string(kProtocObjTmpl); }

void ProtocYamlSpec::enrichJ2Type(jinja2::ValuesMap& j2_type,
                                  const jmg::TypeDef& type_def) const {
  const auto inner_type = jmg::get<Type>(type_def);
  if (kEnum == inner_type) {
    {
      const auto& name = j2_type["name"].asString();
      j2_type["name"] =
        snakeCaseToCamelCase(name, true /* capitalize_leading */);
    }
    {
      auto& enumerations = j2_type["values"].asList();
      auto rewritten = make_reserved<j2::ValuesList>(enumerations.size());
      for (const auto& enumeration : enumerations) {
        auto clone = enumeration.asMap();
        // rewrite the enumeration name to all caps snake case to conform to
        // protobuf conventions/requirements
        clone["name"] =
          camelCaseToSnakeCase(clone["name"].asString(), true /* app_caps */);
        rewritten.push_back(std::move(clone));
      }
      j2_type["values"] = std::move(rewritten);
    }
  }
}

void ProtocYamlSpec::enrichJ2Fld(jinja2::ValuesMap& j2_fld,
                                 const jmg::ObjGrpFld& fld_def) const {
  const auto fld_name = jmg::get<Name>(fld_def);
  j2_fld["name"] = camelCaseToSnakeCase(fld_name);
  {
    const auto required = j2_fld["required"].asString();
    j2_fld["required"] =
      ("jmg::Required"sv == required) ? "required"s : "optional"s;
  }
  {
    const auto inner_type = j2_fld["type"].asString();
    if ("str"sv == inner_type) { j2_fld["type"] = "string"sv; }
    else if ("array"sv == inner_type) {
      const auto rpt_type = jmg::try_get<SubType>(fld_def);
      JMG_ENFORCE(pred(rpt_type), "no subtype provided for repeated field [",
                  fld_name, "]");
      j2_fld["subtype"] = string(translateType(*rpt_type));
    }
    else {
      if (const auto type_def_entry = type_defs_.find(inner_type);
          type_defs_.end() != type_def_entry) {
        // replace declared safe types with their underlying types
        const auto& type_def = value_of(*type_def_entry);
        const auto type_def_type = jmg::get<Type>(type_def);
        if (kEnum != type_def_type) {
          j2_fld["type"] = string(translateType(type_def_type));
        }
      }
    }
  }
  {
    const auto fld_id = jmg::try_get<ProtobufId>(fld_def);
    JMG_ENFORCE(pred(fld_id), "no protobuf field ID provided for field [",
                fld_name, "] when generating .proto IDL output");
    j2_fld["field_id"] = str_cat(*fld_id);
  }
}

void ProtocYamlSpec::enrichJ2Obj(jinja2::ValuesMap& j2_obj,
                                 std::string_view obj_name) const {}

void ProtocYamlSpec::enrichJ2Pkg(jinja2::ValuesMap& j2_pkg,
                                 const jmg::PkgDef& pkg_def) const {
  const auto proto_pkg = jmg::try_get<ProtobufPackage>(pkg_def);
  JMG_ENFORCE(pred(proto_pkg), "no [protobuf_pkg] provided for package [",
              jmg::get<Name>(pkg_def), "] when generating .proto IDL output");
  j2_pkg["proto_pkg"] = string(*proto_pkg);
}

const ProtocYamlSpec::ProtocTypeTranslations ProtocYamlSpec::kTypeTranslations =
  {{"bool"s, "bool"s},
   {"dbl"s, "double"s},
   {"flt"s, "float"s},
   {"i32"s, "sint32"s},
   {"i64"s, "sint64"s},
   {"str"s, "string"s},
   {"timepoint"s, "google.protobuf.Timestamp"s},
   {"u32"s, "uint32"s},
   {"u64"s, "uint64"s}};

/**
 * translate the JMG IDL type name to the correct protobuf IDL type
 */
string_view ProtocYamlSpec::translateType(const string_view jmg_idl_type) const {
  if (const auto entry = kTypeTranslations.find(jmg_idl_type);
      kTypeTranslations.end() != entry) {
    return value_of(*entry);
  }
  return jmg_idl_type;
}

} // namespace jmgc
