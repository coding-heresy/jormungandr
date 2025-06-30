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

#include <yaml-cpp/yaml.h>

#include "jmg/file_util.h"
#include "jmg/meta.h"
#include "jmg/preprocessor.h"
#include "jmg/types.h"
#include "jmg/util.h"
#include "jmg/yaml/yaml.h"

using namespace YAML;
using namespace jmg;
using namespace std;

namespace vws = std::ranges::views;

////////////////////////////////////////////////////////////////////////////////
// Jormungandr field definitions at various levels
//
// NOTE: these fields effectively define the 'syntax' of a YAML file that
// describes a legal definition of a set of Jormungandr/JMG objects
////////////////////////////////////////////////////////////////////////////////

using Name = FieldDef<string, "name", Required>;
using Type = FieldDef<string, "type", Required>;
using SubType = FieldDef<string, "subtype", Optional>;
using Concept = FieldDef<string, "concept", Optional>;
using CbeId = FieldDef<uint32_t, "cbe_id", Optional>;
using RequiredFlag = FieldDef<bool, "required", Optional>;

// enumeration
// TODO(bd) should EnumValue be optional?
using EnumValue = FieldDef<int64_t, "value", Required>;
using EnumUlType = FieldDef<string, "underlying_type", Optional>;
using Enumeration = yaml::Object<Name, EnumValue>;
using EnumValues = FieldDef<yaml::Array<Enumeration>, "values", Optional>;

// objects in the 'types' section
using TypeDef =
  yaml::Object<Name, Type, SubType, Concept, EnumUlType, EnumValues>;

// objects in the 'groups' and 'objects' sections
using ObjGrpField = yaml::Object<Name, Type, SubType, CbeId, RequiredFlag>;
using ObjGrpFields = FieldDef<yaml::Array<ObjGrpField>, "fields", Required>;

// objects in the 'groups' and 'objects' sections have a name and a
// list of fields
using ObjGrp = yaml::Object<Name, ObjGrpFields>;

// top-level fields
using Package = FieldDef<string, "package", Required>;
using Types = FieldDef<yaml::Array<TypeDef>, "types", Optional>;
using Groups = FieldDef<yaml::Array<ObjGrp>, "groups", Optional>;
using Objects = FieldDef<yaml::Array<ObjGrp>, "objects", Required>;

using Spec = yaml::Object<Package, Types, Groups, Objects>;

////////////////////////////////////////////////////////////////////////////////
// constants
////////////////////////////////////////////////////////////////////////////////

namespace
{

const Set<string> primitive_types = {"array"s,              //
                                     "double"s,   "float"s, //
                                     "int8_t"s,   "int16_t"s,
                                     "int32_t"s,  "int64_t"s, //
                                     "uint8_t"s,  "uint16_t"s,
                                     "uint32_t"s, "uint64_t"s, //
                                     "string"s};

const Set<string> allowed_enum_ul_types = {"uint8_t"s, "uint16_t"s, "uint32_t"s,
                                           "uint64_t"s};

const Set<string> allowed_concepts = {"aritmetic"s, "key"s};

/**
 * corrections that may be applied to convert the string values of types
 * specified in the file to types used in field definitions
 */
string correctedTypeName(const string_view type_name) {
  if ("string" == type_name) { return str_cat("std::", type_name); }
  return string(type_name.data(), type_name.size());
}

} // namespace

////////////////////////////////////////////////////////////////////////////////
// encoding generation policy types
////////////////////////////////////////////////////////////////////////////////

/**
 * policy for processing a YAML file or data stream
 */
struct YamlEncodingPolicy {
  static constexpr auto kHeaderFile = "jmg/yaml/yaml.h"sv;
  static constexpr auto kNamespace = "yaml"sv;
  struct FieldData {};
  template<typename T>
  static FieldData processField(const T& fld) {
    ignore = fld;
    return FieldData();
  }
  template<typename T>
  static string emitField(const string_view name, const T& field_def) {
    string rslt = str_cat("using ", name, " = jmg::FieldDef<");
    if ("array" == field_def.type_name) {
      JMG_ENFORCE(field_def.sub_type_name.has_value(), "field [", name,
                  "] is an array but has no subtype");
      const auto sub_type = *(field_def.sub_type_name);
      const auto corrected_sub_type = correctedTypeName(sub_type);
      if (primitive_types.contains(sub_type)) {
        // TODO(bd) rework YAML array types to work like CBE
        str_append(rslt, "std::vector<", corrected_sub_type, ">");
      }
      else { str_append(rslt, "jmg::yaml::Array<", corrected_sub_type, ">"); }
    }
    else { str_append(rslt, correctedTypeName(field_def.type_name)); }
    str_append(rslt, ", \"", name, "\", ",
               (field_def.required ? "jmg::Required"sv : "jmg::Optional"sv),
               ">;\n\n");
    return rslt;
  }
};

