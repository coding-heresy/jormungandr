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

/**
 * compiler for Jormungandr field and message definitions
 */

#include <iostream>
#include <iterator>
#include <optional>
#include <string_view>
#include <unordered_set>

#include <boost/property_tree/xml_parser.hpp>

#include "jmg/field.h"
#include "jmg/file_util.h"
#include "jmg/meta.h"
#include "jmg/object.h"
#include "jmg/preprocessor.h"
#include "jmg/ptree/ptree.h"
#include "jmg/util.h"

using namespace jmg;
using namespace std;
namespace xml = ptree::xml;

namespace
{
constexpr string_view kEnumTypeSuffix = "Enum";
}

namespace quickfix_spec
{
constexpr string_view kTopLevelTag = "fix";
// element names within the top level 'fix' element
constexpr string_view kFixHeader = "header";
constexpr string_view kFixMsgs = "messages";
constexpr string_view kFixTrailer = "trailer";
constexpr string_view kFixComponents = "components";
constexpr string_view kFixFields = "fields";
// element names within header/trailer/message/fields elements
constexpr string_view kFixComponent = "component";
constexpr string_view kFixField = "field";
constexpr string_view kFixGroup = "group";
constexpr string_view kFixMsg = "message";
constexpr string_view kEnumValue = "value";

////////////////////////////////////////////////////////////////////////////////
// Jormungandr field definitions at various levels
////////////////////////////////////////////////////////////////////////////////

// top level fields
JMG_XML_FIELD_DEF(ProtocolType, "type", string, true);
JMG_XML_FIELD_DEF(MajorVersion, "major", string, true);
JMG_XML_FIELD_DEF(MinorVersion, "minor", string, true);
JMG_XML_FIELD_DEF(SvcPackVersion, "servicepack", string, true);
// fields for message definitions
JMG_XML_FIELD_DEF(FixMsgName, "name", string, true);
JMG_XML_FIELD_DEF(FixMsgType, "msgtype", string, true);
JMG_XML_FIELD_DEF(FixMsgCategory, "msgcat", string, true);
JMG_XML_FIELD_DEF(RequiredFlag, "required", string, true);
// fields for FIX field definitions
JMG_XML_FIELD_DEF(FixFieldName, "name", string, true);
JMG_XML_FIELD_DEF(FixFieldTag, "number", string, true);
JMG_XML_FIELD_DEF(FixFieldType, "type", string, true);
// fields for FIX enum value  declarations
JMG_XML_FIELD_DEF(EnumValue, "enum", string, true);
JMG_XML_FIELD_DEF(EnumDescription, "description", string, true);
////////////////////////////////////////////////////////////////////////////////
// fields and objects associated with FIX message definitions
////////////////////////////////////////////////////////////////////////////////
// object specifying a FIX field within a FIX message definition
using MsgField = xml::Object<FixFieldName, RequiredFlag>;
// field referencing an array of FIX message field specifications
using MsgFields = xml::Elements<MsgField, xml::ElementsRequired>;
// object containing an array of FIX message field declarations
using MsgFieldDefs = xml::ElementsArrayT<MsgField>;
// object specifying a FIX message definition
using FixMsg = xml::Object<FixMsgName, FixMsgType, FixMsgCategory, MsgFields>;
// field referencing an array of FIX message definitions
using FixMsgDefs = xml::ElementsArrayT<FixMsg>;

////////////////////////////////////////////////////////////////////////////////
// fields and objects associated with FIX field definitions
////////////////////////////////////////////////////////////////////////////////
// object specifying a FIX enum value
using FixFieldEnum = xml::Object<EnumValue, EnumDescription>;
// field referencing an array of FIX enum values
using FixFieldEnums = xml::Elements<FixFieldEnum>;
// object specifying a FIX field definition
using FixField =
  xml::Object<FixFieldName, FixFieldTag, FixFieldType, FixFieldEnums>;
// field referencing an array of FIX field definitions
using FixFieldDefs = xml::ElementsArrayT<FixField>;

// union that holds one of the following:
//  1. an array of FIX message definitions
//  2. an array of FIX field definitions
// TODO add the following: header, trailer and components
using AnyFixDefinitionSection =
  Union<boost::property_tree::ptree, MsgFieldDefs, FixMsgDefs, FixFieldDefs>;

// field referencing the previously defined union of FIX definition
// sections
struct FixDefinition : FieldDef {
  static constexpr char const* name = kPlaceholder;
  using type = AnyFixDefinitionSection;
  using required = std::true_type;
};

// object containing a FIX definition section
using FixDefinitionSection = xml::Object<FixDefinition>;
// field referencing an array of FIX definition sections
using FixDefinitions =
  xml::Elements<FixDefinitionSection, xml::ElementsRequired>;

using TopLevelElement = xml::
  Object<ProtocolType, MajorVersion, MinorVersion, SvcPackVersion, FixDefinitions>;

using AllXmlElements = xml::ElementsArrayT<TopLevelElement>;

/**
 * helper function to convert Y/N to true/false
 */
[[maybe_unused]] bool isRequired(const string_view val) {
  if (val == "Y") { return true; }
  JMG_ENFORCE(val == "N",
              "unexpected value '" << val << "' for 'required' attribute");
  return false;
}

/**
 * singleton object where all data is stored during parsing, later
 * used to emit output
 */
class AllFixData {
  auto processFieldDeclarations(const string_view name, const auto& fields) {
    // TODO handle 'required' flag
    vector<string> fieldNames;
    for (const auto& field : fields) {
      const auto& fieldTag = jmg::get<ptree::xml::ElementTag>(field);
      if (kFixGroup == fieldTag) {
        cerr << "WARNING: skipping 'group' element of message definition for ["
             << name << "]\n";
        continue;
      }
      JMG_ENFORCE(kFixField == fieldTag,
                  "unexpected XML tag ["
                    << fieldTag
                    << "] on element of message or group fields declarations");
      fieldNames.push_back(jmg::get<FixFieldName>(field));
    }
    return fieldNames;
  }

