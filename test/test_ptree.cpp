
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
      for (const auto& rec : jmg::get<Records>(topLvl)) {
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
