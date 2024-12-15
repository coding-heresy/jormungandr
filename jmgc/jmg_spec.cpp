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

////////////////////////////////////////////////////////////////////////////////
// Jormungandr field definitions at various levels
////////////////////////////////////////////////////////////////////////////////

using Name = FieldDef<string, "name", Required>;
using Type = FieldDef<string, "type", Required>;
using SubType = FieldDef<string, "subtype", Optional>;
using Concept = FieldDef<string, "concept", Optional>;
using RequiredFlag = FieldDef<bool, "required", Optional>;

// enumeration
// TODO(bd) should EnumValue be optional?
using EnumValue = FieldDef<int64_t, "value", Required>;
using EnumUlType = FieldDef<string, "underlying_type", Optional>;
using Enumeration = yaml::Object<Name, EnumValue>;
using Enumerations = yaml::ArrayT<Enumeration>;
using EnumValues = FieldDef<Enumerations, "values", Optional>;

// objects in the 'types' section
using TypeDef =
  yaml::Object<Name, Type, SubType, Concept, EnumUlType, EnumValues>;

// objects in the 'groups' and 'objects' sections
using ObjGrpField = yaml::Object<Name, Type, SubType, RequiredFlag>;
using Fields = yaml::ArrayT<ObjGrpField>;
using ObjGrpFields = FieldDef<Fields, "fields", Required>;

// objects in the 'groups' and 'objects' sections have a name and a
// list of fields
using ObjGrp = yaml::Object<Name, ObjGrpFields>;

// TypesList represents the contents of the 'types' section
using TypesList = yaml::ArrayT<TypeDef>;

// ObjGrpList represents the contents of the 'groups' and 'objects'
// sections
using ObjGrpList = yaml::ArrayT<ObjGrp>;

// top-level fields
using Package = FieldDef<string, "package", Required>;
using Types = FieldDef<TypesList, "types", Optional>;
using Groups = FieldDef<ObjGrpList, "groups", Optional>;
using Objects = FieldDef<ObjGrpList, "objects", Required>;

using Spec = yaml::Object<Package, Types, Groups, Objects>;

