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

#include "jmg/file_util.h"
#include "jmg/ptree/ptree.h"
#include "jmg/types.h"

#include "spec_util.h"

using namespace jmg;
using namespace std;

namespace xml = ptree::xml;

namespace
{
constexpr string_view kTopLevelTag = "sbe:messageSchema";
constexpr string_view kTypesTag = "types";
constexpr string_view kMsgTag = "sbe:message";
constexpr string_view kIncludeTag = "xi:include";
////////////////////////////////////////////////////////////////////////////////
// primitive type constants
////////////////////////////////////////////////////////////////////////////////
constexpr string_view kCharType = "char";
// signed integers
constexpr string_view kInt8Type = "int8";
constexpr string_view kInt16Type = "int16";
constexpr string_view kInt32Type = "int32";
constexpr string_view kInt64Type = "int64";
// unsigned integers
constexpr string_view kUInt8Type = "uint8";
constexpr string_view kUInt16Type = "uint16";
constexpr string_view kUInt32Type = "uint32";
constexpr string_view kUnt64Type = "uint64";
// floating point numbers
constexpr string_view kFloatType = "float";
constexpr string_view kDoubleType = "double";
} // namespace

namespace sbe_spec
{

////////////////////////////////////////////////////////////////////////////////
// field definitions for SBE message schema elements
////////////////////////////////////////////////////////////////////////////////

// common fields
JMG_XML_FIELD_DEF(Name, "name", string, true);
JMG_XML_FIELD_DEF(Id, "id", string, true);
JMG_XML_FIELD_DEF(Description, "description", string, false);
JMG_XML_FIELD_DEF(SemanticType, "semanticType", string, true);
JMG_XML_FIELD_DEF(Presence, "presence", string, false);
JMG_XML_FIELD_DEF(InitialVersion, "sinceVersion", string, false);
JMG_XML_FIELD_DEF(BlockSz, "blockLength", string, true);
JMG_XML_FIELD_DEF(TypeFld, "type", string, true);
// type element fields
JMG_XML_FIELD_DEF(PrimitiveType, "primitiveType", string, true);
JMG_XML_FIELD_DEF(TypeSz, "length", string, false);
JMG_XML_FIELD_DEF(CharEncoding, "characterEncoding", string, false);
// enum element fields
JMG_XML_FIELD_DEF(EncodingType, "encodingType", string, true);
// SBE field element fields
JMG_XML_FIELD_DEF(ValueRef, "valueRef", string, false);
JMG_XML_FIELD_DEF(Offset, "offset", string, false);
// SBE group element fields
JMG_XML_FIELD_DEF(DimensionType, "dimensionType", string, true);
// include element field
JMG_XML_FIELD_DEF(Href, "href", string, true);

////////////////////////////////////////////////////////////////////////////////
// objects associated with SBE message schema elements
////////////////////////////////////////////////////////////////////////////////

// fields common to all elements
using Common1 = FieldGroupDef<Name, Description>;
#if defined(DBG_RECURSIVE_GROUPS_WORKS)
using Common2 = FieldGroupDef<Common1, Presence, SemanticType>;
using Common3 = FieldGroupDef<Common1, Id>;
using Common4 = FieldGroupDef<Common3, SemanticType>;
#else
using Common2 = FieldGroupDef<Name, Description, Presence, SemanticType>;
using Common3 = FieldGroupDef<Name, Description, Id>;
using Common4 = FieldGroupDef<Name, Description, Id, SemanticType>;
#endif

// type element
using SbeType = xml::Object<Common2, PrimitiveType, TypeSz, CharEncoding>;
// array of types
using SbeTypes = xml::Elements<SbeType, xml::ElementsRequired>;

// valid value element
using SbeValidValue = xml::Object<Name, Description>; // TODO: add value field
// array of valid values
using SbeValidValues = xml::Elements<SbeValidValue, xml::ElementsRequired>;

// enum element
using SbeEnum = xml::Object<Common2, EncodingType, SbeValidValues>;

// choice element for set
using SbeSetChoice =
  xml::Object<Name, Description>; // TODO: add bit position field
// array of choice elements
using SbeSetChoices = xml::Elements<SbeSetChoice, xml::ElementsRequired>;

// set element
using SbeSet = xml::Object<Common2, EncodingType, SbeSetChoices>;

// composite element
using SbeComposite = xml::Object<Common1, SemanticType, SbeTypes>;

// fields that make up the 'types' section
using AnySbeCustomType =
  Union<boost::property_tree::ptree, SbeType, SbeEnum, SbeComposite, SbeSet>;

struct SbeCustomType : FieldDef<AnySbeCustomType, kPlaceholder, Required> {};

using SbeCustomTypeSection = xml::Object<SbeCustomType>;

// array of custom types
using SbeCustomTypes = xml::Elements<SbeCustomTypeSection>;

// message element
using SbeMsg = xml::Object<Common4, BlockSz>;

// field element
using SbeField =
  xml::Object<Common4, TypeFld, ValueRef, Offset, InitialVersion>;

// group element
using GroupElem = xml::Object<Common3, DimensionType, BlockSz>;

// data element
using DataElem = xml::Object<Common4, TypeFld, InitialVersion>;

using IncludeDirective = xml::Object<Href>;

// union that holds one of the following
//  1. an 'include' directive
//  2. a set of SBE types
//  3. an SBE message
using AnyTopLevelSbeDef =
  Union<boost::property_tree::ptree, IncludeDirective, SbeCustomTypes, SbeMsg>;

struct TopLevelSbeDef : FieldDef<AnyTopLevelSbeDef, kPlaceholder, Required> {};

using TopLevelSbeDefSection = xml::Object<TopLevelSbeDef>;

using SbeDefs = xml::Elements<TopLevelSbeDefSection, xml::ElementsRequired>;

// top-level message schema element
// TODO: add package, version, semantic version and byteOrder
using MsgSchema = xml::Object<Id, Description, SbeDefs>;

using AllXmlElements = xml::ElementsArrayT<MsgSchema>;

/**
 * singleton object where all data is stored during parsing, later
 * used to emit output
 */
class AllSbeDefs {
public:
  void processTypes(const auto& typesElement) {
#if 0
    HERE HERE HERE
#endif
  }

private:
  enum class Presence { constant, required, optional };
  struct CustomTypeSpec {
    Presence presence = Presence::required;
    optional<string> semanticType;
    optional<string> description;
    string primitiveType;
    uint16_t sz;
    optional<string> characterEncoding;
  };
  Dict<string, CustomTypeSpec> types;
};

void process(const string_view filePath) {
  const auto data = loadXmlData(filePath, "quickfix"sv);

  const auto allElements = AllXmlElements(data);
  JMG_ENFORCE(1 == allElements.size(),
              "SBE XML spec should have a single "
              "top-level element but actually has [",
              allElements.size(), "]");

  auto schema = *(allElements.begin());
  JMG_ENFORCE(kTopLevelTag == jmg::get<xml::ElementTag>(schema),
              "SBE XML spec top-level element should have name [", kTopLevelTag,
              "] but actually has [", jmg::get<xml::ElementTag>(schema), "]");
  {
    const auto dsc = jmg::try_get<Description>(schema);
#if defined(DBG_RANDOM)
    cout << "DBG schema ID [" << jmg::get<Id>(schema) << "]";
    if (dsc.has_value()) { cout << " description [" << *dsc << "]"; }
    cout << "\n";
#endif
  }
  AllSbeDefs sbeDefs;
  for (const auto& elem : jmg::get<SbeDefs>(schema)) {
    const auto& tag = jmg::get<ptree::xml::ElementTag>(elem);
    if (tag == kTypesTag) { sbeDefs.processTypes(elem); }
    else if (tag == kMsgTag) { cout << "DBG found message def\n"; }
    else if (tag == kIncludeTag) { cout << "DBG found include directive\n"; }
    else {
      cerr << "ERROR: ignoring XML element with tag [" << tag
           << "] in main SBE schema elements\n";
    }
  }
}

} // namespace sbe_spec
