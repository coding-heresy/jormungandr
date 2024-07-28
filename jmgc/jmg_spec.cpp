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
// TODO should EnumValue be optional?
using EnumValue = FieldDef<int64_t, "value", Required>;
using Enumeration = yaml::Object<Name, EnumValue>;
using Enumerations = yaml::ArrayT<Enumeration>;
using EnumValues = FieldDef<Enumerations, "values", Optional>;

using TypeDef = yaml::Object<Name, Type, SubType, Concept, EnumValues>;

using ObjGrpField = yaml::Object<Name, Type, SubType, RequiredFlag>;
using Fields = yaml::ArrayT<ObjGrpField>;
using ObjGrpFields = FieldDef<Fields, "fields", Required>;

using ObjGrp = yaml::Object<Name, ObjGrpFields>;

using TypesList = yaml::ArrayT<TypeDef>;

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

  struct StrongAlias {
    string type_name;
    string cncpt;
  };

  struct DefObjGrp {
    string name;
    string type_name;
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
        if (allowed_types.contains(type_name)) {
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
          // TODO save enumerations
        }
        else if (kArray == type_name) {
          // TODO support array aliases?
          JMG_ENFORCE(false, "array definition not yet supported");
        }
        else {
          JMG_ENFORCE(kUnion == type_name, "unsupported type name ["
                                             << type_name
                                             << "] for type definition ["
                                             << def_name << "]");
          // TODO support union definitions?
          JMG_ENFORCE(false, "union definition not yet supported");
        }
      }
    }

    // process groups
    const auto groups = try_get<Groups>(spec);
    if (groups.has_value()) {
      for (const auto& group : *groups) {
        // TODO process each object
      }
    }

    // process objects
    for (const auto& object : get<Objects>(spec)) {
      // TODO process each object
    }
  }

  static void emitType(const string_view name, const StrongAlias& alias) {
    cout << "JMG_NEW_SAFE_BASE_TYPE(" << name << ", " << alias.type_name
         << ", ";
    if (kKeyConcept == alias.cncpt) {
      cout << "\n                       st::equality_comparable,"
           << "\n                       st::hashable,"
           << "\n                       st::orderable);\n\n";
    }
    else {
      JMG_ENFORCE(kArithmeticConcept == alias.cncpt,
                  "unsupported concept ["
                    << alias.cncpt << "] specified for type [" << name << "]");
      cout << "st::aritmetic);\n\n";
    }
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
    // TODO allow encodings other than yaml
    cout << "#include \"jmg/yaml/yaml.h\"\n";
    cout << "\nnamespace " << package_ << "\n{\n";

    // TODO emit type aliases
    for (const auto& [name, alias] : types_) { emitType(name, alias); }

    // TODO emit enumerations

    // TODO emit groups

    // TODO emit objects

    cout << "} // namespace " << package_ << "\n";
  }

private:
  static constexpr string_view kEnum = "enum";
  static constexpr string_view kArray = "array";
  static constexpr string_view kUnion = "array";
  static constexpr string_view kKeyConcept = "key";
  static constexpr string_view kArithmeticConcept = "arithmetic";

  static const Set<string> allowed_types;
  static const Set<string> allowed_concepts;

  string package_;
  Dict<string, StrongAlias> types_;
  Dict<string, vector<DefEnumValue>> enums_;
  Dict<string, DefObjGrp> groups_;
  Dict<string, DefObjGrp> objects_;
};

const Set<string> AllJmgDefs::allowed_types = {
  "string"s,   "int8_t"s,   "int16_t"s,  "int32_t"s, "int64_t"s, "uint8_t"s,
  "uint16_t"s, "uint32_t"s, "uint64_t"s, "float"s,   "double"s};

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