class AllJmgDefs {
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
  };

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
          always_emplace("type aliases", types_, def_name,
                         StrongAlias(type_name, *cncpt));
        }
        else if (kEnum == type_name) {
          const auto& enumerations = try_get<EnumValues>(def);
          JMG_ENFORCE(
            enumerations.has_value(),
            "enum definition must be associated with enumerated values");
          const auto& ul_type = try_get<EnumUlType>(def);
          if (ul_type.has_value()) {
            JMG_ENFORCE(
              allowed_enum_ul_types.contains(*ul_type),
              "type [" << *ul_type
                       << "] not allowed as underlying type for enumeration");
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
          always_emplace("enumerations", enums_, def_name,
                         DefEnum{.ul_type = ul_type,
                                 .values = std::move(def_values)});
        }
        else if (kArray == type_name) {
          // TODO(bd) support array aliases?
          JMG_ENFORCE(false, "array definition not yet supported");
        }
        else {
          JMG_ENFORCE(kUnion == type_name, "unsupported type name ["
                                             << type_name
                                             << "] for type definition ["
                                             << def_name << "]");
          // TODO(bd) support union definitions?
          JMG_ENFORCE(false, "union definition not yet supported");
        }
      }
    }

    // process groups
    const auto groups = try_get<Groups>(spec);
    if (groups.has_value()) {
      for (const auto& group : *groups) {
        const auto group_name = get<Name>(group);
        const auto& group_fields = get<ObjGrpFields>(group);
        vector<string> fields;
        fields.reserve(group_fields.size());
        for (const auto& fld : group_fields) {
          const auto field_name = get<Name>(fld);
          const auto field_type = get<Type>(fld);
          const auto field_sub_type = try_get<SubType>(fld);
          const auto field_required = try_get<RequiredFlag>(fld);
          // fields default to being required
          const bool required =
            field_required.has_value() ? *field_required : true;
          // check if the field already exists
          const auto& entry = fields_.find(field_name);
          if (entry != fields_.end()) {
            // verify that the details match the existing field
            verify_field(field_name, field_type, field_sub_type, required,
                         value_of(*entry));
          }
          else {
            // verify that types are valid
            verify_field_type(field_name, field_type);
            if (field_sub_type.has_value()) {
              verify_field_type(field_name, *field_sub_type);
            }
            // add the entry for this field
            always_emplace("fields", fields_, field_name,
                           DefField{.type_name = field_type,
                                    .sub_type_name = field_sub_type,
                                    .required = required});
          }
          fields.push_back(std::move(field_name));
        }
        always_emplace("groups", groups_, group_name, std::move(fields));
      }
    }

    // process objects
    for (const auto& object : get<Objects>(spec)) {
      // TODO(bd) process each object
    }
  }

  /**
   * emit a single type
   */
  static void emitType(const string_view name, const StrongAlias& alias) {
    cout << "using " << name << " = jmg::SafeType<" << alias.type_name << ", ";
    if (kKeyConcept == alias.cncpt) {
      cout << "\n  st::equality_comparable," << "\n  st::hashable,"
           << "\n  st::orderable>;\n\n";
    }
    else {
      JMG_ENFORCE(kArithmeticConcept == alias.cncpt,
                  "unsupported concept ["
                    << alias.cncpt << "] specified for type [" << name << "]");
      cout << "st::aritmetic>;\n\n";
    }
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
    cout << "using " << name << " = FieldDef<";
    if ("array" == field_def.type_name) {
      JMG_ENFORCE(field_def.sub_type_name.has_value(),
                  "field [" << name << "] is an array but has no subtype");
      // TODO(bd) allow encodings other than yaml
      cout << "yaml::ArrayT<" << *(field_def.sub_type_name) << ">";
    }
    else { cout << field_def.type_name; }
    cout << ", \"" << name << "\", "
         << (field_def.required ? "Required"sv : "Optional"sv) << ">;\n\n";
  }

  /**
   * verify that a newly declared field matches any existing field of
   * the same name
   */
  void verify_field(const string_view fld_name,
                    const string_view fld_type,
                    const optional<string>& fld_sub_type,
                    const bool fld_required,
                    const DefField& entry) {
    JMG_ENFORCE(fld_type == entry.type_name,
                "mismatched type names found for field ["
                  << fld_name << "]: [" << fld_type << "] vs ["
                  << entry.type_name << "]");
    if (fld_sub_type.has_value()) {
      JMG_ENFORCE(entry.sub_type_name.has_value(),
                  "field [" << fld_name
                            << "] incorrectly redefined to have subtype ["
                            << *fld_sub_type << "]");
    }
    else {
      JMG_ENFORCE((!entry.sub_type_name.has_value()),
                  "field ["
                    << fld_name
                    << "] incorrectly redefined to remove existing subtype");
    }
    JMG_ENFORCE(
      fld_required == entry.required,
      "field ["
        << fld_name
        << "] incorrectly redefined to reverse polarity of 'required' flag");
  }

  /**
   * verify that a type or subtype of a field was previously declared
   */
  void verify_field_type(const string_view fld_name,
                         const string_view fld_type) {
    JMG_ENFORCE((primitive_types.contains(fld_type) || types_.contains(fld_type)
                 || enums_.contains(fld_type) || objects_.contains(fld_type)),
                "field [" << fld_name << "] has type (or subtype) [" << fld_type
                          << "] that was not previously declared");
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
    // TODO(bd) allow encodings other than yaml
    cout << "#include \"jmg/yaml/yaml.h\"\n";
    cout << "\nnamespace " << package_ << "\n{\n";

    // emit type aliases
    for (const auto& [name, alias] : types_) { emitType(name, alias); }

    // emit enumerations
    for (const auto& [name, enum_def] : enums_) { emitEnum(name, enum_def); }

    // emit fields
    for (const auto& [name, field_def] : fields_) {
      emitField(name, field_def);
    }

    // TODO(bd) emit groups

    // TODO(bd) emit objects

    cout << "} // namespace " << package_ << "\n";
  }

private:
  static constexpr string_view kEnum = "enum";
  static constexpr string_view kArray = "array";
  static constexpr string_view kUnion = "array";
  static constexpr string_view kKeyConcept = "key";
  static constexpr string_view kArithmeticConcept = "arithmetic";

  static const Set<string> primitive_types;
  static const Set<string> allowed_enum_ul_types;
  static const Set<string> allowed_concepts;

  string package_;
  Dict<string, StrongAlias> types_;
  Dict<string, DefEnum> enums_;
  Dict<string, DefField> fields_;
  Dict<string, vector<string>> groups_;
  Dict<string, vector<string>> objects_;
};

const Set<string> AllJmgDefs::primitive_types = {"array"s,              //
                                                 "double"s,   "float"s, //
                                                 "int8_t"s,   "int16_t"s,
                                                 "int32_t"s,  "int64_t"s, //
                                                 "uint8_t"s,  "uint16_t"s,
                                                 "uint32_t"s, "uint64_t"s, //
                                                 "string"s};

const Set<string> AllJmgDefs::allowed_enum_ul_types = {"uint8_t"s, "uint16_t"s,
                                                       "uint32_t"s,
                                                       "uint64_t"s};

const Set<string> AllJmgDefs::allowed_concepts = {"aritmetic"s, "key"s};

namespace jmg_spec
{

void process(const string_view filePath) {
  JMG_ENFORCE(filePath.ends_with(".yml") || filePath.ends_with(".yaml"),
              "encountered non-YAML file ["
                << filePath
                << "] when attempting to process a JMG "
                   "specification");
  const auto yaml = LoadFile(string(filePath));

  const auto defs = AllJmgDefs(Spec(yaml));
  defs.emit();
}

} // namespace jmg_spec