  void dumpStrVec(const vector<string>& vec) {
    auto itr = std::begin(vec);
    cout << *itr;
    ++itr;
    ranges::copy(ranges::subrange{itr, std::end(vec)}
                   | views::transform([](const auto& fld) { return "," + fld; }),
                 ostream_iterator<string>(cout));
  }

public:
  /**
   * process all entries in the 'header' element
   */
  void processHeader(const auto& headerElement) {
    // TODO handle 'required' flag
    auto fieldNames = processFieldDeclarations(
      "header",
      jmg::get<FixDefinition>(headerElement).template as<MsgFieldDefs>());
    headerFields_ = std::move(fieldNames);
  }

  /**
   * process all entries in the 'trailer' element
   */
  void processTrailer(const auto& trailerElement) {
    // TODO handle 'required' flag
    trailerFields_ = processFieldDeclarations(
      "trailer",
      jmg::get<FixDefinition>(trailerElement).template as<MsgFieldDefs>());
  }

  /**
   * process all entries in the 'messages' element
   */
  void processMsgs(const auto& msgsElement) {
    for (const auto& msg :
         jmg::get<FixDefinition>(msgsElement).template as<FixMsgDefs>()) {
      const auto& tag = jmg::get<ptree::xml::ElementTag>(msg);
      JMG_ENFORCE(kFixMsg == tag, "unexpected XML tag ["
                                    << tag
                                    << "] on element in [messages] section");
      const auto& msgName = jmg::get<FixMsgName>(msg);
      auto fieldNames =
        processFieldDeclarations(msgName, jmg::get<MsgFields>(msg));
      auto [_, inserted] = msgs_.try_emplace(msgName, std::move(fieldNames));
      JMG_ENFORCE(inserted,
                  "duplicate definitions for message [" << msgName << "]");
    }
  }

  /**
   * process all values for an enum field
   */
  void processEnum(const string& fieldName,
                   const string& fieldType,
                   const auto& enumValues) {
    char delim = '\'';
    EnumType enumType = EnumType::kInt;
    if (kCharFieldTypes_.count(fieldType)) {
      if (("STRING" == fieldType) && (enumValues.size() > 1)
          && (enumValues.end()
              != ranges::find_if(enumValues, [&](const auto& enumVal) {
                   return jmg::get<EnumValue>(enumVal).size() > 1;
                 }))) {
        enumType = EnumType::kString;
        delim = '"';
      }
      else { enumType = EnumType::kChar; }
    }
    else {
      JMG_ENFORCE("INT" == fieldType,
                  "unexpected type ["
                    << fieldType << "] associated with field [" << fieldName
                    << "] that is an enumeration");
    }
    auto [entry, inserted] = enums_.insert({fieldName, {}});
    JMG_ENFORCE(inserted,
                "duplicate enumerations for field [" << fieldName << "]");
    std::get<0>(value_of(*entry)) = enumType;
    auto& values = std::get<1>(value_of(*entry));
    for (const auto& enumVal : enumValues) {
      const auto& tag = jmg::get<ptree::xml::ElementTag>(enumVal);
      JMG_ENFORCE(kEnumValue == tag,
                  "unexpected XML tag ["
                    << tag << "] on element in enumeration values for field ["
                    << fieldName << "]");
      const auto& rawValue = jmg::get<EnumValue>(enumVal);
      ostringstream value;
      if (EnumType::kInt != enumType) { value << delim; }
      value << rawValue;
      if (EnumType::kInt != enumType) { value << delim; }
      // TODO add the first field of the tuple indicating the base type of CHAR,
      // STRING or INT
      values.push_back({.value = value.str(),
                        .name = jmg::get<EnumDescription>(enumVal)});
    }
  }