/**
 * policy for processing a file or data stream encoded in "Compressed Binary
 * Encoding"
 */
struct CbeEncodingPolicy {
  static constexpr auto kHeaderFile = "jmg/cbe/cbe.h"sv;
  static constexpr auto kNamespace = "cbe"sv;
  struct FieldData {
    uint32_t id;
  };
  template<typename T>
  static FieldData processField(const T& fld) {
    const auto cbe_id = try_get<CbeId>(fld);
    JMG_ENFORCE(pred(cbe_id), "field [", get<Name>(fld),
                "] is missing required CBE ID");
    // TODO(bd) add code to check for duplicate field IDs?
    return FieldData{.id = *cbe_id};
  }
  template<typename T>
  static string emitField(const string_view name, const T& field_def) {
    string rslt = str_cat("using ", name, " = ");
    if ("string" == field_def.type_name) {
      str_append(rslt, "jmg::cbe::StringField<");
    }
    else if ("array" == field_def.type_name) {
      JMG_ENFORCE(field_def.sub_type_name.has_value(), "field [", name,
                  "] is an array but has no subtype");
      str_append(rslt, "jmg::cbe::ArrayField<",
                 correctedTypeName(*(field_def.sub_type_name)), ", ");
    }
    else { str_append(rslt, "jmg::cbe::FieldDef<", field_def.type_name, ", "); }
    const auto id = field_def.extra_data.id;
    str_append(rslt, "\"", name, "\", ",
               (field_def.required ? "jmg::Required"sv : "jmg::Optional"sv));
    str_append(rslt, ", ", id, "U /* kFldId */>;\n\n");
    return rslt;
  }
};

/**
 * JMG definition processor and code emitter
 */
template<typename EncodingPolicy>
class AllJmgDefs {
  static constexpr auto kHeaderFile = EncodingPolicy::kHeaderFile;
  static constexpr auto kNamespace = EncodingPolicy::kNamespace;

  struct DefEnumValue {
    string name;
    int64_t value;
  };

  struct DefEnum {
    optional<string> ul_type;
    vector<DefEnumValue> values;
  };

  struct StrongAlias {
    string type_name;
    string cncpt;
  };

  struct DefField {
    string type_name;
    optional<string> sub_type_name;
    bool required;
    typename EncodingPolicy::FieldData extra_data;
  };

  using StringLists = Dict<string, vector<string>>;

public:
  AllJmgDefs(const Spec& spec) {
    package_ = get<Package>(spec);

    // process types
    const auto types = try_get<Types>(spec);
    if (types.has_value()) {
      for (const auto& def : *types) {
        const auto def_name = get<Name>(def);
        const auto type_name = get<Type>(def);
        if (primitive_types.contains(type_name)) {
          const auto cncpt = try_get<Concept>(def);
          JMG_ENFORCE(cncpt.has_value(),
                      "type alias must be associated with a concept");
          types_.push_back(make_tuple(def_name,
                                      StrongAlias(correctedTypeName(type_name),
                                                  *cncpt)));
          always_insert("known types", known_types_, def_name);
        }
        else if (kEnum == type_name) {
          const auto& enumerations = try_get<EnumValues>(def);
          JMG_ENFORCE(
            enumerations.has_value(),
            "enum definition must be associated with enumerated values");
          const auto& ul_type = try_get<EnumUlType>(def);
          if (ul_type.has_value()) {
            JMG_ENFORCE(allowed_enum_ul_types.contains(*ul_type), "type [",
                        *ul_type,
                        "] not allowed as underlying type for enumeration");
            // TODO(bd) list the allowed types?
          }
          vector<DefEnumValue> def_values;
          def_values.reserve(enumerations->size());
          for (const auto& val : *enumerations) {
            // ensure that the first letter is upper case
            string enumeration_name = jmg::get<Name>(val);
            enumeration_name[0] = toupper(enumeration_name[0]);
            def_values.emplace_back(DefEnumValue{.name =
                                                   std::move(enumeration_name),
                                                 .value =
                                                   jmg::get<EnumValue>(val)});
          }
          enums_.push_back(make_tuple(def_name,
                                      DefEnum{.ul_type = ul_type,
                                              .values = std::move(def_values)}));
          always_insert("enumerations", known_enums_, def_name);
        }
        else if (kArray == type_name) {
          // TODO(bd) support array aliases?
          JMG_ENFORCE(false, "array definition not yet supported");
        }
        else {
          JMG_ENFORCE(kUnion == type_name, "unsupported type name [", type_name,
                      "] for type definition [", def_name, "]");
          // TODO(bd) support union definitions?
          JMG_ENFORCE(false, "union definition not yet supported");
        }
      }
    }

    // process groups
    const auto groups = try_get<Groups>(spec);
    if (groups.has_value()) {
      groups_ = processGroupsOrObjects("groups", *groups);
    }

    // process objects
    objects_ = processGroupsOrObjects("objects", jmg::get<Objects>(spec));
  }

