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

#include <cstdlib>

#include <iostream>
#include <iterator>
#include <optional>
#include <string_view>

#include <boost/property_tree/xml_parser.hpp>

#include "jmg/file_util.h"
#include "jmg/meta.h"
#include "jmg/preprocessor.h"
#include "jmg/ptree/ptree.h"
#include "jmg/types.h"
#include "jmg/util.h"
#include "spec_util.h"

using namespace jmg;
using namespace std;

namespace xml = ptree::xml;

namespace
{
constexpr string_view kEnumTypeSuffix = "Enum";
} // namespace

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
// TODO(bd) 'component' seems to be unused
// constexpr string_view kFixComponent = "component";
constexpr string_view kFixField = "field";
constexpr string_view kFixGroup = "group";
constexpr string_view kFixMsg = "message";
constexpr string_view kEnumValue = "value";
// header and trailer field group definition names
constexpr string_view kHeaderDef = "MsgHeader";
constexpr string_view kTrailerDef = "MsgTrailer";

////////////////////////////////////////////////////////////////////////////////
// field definitions at various levels
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
// objects associated with FIX message definitions
////////////////////////////////////////////////////////////////////////////////

// FIX field within a FIX message definition
using MsgField = xml::Object<FixFieldName, RequiredFlag>;
// array of FIX message field specifications
using MsgFields = xml::Elements<MsgField, xml::ElementsRequired>;
// array of FIX message field declarations
using MsgFieldDefs = xml::ElementsArrayT<MsgField>;
// FIX message definition
using FixMsg = xml::Object<FixMsgName, FixMsgType, FixMsgCategory, MsgFields>;
// array of FIX message definitions
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
//  1. an array of FIX message field definitions
//  2. an array of FIX message definitions
//  3. an array of FIX field definitions
// TODO add components definitions section?
using AnyFixDefinitionSection =
  Union<boost::property_tree::ptree, MsgFieldDefs, FixMsgDefs, FixFieldDefs>;

// field referencing the previously defined union of FIX definition
// sections
struct FixDefinition
  : FieldDef<AnyFixDefinitionSection, kPlaceholder, Required> {};

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
class AllFixDefs {
public:
  /**
   * process all entries in the 'header' element
   */
  void processHeader(const auto& headerElement) {
    header_ = processFieldDeclarations(
      kHeaderDef,
      jmg::get<FixDefinition>(headerElement).template as<MsgFieldDefs>());
  }

  /**
   * process all entries in the 'trailer' element
   */
  void processTrailer(const auto& trailerElement) {
    trailer_ = processFieldDeclarations(
      kTrailerDef,
      jmg::get<FixDefinition>(trailerElement).template as<MsgFieldDefs>());
  }