  /**
   * process all entries in the 'fields' element
   */
  void processFields(const auto& fieldsElement) {
    for (const auto& field :
         jmg::get<FixDefinition>(fieldsElement).template as<FixFieldDefs>()) {
      const auto& tag = jmg::get<ptree::xml::ElementTag>(field);
      JMG_ENFORCE(kFixField == tag, "unexpected XML tag ["
                                      << tag
                                      << "] on element in [fields] section");
      // process number/name/type and add basic entry to the
      // dictionary of fields
      const auto& fieldName = jmg::get<FixFieldName>(field);
      const int fixTag = stoi(jmg::get<FixFieldTag>(field));
      JMG_ENFORCE(fixTag >= 0, "got negative FIX tag ["
                                 << fixTag << "] for field named [" << fieldName
                                 << "]");
      const auto& fieldType = jmg::get<FixFieldType>(field);
      // handle any associated enumeration, which may update the type
      string effectiveType = fieldType;
      const auto enumValues = jmg::try_get<FixFieldEnums>(field);
      if (enumValues) { processEnum(fieldName, fieldType, *enumValues); }
      auto [_, inserted] =
        fields_.try_emplace(fieldName,
                            FieldSpec{.tag = static_cast<unsigned>(fixTag),
                                      .type = effectiveType});
      JMG_ENFORCE(inserted, "duplicate definitions for field ["
                              << fieldName << "] in [fields] section");
    }
  }

  /**
   * emit C++ text output
   */
  void emit() {
    cout << "#pragma once\n";
    cout << "#include \"jmg/quickfix/quickfix.h\"\n";
    cout << "\nnamespace jmg\n{\n";
    cout << "\n// enumerations\n\n";
    for (const auto& [name, def] : enums_) {
      string prefix = "";
      string term = ",";
      if (EnumType::kString == std::get<0>(def)) {
        cout << "struct " << name << kEnumTypeSuffix << " {\n";
        prefix = "inline static const std::string ";
        term = ";";
      }
      else {
        cout << "enum class " << name << kEnumTypeSuffix << " : "
             << ((EnumType::kInt == std::get<0>(def)) ? "uint8_t"s : "char"s)
             << " {\n";
      }
      for (const auto& spec : std::get<1>(def)) {
        cout << "  " << prefix << "k" << spec.name << " = " << spec.value
             << term << "\n";
      }
      cout << "};\n";
    }
    cout << "\n// fields\n\n";
    for (const auto& [name, spec] : fields_) {
      cout << "struct " << name << " : jmg::FieldDef\n{\n";
      cout << "  static constexpr char name[] = \"" << name << "\";\n";
      cout << "  using type = ";
      const auto enumEntry = enums_.find(name);
      if (enums_.end() == enumEntry) {
        // values for this field come from a standard type and not an
        // enumeration
        const auto entry = kTypeTranslation_.find(spec.type);
        JMG_ENFORCE(kTypeTranslation_.end() != entry,
                    "unknown FIX protocol type [" << spec.type << "]");
        cout << value_of(*entry);
      }
      else { cout << key_of(*enumEntry) << kEnumTypeSuffix; }
      cout << ";\n";
      // TODO not all fields should be required at all times but this
      // brings up the thorny question of fields being required in
      // some messages and not in others...
      cout << "  using required = std::true_type;\n";
      cout << "  static constexpr uint32_t kFixTag = " << spec.tag << ";\n";
      cout << "  using FixType = FIX::" << name << ";\n";
      cout << "};\n\n";
    }
    cout << "\n// message header\n\n";
    cout << "using MsgHeader = jmg::FieldGroupDef<";
    dumpStrVec(headerFields_);
    cout << ">;\n";

    cout << "\n// message trailer\n\n";
    cout << "using MsgTrailer = jmg::FieldGroupDef<";
    dumpStrVec(trailerFields_);
    cout << ">;\n";

    cout << "\n// messages\n\n";
    for (const auto& [name, fields] : msgs_) {
      cout << "using " << name << " = jmg::quickfix::Object<MsgHeader,";
      ranges::copy(fields, ostream_iterator<string>(cout, ","));
      cout << "MsgTrailer>;\n";
    }
    cout << "\n} // namespace jmg\n";
  }

private:
  /**
   * field declarations for header/messages/trailer sections
   */
  struct MsgField {
    string name;
    bool required;
  };
  /**
   * message definitions for 'messages' section
   */
  struct Msg {
    string name;
    unsigned typeNumber;
    // TODO add message category from msgcat?
    vector<MsgField> fields;
  };
  /**
   * field definitions for 'fields' section
   */
  struct FieldSpec {
    unsigned tag;
    string type;
  };
  /**
   * value declarations for values associated with enum fields
   */
  struct FieldEnumeration {
    string value;
    string name;
  };
  enum class EnumType : uint8_t { kChar, kString, kInt };
  using EnumSpec = tuple<EnumType, vector<FieldEnumeration>>;