  void emit() const {
    cout << "//////////////////////////////////////////////////////////////////"
            "//////////////\n";
    cout << "// WARNING: this file is generated automatically by jmgc and "
            "should not be\n";
    cout << "// edited manually\n";
    cout << "//////////////////////////////////////////////////////////////////"
            "//////////////\n";
    cout << "#pragma once\n\n";
    cout << "#include \"jmg/safe_types.h\"\n";
    cout << "#include \"" << kHeaderFile << "\"\n";
    cout << "\nnamespace " << package_ << "\n{\n";

    // emit type aliases
    cout << "////////////////////\n// types\n\n";
    for (const auto& [name, alias] : types_) { emitType(name, alias); }

    // emit enumerations
    cout << "////////////////////\n// enumerations\n\n";
    for (const auto& [name, enum_def] : enums_) { emitEnum(name, enum_def); }

    // emit fields
    cout << "////////////////////\n// fields\n\n";
    for (const auto& [name, field_def] : fields_) {
      emitField(name, field_def);
    }

    // emit groups
    cout << "////////////////////\n// groups\n\n";
    for (const auto& [name, fields] : groups_) {
      cout << "using " << name << " = jmg::FieldGroupDef<"
           << str_join(fields, ", ") << ">;\n\n";
    }

    // emit objects
    cout << "////////////////////\n// objects\n\n";
    for (const auto& [name, fields] : objects_) {
      cout << "using " << name << " = jmg::" << kNamespace << "::Object<"
           << str_join(fields, ", ") << ">;\n\n";
    }

    cout << "} // namespace " << package_ << "\n";
  }

private:
  ////////////////////////////////////////////////////////////////////////////////
  // member functions
  ////////////////////////////////////////////////////////////////////////////////

  /**
   * emit a single type
   */
  static void emitType(const string_view name, const StrongAlias& alias) {
#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
    cout << "using " << name << " = jmg::SafeType<";
#else
    cout << "JMG_NEW_SAFE_TYPE(";
#endif
    if (kKeyConcept == alias.cncpt) {
      cout
        << "\n  " << name << "," << "\n  " << alias.type_name
        << ",\n  st::equality_comparable,\n  st::hashable,\n  st::orderable\n";
    }
    else {
      JMG_ENFORCE(kArithmeticConcept == alias.cncpt, "unsupported concept [",
                  alias.cncpt, "] specified for type [", name, "]");
      cout << name << "," << " " << alias.type_name << ", st::arithmetic";
    }
#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
    cout << ">;\n\n";
#else
    cout << ");\n\n";
#endif
  }

  /**
   * emit a single enum
   */
  static void emitEnum(const string_view name, const DefEnum& enum_def) {
    cout << "enum class " << name;
    if (enum_def.ul_type.has_value()) { cout << " : " << *(enum_def.ul_type); }
    cout << " {\n";
    bool is_first = true;
    for (const auto& value : enum_def.values) {
      if (is_first) { is_first = false; }
      else { cout << ",\n"; }
      cout << "  k" << value.name << " = " << value.value;
    }
    cout << "\n};\n\n";
  }

  /**
   * emit a single field
   */
  static void emitField(const string_view name, const DefField& field_def) {
    cout << EncodingPolicy::emitField(name, field_def);
  }

  /**
   * verify that a newly declared field matches any existing field of
   * the same name
   */
  void verifyField(const string_view fld_name,
                   const string_view fld_type,
                   const optional<string>& fld_sub_type,
                   const bool fld_required,
                   const DefField& entry) {
    JMG_ENFORCE(fld_type == entry.type_name,
                "mismatched type names found for field [", fld_name, "]: [",
                fld_type, "] vs [", entry.type_name, "]");
    if (fld_sub_type.has_value()) {
      JMG_ENFORCE(entry.sub_type_name.has_value(), "field [", fld_name,
                  "] incorrectly redefined to have subtype [", *fld_sub_type,
                  "]");
    }
    else {
      JMG_ENFORCE((!entry.sub_type_name.has_value()), "field [", fld_name,
                  "] incorrectly redefined to remove existing subtype");
    }
    JMG_ENFORCE(
      fld_required == entry.required, "field [", fld_name,
      "] incorrectly redefined to reverse polarity of 'required' flag");
  }

