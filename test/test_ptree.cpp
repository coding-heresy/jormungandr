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

#include <boost/property_tree/ptree.hpp>

#include "jmg/ptree/ptree.h"

using namespace jmg;
using namespace std;
using namespace std::literals::string_literals;

JMG_XML_FIELD_DEF(RecordValue, "value", string, true);
JMG_XML_FIELD_DEF(OptionalRecordValue, "optional_value", string, false);
JMG_XML_FIELD_DEF(RecordValueType, "value_type", string, true);
JMG_XML_FIELD_DEF(TopLevelAttribute, "attribute", string, true);

TEST(PtreeTests, TestXmlPtreeDataRetrieval) {
  namespace pt = boost::property_tree;

  pt::ptree allXmlData;
  {
    pt::ptree xmlTopLevel;
    {
      pt::ptree rec1;
      rec1.put("<xmlattr>.value", "foo");
      rec1.put("<xmlattr>.value_type", "string");
      xmlTopLevel.push_back(pt::ptree::value_type("record", std::move(rec1)));
    }
    {
      pt::ptree rec2;
      rec2.put("<xmlattr>.value", "bar");
      rec2.put("<xmlattr>.value_type", "string");
      rec2.put("<xmlattr>.optional_value", "baz");
      xmlTopLevel.push_back(pt::ptree::value_type("record", std::move(rec2)));
    }
    xmlTopLevel.put("<xmlattr>.attribute", "test");
    allXmlData.push_back(pt::ptree::value_type("top_level", std::move(xmlTopLevel)));
  }

  using namespace jmg::ptree;
  using Record = xml::Object<RecordValue, RecordValueType, OptionalRecordValue>;
  using Records = xml::Elements<Record, xml::ElementsRequired>;
  using TopLevel = xml::Object<TopLevelAttribute, Records>;
  using AllXmlData = xml::ElementsArrayT<TopLevel>;

  AllXmlData allJmgData{allXmlData};
  EXPECT_EQ(1, allJmgData.size());
  size_t ctr1 = 0;
  for (const auto& topLvl : allJmgData) {
    EXPECT_EQ("top_level"s, jmg::get<xml::ElementTag>(topLvl));
    EXPECT_EQ("test"s, jmg::get<TopLevelAttribute>(topLvl));
    {
      size_t ctr2 = 0;
      const auto& recs = jmg::get<Records>(topLvl);
      EXPECT_EQ(2, recs.size());
      for (const auto& rec : recs) {
	EXPECT_EQ("string"s, jmg::get<RecordValueType>(rec));
	if (!ctr2) {
	  EXPECT_EQ("foo"s, jmg::get<RecordValue>(rec));
	  EXPECT_FALSE(jmg::try_get<OptionalRecordValue>(rec).has_value());
	}
	else {
	  EXPECT_EQ("bar"s, jmg::get<RecordValue>(rec));
	  EXPECT_EQ("baz"s, *jmg::try_get<OptionalRecordValue>(rec));
	}
	++ctr2;
      }
      EXPECT_EQ(2, ctr2);
    }
    ++ctr1;
  }
  EXPECT_EQ(1, ctr1);
}