  /**
   * process all entries in the 'messages' element
   */
  void processMsgs(const auto& msgsElement) {
    Set<string> names;
    for (const auto& msg :
         jmg::get<FixDefinition>(msgsElement).template as<FixMsgDefs>()) {
      const auto& tag = jmg::get<ptree::xml::ElementTag>(msg);
      JMG_ENFORCE(kFixMsg == tag, "unexpected XML tag ["
                                    << tag
                                    << "] on element in [messages] section");
      const auto& name = jmg::get<FixMsgName>(msg);
      const auto [_, inserted] = names.insert(name);
      JMG_ENFORCE(inserted,
                  "encountered duplicate message name [" << name << "]");
      msgs_.push_back(processFieldDeclarations(name, jmg::get<MsgFields>(msg)));
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
    cout << "//////////////////////////////////////////////////////////////////"
            "//////////////\n";
    cout << "// WARNING: this file is generated automatically by jmgc and "
            "should not be\n";
    cout << "// edited manually\n";
    cout << "//////////////////////////////////////////////////////////////////"
            "//////////////\n";
    cout << "#pragma once\n\n";
    cout << "#include \"jmg/quickfix/quickfix.h\"\n";
    cout << "\nnamespace fix_spec\n{\n";

    // emit all enumerations first
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

    // emit all message definitions
    emitMsg(header_);
    emitMsg(trailer_);
    for (const auto& msg : msgs_) { emitMsg(msg); }

    // emit mappings for length fields that must be parsed separately
    cout << "inline const jmg::Dict<unsigned, unsigned> kLengthFields{";
    bool firstMatch = false;
    for (const auto& [name, spec] : fields_) {
      if ("LENGTH"s == spec.type) {
        // find the data field that this LENGTH field corresponds to
        const auto matchName = [&]() {
          if (name.ends_with("Len")) { return name.substr(0, name.size() - 3); }
          JMG_ENFORCE(name.ends_with("Length"),
                      "encountered bad named ["
                        << name
                        << "] for LENGTH field, should have suffix "
                           "'Len' or 'Length'");
          return name.substr(0, name.size() - 6);
        }();
        const auto match = fields_.find(matchName);
        JMG_ENFORCE(match != fields_.end(),
                    "matching name ["
                      << matchName << "] for length field [" << name
                      << "] was not found in field definitions");
        if (!firstMatch) { firstMatch = true; }
        else { cout << ","; }
        cout << "\n  {" << spec.tag << "," << value_of(*match).tag << "}";
      }
    }
    cout << "\n};\n\n";
    cout << "} // namespace jmg::\n";
  }

private:
  /**
   * field declarations for header/messages/trailer sections
   */
  struct FieldInMsg {
    string name;
    bool required;
  };
  /**
   * message definitions for 'messages' section
   */
  struct Msg {
    string name;
    optional<string> msgType;
    // TODO add message category from msgcat?
    vector<FieldInMsg> fields;
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

  auto processFieldDeclarations(const string_view name, const auto& fields) {
    Msg msg;
    msg.name = name;
    for (const auto& field : fields) {
      const auto& fieldTag = jmg::get<ptree::xml::ElementTag>(field);
      if (kFixGroup == fieldTag) {
        // TODO handle repeating groups correctly
        cerr << "WARNING: skipping 'group' element of message definition for ["
             << name << "]\n";
        continue;
      }
      JMG_ENFORCE(kFixField == fieldTag,
                  "unexpected XML tag ["
                    << fieldTag
                    << "] on element of message or group fields declarations");
      msg.fields.emplace_back(jmg::get<FixFieldName>(field),
                              isRequired(jmg::get<RequiredFlag>(field)));
    }
    return msg;
  }

  void emitField(const FieldInMsg& fld) {
    cout << "struct " << fld.name << " : jmg::FieldDef<";

    // look up field spec using name
    const auto spec_entry = fields_.find(fld.name);
    JMG_ENFORCE(fields_.end() != spec_entry,
                "unknown message field name [" << fld.name << "]");
    const auto& spec = value_of(*spec_entry);

    // emit the field type
    const auto enumEntry = enums_.find(fld.name);
    if (enums_.end() == enumEntry) {
      // values for this field come from a standard type and not an
      // enumeration
      const auto entry = kTypeTranslation_.find(spec.type);
      JMG_ENFORCE(kTypeTranslation_.end() != entry,
                  "unknown FIX protocol type [" << spec.type << "]");
      cout << value_of(*entry);
    }
    else { cout << key_of(*enumEntry) << kEnumTypeSuffix; }

    // emit the field name
    cout << ", \"" << fld.name << "\", ";

    // emit the 'required' attribute
    if (fld.required) { cout << "jmg::Required"; }
    else { cout << "jmg::Optional"; }
    cout << "> {\n";

    // emit the tag
    cout << "  static constexpr uint32_t kFixTag = " << spec.tag << ";\n";
    cout << "};\n";
  }

  string makeNamespaceFor(const Msg& msg) {
    string rslt;
    rslt.reserve(2 * msg.name.size());
    auto entry = msg.name.begin();
    JMG_ENFORCE(isupper(*entry),
                "message name ["
                  << msg.name << "] does not start with an uppercase letter");
    rslt.push_back(tolower(*entry));
    ++entry;
    for (; entry != msg.name.end(); ++entry) {
      if (isupper(*entry)) {
        rslt.push_back('_');
        rslt.push_back(tolower(*entry));
      }
      else { rslt.push_back(*entry); }
    }
    return rslt;
  }

  void emitMsg(const Msg& msg) {
    const auto ns = makeNamespaceFor(msg);
    cout << "namespace " << ns << "\n{\n";

    // emit relevant fields in the correct namespace
    for (const auto& fld : msg.fields) { emitField(fld); }
    cout << "} // namespace " << ns << "\n\n";

    // emit the message definition outside the namespace
    const auto [objTypeName, isGroup] = [&]() {
      if ((kHeaderDef == msg.name) || (kTrailerDef == msg.name)) {
        return tuple("jmg::FieldGroupDef"s, true);
      }
      return tuple("jmg::quickfix::Object"s, false);
    }();
    cout << "using " << msg.name << " = " << objTypeName << "<\n";
    if (!isGroup) { cout << "  " << kHeaderDef << ",\n"; }
    auto itr = msg.fields.begin();
    cout << "  " << ns << "::" << itr->name;
    ++itr;
    for (; itr != msg.fields.end(); ++itr) {
      cout << ",\n  " << ns << "::" << itr->name;
    }
    if (!isGroup) { cout << ",\n  " << kTrailerDef << "\n"; }
    cout << "\n>;\n\n";
  }

  // translation from type string in the XML declaration to the
  // appropriate C++ type
  static const Dict<string, string> kTypeTranslation_;
  // set of types for that are represented by strings or single
  // characters
  static const Set<string> kCharFieldTypes_;

  Msg header_;
  Msg trailer_;
  Dict<string, FieldSpec> fields_;
  Dict<string, EnumSpec> enums_;
  vector<Msg> msgs_;
};

const Dict<string, string> AllFixDefs::kTypeTranslation_ = {
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

const Set<string> AllFixDefs::kCharFieldTypes_ = {"CHAR", "STRING", "BOOLEAN",
                                                  "MULTIPLEVALUESTRING"};

void process(const string_view filePath) {
  const auto data = loadXmlData(filePath, "quickfix"sv);

  const AllXmlElements allElements{data};
  JMG_ENFORCE(1 == allElements.size(), "quickfix XML spec should have a single "
                                       "top-level element but actually has ["
                                         << data.size() << "]");

  auto allFixDefs = *(allElements.begin());
  JMG_ENFORCE(kTopLevelTag == jmg::get<ptree::xml::ElementTag>(allFixDefs),
              "quickfix XML spec top-level element should have name [fix] but "
              "actually has ["
                << jmg::get<ptree::xml::ElementTag>(allFixDefs) << "]");

  AllFixDefs fixDefs;
  for (const auto& elem : jmg::get<FixDefinitions>(allFixDefs)) {
    const auto& tag = jmg::get<ptree::xml::ElementTag>(elem);
    if (tag == kFixHeader) { fixDefs.processHeader(elem); }
    else if (tag == kFixMsgs) { fixDefs.processMsgs(elem); }
    else if (tag == kFixTrailer) { fixDefs.processTrailer(elem); }
    else if (tag == kFixComponents) {
      // TODO components appears to be empty for FIX4.2
    }
    else if (tag == kFixFields) { fixDefs.processFields(elem); }
    else {
      cerr << "ERROR: ignoring XML element with tag [" << tag
           << "] in main FIX definitions\n";
    }
  }
  fixDefs.emit();
}

} // namespace quickfix_spec