  /**
   * verify that a type or subtype of a field was previously declared
   */
  void verifyFieldType(const string_view fld_name,
                       const string_view fld_type,
                       StringLists internally_declared) {
    JMG_ENFORCE((primitive_types.contains(fld_type)
                 || known_types_.contains(fld_type)
                 || known_enums_.contains(fld_type)
                 || objects_.contains(fld_type)
                 || internally_declared.contains(fld_type)),
                "field [", fld_name, "] has type (or subtype) [", fld_type,
                "] that was not previously declared");
  }

  /**
   * process the specs for both groups and objects
   *
   * NOTE: the format is the same for both
   */
  StringLists processGroupsOrObjects(const string_view description,
                                     const yaml::Array<ObjGrp>& spec) {
    StringLists rslt;
    for (const auto& sub_spec : spec) {
      const auto spec_name = get<Name>(sub_spec);
      const auto& spec_fields = get<ObjGrpFields>(sub_spec);
      vector<string> fields;
      fields.reserve(spec_fields.size());
      for (const auto& fld : spec_fields) {
        const auto extra_data = EncodingPolicy::processField(fld);
        const auto field_name = get<Name>(fld);
        const auto field_type = get<Type>(fld);
        const auto field_sub_type = try_get<SubType>(fld);
        const auto field_required = try_get<RequiredFlag>(fld);
        // fields default to being required
        const bool required =
          field_required.has_value() ? *field_required : true;
        // check if the field already exists
        const auto& entry = fields_indices_.find(field_name);
        if (entry != fields_indices_.end()) {
          // verify that the details match the existing field
          const auto& def = value_of(fields_.at(value_of(*entry)));
          verifyField(field_name, field_type, field_sub_type, required, def);
        }
        else {
          // verify that types are valid
          verifyFieldType(field_name, field_type, rslt);
          if (field_sub_type.has_value()) {
            verifyFieldType(field_name, *field_sub_type, rslt);
          }
          // add the entry for this field
          const auto idx = fields_.size() + 1;
          fields_.push_back(make_tuple(field_name,
                                       DefField{.type_name = field_type,
                                                .sub_type_name = field_sub_type,
                                                .required = required,
                                                .extra_data = extra_data}));
          always_emplace("fields", fields_indices_, field_name, idx);
        }
        fields.push_back(std::move(field_name));
      }
      always_emplace(description, rslt, spec_name, std::move(fields));
    }
    return rslt;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // constants
  ////////////////////////////////////////////////////////////////////////////////

  static constexpr string_view kEnum = "enum";
  static constexpr string_view kArray = "array";
  static constexpr string_view kUnion = "union";
  static constexpr string_view kKeyConcept = "key";
  static constexpr string_view kArithmeticConcept = "arithmetic";

  ////////////////////////////////////////////////////////////////////////////////
  // data members
  ////////////////////////////////////////////////////////////////////////////////

  string package_;

  // types
  vector<tuple<string, StrongAlias>> types_;
  Set<string> known_types_;

  // enums
  vector<tuple<string, DefEnum>> enums_;
  Set<string> known_enums_;

  // fields
  vector<tuple<string, DefField>> fields_;
  Dict<string, size_t> fields_indices_;

  StringLists groups_;
  StringLists objects_;
};

namespace jmg_yml_spec
{

void process(const string_view filePath) {
  JMG_ENFORCE(filePath.ends_with(".yml") || filePath.ends_with(".yaml"),
              "encountered non-YAML file [", filePath,
              "] when attempting to process a JMG specification");
  const auto yaml = LoadFile(string(filePath));

  const auto defs = AllJmgDefs<YamlEncodingPolicy>(Spec(yaml));
  defs.emit();
}

} // namespace jmg_yml_spec

namespace jmg_cbe_spec
{
void process(const string_view filePath) {
  JMG_ENFORCE(filePath.ends_with(".yml") || filePath.ends_with(".yaml"),
              "encountered non-YAML file [", filePath,
              "] when attempting to process a JMG specification");
  const auto yaml = LoadFile(string(filePath));

  const auto defs = AllJmgDefs<CbeEncodingPolicy>(Spec(yaml));
  defs.emit();
}

} // namespace jmg_cbe_spec
