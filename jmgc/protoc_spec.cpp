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

namespace vws = std::views;

namespace jmgc
{

std::string ProtocYamlSpec::tgtFileName() const {
  JMG_ENFORCE_USING(
    logic_error, pred(pkg_),
    "requested target file name before input file was processed");
  return str_cat(snakeCaseToCamelCase(jmg::get<Name>(*pkg_)), ".proto");
}

void ProtocYamlSpec::emit(ostream& strm) const {
  JMG_ENFORCE_USING(
    logic_error, pkg_,
    "attempted to emit spec results before processing a [package] section");

  strm << "\nsyntax = \"proto2\";\n\n";

  // emit package
  emitPkg(strm, *pkg_);

  // emit all types
  for (const auto& type_def : types_) { emitType(strm, type_def); }

  // emit all objects in the order in which they were encountered
  for (const auto& obj_name : obj_names_) {
    const auto& flds =
      find_required(obj_dict_, obj_name, "defined objects", "object name");
    emitObj(strm, obj_name, flds);
  }
}

void ProtocYamlSpec::emitPkg(ostream& strm, const jmg::PkgDef& pkg_def) const {
  const auto proto_pkg = jmg::try_get<ProtobufPackage>(pkg_def);
  JMG_ENFORCE(pred(proto_pkg), "no [protobuf_pkg] provided for package [",
              jmg::get<Name>(pkg_def), "] when generating .proto IDL output");
  strm << "package " << *proto_pkg << ";\n\n";

  if (const auto proto_imports = jmg::try_get<ProtobufImports>(pkg_def);
      proto_imports) {
    JMG_ENFORCE(!(proto_imports->empty()),
                "provided [proto_imports] section was empty");
    for (const auto proto_import : *proto_imports) {
      const auto file_path = jmg::get<File>(proto_import);
    }
  }

  // TODO(bd) add more here
}

void ProtocYamlSpec::emitEnum(ostream& strm,
                              string_view name,
                              optional<string_view> /* ul_type */,
                              const jmg::Enumerations& enumerations) const {
  strm << "enum " << snakeCaseToCamelCase(name, true /* capitalize_leading */)
       << " {\n";
  const auto enums_view =
    enumerations | vws::transform([&](const auto& enumeration) {
      auto enum_name =
        camelCaseToSnakeCase(jmg::get<Name>(enumeration), true /* app_caps */);
      const auto enum_val = jmg::get<EnumValue>(enumeration);
      return str_cat("  ", std::move(enum_name), " = ", enum_val, ";");
    });
  strm << str_join(enums_view, "\n") << "\n}\n\n";
}

void ProtocYamlSpec::emitSafeType(
  ostream& /* strm */,
  string_view name,
  string_view /* inner_type */,
  optional<string_view> /* safe_concept */) const {
  cerr << "WARN: skipping safe type definition [" << name
       << "] since safe types are not supported directly by protoc\n";
}

void ProtocYamlSpec::emitFld(ostream& strm,
                             const jmg::ObjGrpFld& fld_def) const {
  const auto fld_name = jmg::get<Name>(fld_def);
  const auto fld_type = jmg::get<Type>(fld_def);
  strm << "\n  ";
  if (kArray == fld_type) {
    strm << "repeated ";
    const auto rpt_type = jmg::try_get<SubType>(fld_def);
    JMG_ENFORCE(pred(rpt_type), "no subtype provided for repeated field [",
                fld_name, "]");
    strm << translateType(*rpt_type);
  }
  else {
    if (const auto required_flag = jmg::try_get<RequiredFlag>(fld_def);
        required_flag) {
      if (*required_flag) { strm << "required "; }
      else { strm << "optional "; }
    }
    else { strm << "required "; }
    strm << translateType(fld_type);
  }

  strm << " " << camelCaseToSnakeCase(fld_name);

  const auto fld_id = jmg::try_get<ProtobufId>(fld_def);
  JMG_ENFORCE(pred(fld_id), "no protobuf field ID provided for field [",
              fld_name, "] when generating .proto IDL output");
  strm << " = " << *fld_id;

  strm << ";\n";
}

void ProtocYamlSpec::emitObj(ostream& strm,
                             const string_view name,
                             const JmgObjGrpFlds& flds) const {
  strm << "message " << name << " {\n";
  for (const auto& fld : flds) { emitFld(strm, *fld); }
  strm << "}\n\n";
}

const Dict<string, string> ProtocYamlSpec::kTypeTranslations = {
  {"bool"s, "bool"s},
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
