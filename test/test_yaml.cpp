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

#include <gmock/gmock.h>

#include "jmg/field.h"
#include "jmg/yaml/yaml.h"

using namespace jmg;
using namespace std;
using namespace std::literals::string_literals;
using testing::ElementsAre;

JMG_SAFE_ID_32(Id32);

JMG_FIELD_DEF(InnerField, "inner", int, true);
using InnerObject = yaml::Object<InnerField>;
using ComplexArrayProxy = yaml::ArrayT<InnerObject>;

JMG_FIELD_DEF(StrField, "str", string, true);
JMG_FIELD_DEF(IntField, "int", int, true);
JMG_FIELD_DEF(OptField, "opt", double, false);
JMG_FIELD_DEF(Id32Field, "id32", Id32, true);
// PrimitiveArray is an array of primitive (i.e. non-object) elements
JMG_FIELD_DEF(PrimitiveArray, "primitive", vector<int>, true);
// ComplexArray is an array of non-primitive (i.e. object) elements
JMG_FIELD_DEF(ComplexArray, "complex", ComplexArrayProxy, true);
// OptComplexArray is an optional array of non-primitive elements
JMG_FIELD_DEF(OptComplexArray, "opt_complex", ComplexArrayProxy, false);

// clang-format off
using TestObj = yaml::Object<StrField, IntField, OptField, Id32Field,
                             PrimitiveArray, ComplexArray, OptComplexArray>;
// clang-format on

TEST(YamlTests, FieldRetrieval) {
  // build initial tree of nodes, some portions of the tree are not
  // constructed until later in order to test the handling of optional
  // fields
  YAML::Node raw;
  raw["str"] = "foo";
  raw["int"] = "42";
  raw["id32"] = "20010911";
  raw["primitive"].push_back("42");
  raw["primitive"].push_back("20010911");
  {
    YAML::Node node;
    node["inner"] = "20010911";
    raw["complex"].push_back(std::move(node));
  }
  {
    YAML::Node node;
    node["inner"] = "42";
    raw["complex"].push_back(std::move(node));
  }

  // construct the jmg object and test basic functionality
  TestObj obj{raw};
  EXPECT_EQ("foo"s, jmg::get<StrField>(obj));
  EXPECT_EQ(42, jmg::get<IntField>(obj));
  EXPECT_EQ(20010911U, unsafe(jmg::get<Id32Field>(obj)));
  {
    const auto& primitive = jmg::get<PrimitiveArray>(obj);
    EXPECT_EQ(2, primitive.size());
    EXPECT_THAT(primitive, ElementsAre(42, 20010911));
  }
  {
    const auto complex = jmg::get<ComplexArray>(obj);
    EXPECT_EQ(2, complex.size());
    auto itr = complex.begin();
    EXPECT_EQ(20010911, jmg::get<InnerField>(*itr));
    ++itr;
    EXPECT_EQ(42, jmg::get<InnerField>(*itr));
    ++itr;
    EXPECT_EQ(itr, complex.end());
  }
  {
    // OptField starts out empty
    const auto empty = jmg::try_get<OptField>(obj);
    EXPECT_FALSE(empty.has_value());
  }
  {
    // OptComplexArray starts out empty
    const auto empty = jmg::try_get<OptComplexArray>(obj);
    EXPECT_FALSE(empty.has_value());
  }

  // populate and test OptField
  raw["opt"] = "-1.0";
  {
    const auto engaged = jmg::try_get<OptField>(obj);
    EXPECT_TRUE(engaged.has_value());
    EXPECT_DOUBLE_EQ(-1.0, *engaged);
  }

  // populate and test OptComplexArray
  {
    YAML::Node node;
    node["inner"] = "42";
    raw["opt_complex"].push_back(std::move(node));
  }
  {
    YAML::Node node;
    node["inner"] = "20010911";
    raw["opt_complex"].push_back(std::move(node));
  }
  {
    const auto opt = jmg::try_get<OptComplexArray>(obj);
    EXPECT_TRUE(opt.has_value());
    const auto& complex = *opt;
    auto itr = complex.begin();
    EXPECT_EQ(42, jmg::get<InnerField>(*itr));
    ++itr;
    EXPECT_EQ(20010911, jmg::get<InnerField>(*itr));
  }
}
