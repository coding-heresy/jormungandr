
#include <gtest/gtest.h>

#include "jmg/conversion.h"

using namespace jmg;
using namespace std;
using namespace std::literals::string_literals;

TEST(ConversionTests, TestStringFromString) {
  const string_view src = "foo";
  EXPECT_EQ("foo"s, static_cast<string>(from_string(src)));
}

TEST(ConversionTests, TestIntFromString) {
  EXPECT_EQ(42, static_cast<int>(from_string("42")));
}

TEST(ConversionTests, TestDoubleFromString) {
  EXPECT_DOUBLE_EQ(0.5, static_cast<double>(from_string("0.5")));
}

TEST(ConversionTests, TestFromStringOverloading) {
  const string_view str = "42";
  const int intVal = from_string(str);
  EXPECT_EQ(42, intVal);
  const double dblVal = from_string(str);
  EXPECT_DOUBLE_EQ(42.0, dblVal);
}

TEST(ConversionTests, TestFailedIntFromString) {
  EXPECT_THROW([[maybe_unused]] int bad = from_string("a"), std::runtime_error);
}