  // translation from type string in the XML declaration to the
  // appropriate C++ type
  static const unordered_map<string, string> kTypeTranslation_;
  // set of types for that are represented by strings or single
  // characters
  static const unordered_set<string> kCharFieldTypes_;

  vector<string> headerFields_;
  vector<string> trailerFields_;
  unordered_map<string, FieldSpec> fields_;
  unordered_map<string, EnumSpec> enums_;
  unordered_map<string, vector<string>> msgs_;
};

const unordered_map<string, string> AllFixData::kTypeTranslation_ = {
  {"STRING", "std::string"},
  {"CHAR", "char"},
  {"BOOLEAN", "bool"},
  {"INT", "int"},
  {"FLOAT", "double"},
  {"LENGTH", "size_t"},
  // TODO create precise class for prices
  {"PRICE", "double"},
  {"PRICEOFFSET", "double"},
  // TODO use strongly-typed alias for the following types
  {"AMT", "unsigned"},
  {"QTY", "unsigned"},
  {"CURRENCY", "unsigned"},
  {"AMT", "unsigned"},
  {"DAYOFMONTH", "uint8_t"},
  // TODO the following types are likely enums
  {"MULTIPLEVALUESTRING", "std::string"},
  {"EXCHANGE", "std::string"},
  // TODO decide which type(s) to use for timestamps, dates, etc
  {"UTCTIMESTAMP", "std::string"},
  {"LOCALMKTDATE", "std::string"},
  {"UTCTIMEONLY", "std::string"},
  {"UTCDATE", "std::string"},
  {"MONTHYEAR", "std::string"},
  // TODO use some sort of raw byte buffer type for this
  {"DATA", "std::string"}};

const unordered_set<string> AllFixData::kCharFieldTypes_ = {
  "CHAR", "STRING", "BOOLEAN", "MULTIPLEVALUESTRING"};

void process(const string_view filePath) {
  auto strm = open_file<ifstream>(filePath);
  boost::property_tree::ptree data;
  read_xml(strm, data);

  const AllXmlElements allElements{data};
  JMG_ENFORCE(1 == allElements.size(), "quickfix XML spec should have a single "
                                       "top-level element but actually has ["
                                         << data.size() << "]");

  auto allFixDefs = *(allElements.begin());
  JMG_ENFORCE(kTopLevelTag == jmg::get<ptree::xml::ElementTag>(allFixDefs),
              "quickfix XML spec top-level element should have name [fix] but "
              "actually has ["
                << jmg::get<ptree::xml::ElementTag>(allFixDefs) << "]");

  AllFixData fixData;
  for (const auto& elem : jmg::get<FixDefinitions>(allFixDefs)) {
    const auto& tag = jmg::get<ptree::xml::ElementTag>(elem);
    if (tag == kFixHeader) { fixData.processHeader(elem); }
    else if (tag == kFixMsgs) { fixData.processMsgs(elem); }
    else if (tag == kFixTrailer) { fixData.processTrailer(elem); }
    else if (tag == kFixComponents) {
      // TODO
    }
    else if (tag == kFixFields) { fixData.processFields(elem); }
    else {
      cerr << "ERROR: ignoring XML element with tag [" << tag
           << "] in main FIX definitions\n";
    }
  }
  fixData.emit();
}

} // namespace quickfix_spec

int main(const int argc, const char** argv) {
  try {
    quickfix_spec::process(argv[1]);
  }
  catch (const exception& e) {
    cerr << "exception: " << e.what() << "\n";
  }
  catch (...) {
    cerr << "unexpected exception type\n";
  }
}
