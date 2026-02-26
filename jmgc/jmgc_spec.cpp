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

#include "jmg/file_util.h"

using namespace YAML;
using namespace jmg;
using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace fs = std::filesystem;
namespace vws = std::views;

namespace
{

////////////////////////////////////////////////////////////////////////////////
// constants
////////////////////////////////////////////////////////////////////////////////

const auto kKeyConcept = "key"s;
const auto kArithmeticConcept = "arithmetic"s;

const auto kConceptTranslations = Dict<string, string>{
  {kKeyConcept,
   ",\n  st::equality_comparable,\n  st::hashable,\n  st::orderable\n"s},
  {kArithmeticConcept, ", st::arithmetic"s}};

} // namespace

namespace jmgc
{
const jmg::Dict<string, string> JmgYamlSpec::kPrimitiveTypeTranslations = {
  {"bool"s, "bool"s},   {"dbl"s, "double"s},      {"flt"s, "float"s},
  {"i8"s, "int8_t"s},   {"i16"s, "int16_t"s},     {"i32"s, "int32_t"s},
  {"i64"s, "int64_t"s}, {"str"s, "std::string"s}, {"timepoint"s, "TimePoint"s},
  {"u8"s, "uint8_t"s},  {"u16"s, "uint16_t"s},    {"u32"s, "uint32_t"s},
  {"u64"s, "uint64_t"s}};

/**
 * translate the JMG IDL type name to the correct C++ type
 */
string_view JmgYamlSpec::translateType(const string_view jmg_idl_type) const {
  if (const auto entry = kPrimitiveTypeTranslations.find(jmg_idl_type);
      kPrimitiveTypeTranslations.end() != entry) {
    return value_of(*entry);
  }
  if (const auto entry = extraTranslations_.find(jmg_idl_type);
      extraTranslations_.end() != entry) {
    return value_of(*entry);
  }
  // no translation
  return jmg_idl_type;
}

void JmgYamlSpec::processPkg(const Node& jmg_pkg) {
  JMG_ENFORCE_USING(logic_error, !pkg_,
                    "more than 1 [package] section encountered");
  pkg_ = make_unique<PkgDef>(jmg_pkg);
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

  // update symbol table and internal data structure
  insert_uniq("known type name", type_names_, type_name);
  types_.emplace_back(std::move(type_def));
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
  auto& flds = [&]() -> JmgObjGrpFlds& {
    auto entry = obj_dict_.find(obj_name);
    if (obj_dict_.end() == entry) {
      // add new entry for this object
      JmgObjGrpFlds new_flds;
      entry = emplace_uniq("object dictionary", obj_dict_, string(obj_name),
                           new_flds);
      obj_names_.push_back(string(obj_name));

      // add this object to the list of known types
      insert_uniq("known type name", type_names_, string(obj_name));
    }
    return value_of(*entry);
  }();

  // add the field to the list of fields in the object
  flds.emplace_back(std::move(ptr));
}

void JmgYamlSpec::emit(ostream& strm) const {
  JMG_ENFORCE_USING(
    logic_error, pkg_,
    "attempted to emit spec results before processing a [package] section");

  strm << "////////////////////////////////////////////////////////////////////"
          "////////////\n";
  strm << "// WARNING: this file is generated automatically by jmgc and "
          "should not be\n";
  strm << "// edited manually\n";
  strm << "////////////////////////////////////////////////////////////////////"
          "////////////\n";
  strm << "#pragma once\n\n";
  strm << "#include \"jmg/safe_types.h\"\n";
  if (const auto encoding_hdr = encodingHeaderFileName();
      !encoding_hdr.empty()) {
    strm << "#include \"" << encoding_hdr << "\"\n";
  }
  strm << "\n";

  // emit package
  emitPkg(strm, *pkg_);

  // emit all types
  for (const auto& type_def : types_) { emitType(strm, type_def); }

  // TODO(bd) use a dependency graph to handle the case where objects are
  // declared out of order?

  // emit all objects in the order in which they occur in the input
  Set<string> fld_names;
  for (const auto& obj_name : obj_names_) {
    // emit all fields of the object
    const auto& flds =
      find_required(obj_dict_, obj_name, "defined objects", "object name");
    for (const auto& fld : flds) {
      const auto& fld_name = jmg::get<Name>(*fld);
      const auto [_, inserted] = fld_names.insert(fld_name);
      // only emit a field definition once
      if (inserted) { emitFld(strm, *fld); }
    }
    // emit the object
    emitObj(strm, obj_name, flds);
  }

  strm << "}\n";
}

string JmgYamlSpec::tgtFileName() const {
  JMG_ENFORCE_USING(
    logic_error, pred(pkg_),
    "requested target file name before input file was processed");
  return str_cat(snakeCaseToCamelCase(jmg::get<Name>(*pkg_)), ".h");
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

void JmgYamlSpec::emitPkg(ostream& strm, const jmg::PkgDef& pkg_def) const {
  strm << "namespace " << jmg::get<Name>(pkg_def) << "\n{\n";
}

void JmgYamlSpec::emitType(ostream& strm, const jmg::TypeDef& type_def) const {
  const auto type_name = jmg::get<Name>(type_def);
  const auto inner_type = jmg::get<Type>(type_def);
  if (kEnum == inner_type) {
    // emit enum
    JMG_ENFORCE(!pred(jmg::try_get<SubType>(type_def)),
                "subtype is not valid with enum for type named [", type_name,
                "]");
    JMG_ENFORCE(!pred(jmg::try_get<Concept>(type_def)),
                "concept is not valid with enum for type named [", type_name,
                "]");
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
    emitEnum(strm, type_name, jmg::try_get<EnumUlType>(type_def), *enumerations);
    return;
  }

  // emit safe type
  JMG_ENFORCE(!pred(jmg::try_get<EnumUlType>(type_def)),
              "underlying type is not valid for safe type named [", type_name,
              "]");
  JMG_ENFORCE(!pred(jmg::try_get<EnumValues>(type_def)),
              "enumeration values are not valid for safe type named [",
              type_name, "]");
  emitSafeType(strm, type_name, inner_type, jmg::try_get<Concept>(type_def));
}

void JmgYamlSpec::emitEnum(ostream& strm,
                           const string_view name,
                           const optional<string_view> ul_type,
                           const Enumerations& enumerations) const {
  strm << "\nenum class " << name;
  if (ul_type) { strm << " : " << translateType(*ul_type) << " "; }
  const auto enums_view =
    enumerations | vws::transform([&](const auto& enumeration) {
      auto enum_name = snakeCaseToCamelCase(jmg::get<Name>(enumeration),
                                            true /* capitalize_leading */);
      const auto enum_val = jmg::get<EnumValue>(enumeration);
      return str_cat("  k", std::move(enum_name), " = ", enum_val);
    });
  strm << "{\n" << str_join(enums_view, ",\n") << "\n};\n";
}

void JmgYamlSpec::emitSafeType(ostream& strm,
                               const string_view name,
                               const string_view inner_type,
                               const optional<string_view> safe_concept) const {
#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
  strm << "\nusing " << name << " = jmg::SafeType<";
#else
  strm << "\nJMG_NEW_SAFE_TYPE(" << name << ", ";
#endif
  strm << translateType(inner_type);
  if (safe_concept) {
    // TODO(bd) should not need to convert deref'd safe_concept to string here
    strm << find_required(kConceptTranslations, string(*safe_concept),
                          "safe-type concepts"sv, "concept"sv);
  }
#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
  strm << ">;\n";
#else
  strm << ");\n";
#endif
}

void JmgYamlSpec::emitFld(ostream& strm, const jmg::ObjGrpFld& fld_def) const {
  const auto fld_name = jmg::get<Name>(fld_def);
  const auto fld_type = jmg::get<Type>(fld_def);
  strm << "\nusing "
       << snakeCaseToCamelCase(fld_name, true /* capitalize_leading */)
       << " = ";
  if (kString == fld_type) { strm << encodingNamespace() << "::StringField<"; }
  else if (kArray == fld_type) {
    const auto sub_type = jmg::try_get<SubType>(fld_def);
    JMG_ENFORCE(pred(sub_type), "no subtype provided for array field [",
                fld_name, "]");
    strm << encodingNamespace() << "::ArrayField<" << translateType(*sub_type)
         << ", ";
  }
  else {
    strm << encodingNamespace() << "::" << encodingFieldDef() << "<"
         << translateType(fld_type) << ", ";
  }
  strm << "\"" << camelCaseToSnakeCase(fld_name) << "\", ";
  if (const auto required_flag = jmg::try_get<RequiredFlag>(fld_def);
      required_flag) {
    if (*required_flag) { strm << "jmg::Required"; }
    else { strm << "jmg::Optional"; }
  }
  else { strm << "jmg::Required"; }
  enrichFld(strm, fld_def);
  strm << ">;\n";
}

void JmgYamlSpec::emitObj(ostream& strm,
                          const string_view name,
                          const JmgObjGrpFlds& flds) const {
  strm << "\nusing " << name << " = " << encodingObjDef() << "<\n  ";
  const auto flds_view =
    flds | vws::transform([&](const auto& fld) {
      return snakeCaseToCamelCase(jmg::get<Name>(*fld),
                                  true /* capitalize_leading */);
    });
  strm << str_join(flds_view, ",\n  ");
  strm << ">;\n";
}

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

void JmgcYamlSpecMgr::emit(ostream& strm) const {
  for (auto& spec : specs_) {
    cerr << "base target output file would be [" << spec->tgtFileName()
         << "]\n";
    spec->emit(strm);
  }
}

void JmgcYamlSpecMgr::emit(const fs::path& tgt_directory) const {
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
