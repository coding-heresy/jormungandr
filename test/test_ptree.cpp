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

#include <gtest/gtest.h>

#include <filesystem>

#include <boost/property_tree/xml_parser.hpp>

#include "jmg/ptree/ptree.h"

#include "jmg/file_util.h"

using namespace jmg;
using namespace std;
using namespace std::literals::string_literals;

namespace vws = std::views;
namespace pt = boost::property_tree;

JMG_XML_STR_FIELD_DEF(RecordValue, "value", true);
JMG_XML_STR_FIELD_DEF(OptionalRecordValue, "optional_value", false);
JMG_XML_STR_FIELD_DEF(RecordValueType, "value_type", true);
JMG_XML_STR_FIELD_DEF(TopLevelAttribute, "attribute", true);

constexpr string_view kXmlText = R"(
<top_level attribute="test">
  <record value="foo" value_type="string"/>
  <record value="bar" value_type="string" optional_value="baz"/>
</top_level>
)";

using namespace jmg::ptree;
using Record = xml::Object<RecordValue, RecordValueType, OptionalRecordValue>;
using Records = xml::Elements<Record, xml::ElementsRequired>;
using TopLevel = xml::Object<TopLevelAttribute, Records>;
using AllXmlData = xml::ElementsArrayT<TopLevel>;

/**
 * macro that defines the verification code which is run once on an
 * object constructed in code and once on an object read from a file
 */
#define VERIFY_LOADED_DATA(all_xml_data)                                      \
  do {                                                                        \
    AllXmlData all_jmg_data{all_xml_data};                                    \
    EXPECT_EQ(1, all_jmg_data.size());                                        \
    size_t ctr = 0;                                                           \
    for (const auto& top_lvl : all_jmg_data) {                                \
      EXPECT_EQ("top_level"s, jmg::get<xml::ElementTag>(top_lvl));            \
      EXPECT_EQ("test"s, jmg::get<TopLevelAttribute>(top_lvl));               \
      {                                                                       \
        const auto& recs = jmg::get<Records>(top_lvl);                        \
        EXPECT_EQ(2U, recs.size());                                           \
        for (auto&& [idx, rec] : vws::enumerate(recs)) {                      \
          EXPECT_EQ("string"sv, jmg::get<RecordValueType>(rec));              \
          if (!idx) {                                                         \
            EXPECT_EQ("foo"sv, jmg::get<RecordValue>(rec));                   \
            EXPECT_FALSE(jmg::try_get<OptionalRecordValue>(rec).has_value()); \
          }                                                                   \
          else {                                                              \
            EXPECT_EQ("bar"s, jmg::get<RecordValue>(rec));                    \
            EXPECT_EQ("baz"s, *jmg::try_get<OptionalRecordValue>(rec));       \
          }                                                                   \
        }                                                                     \
      }                                                                       \
      ++ctr;                                                                  \
    }                                                                         \
    /* verify that iteration actually occurred */                             \
    EXPECT_EQ(1, ctr);                                                        \
  } while (0)

TEST(PtreeTests, TestXmlPtreeDataRetrievalFromConstructedObject) {
  pt::ptree all_xml_data;
  {
    // construct a ptree representation equivalent to kXmlText
    pt::ptree xml_top_level;
    {
      pt::ptree rec1;
      rec1.put("<xmlattr>.value", "foo");
      rec1.put("<xmlattr>.value_type", "string");
      xml_top_level.push_back(pt::ptree::value_type("record", std::move(rec1)));
    }
    {
      pt::ptree rec2;
      rec2.put("<xmlattr>.value", "bar");
      rec2.put("<xmlattr>.value_type", "string");
      rec2.put("<xmlattr>.optional_value", "baz");
      xml_top_level.push_back(pt::ptree::value_type("record", std::move(rec2)));
    }
    xml_top_level.put("<xmlattr>.attribute", "test");
    all_xml_data.push_back(pt::ptree::value_type("top_level",
                                                 std::move(xml_top_level)));
  }
  VERIFY_LOADED_DATA(all_xml_data);
}

TEST(PtreeTests, TestXmlPtreeDataRetrievalFromFile) {
  pt::ptree all_xml_data;
  TmpFile xml_file{kXmlText};
  pt::xml_parser::read_xml(string(xml_file.name()).c_str(), all_xml_data);

  VERIFY_LOADED_DATA(all_xml_data);
}
