
#include <gtest/gtest.h>

#include "jmg/safe_types.h"

TEST(SafeTypesTests, IdsAreComparable) {
  JMG_SAFE_ID_32(TestId32);
  TestId32 val1{42};
  TestId32 val2{42};
  TestId32 val3{20010911};

  EXPECT_EQ(val1, val2);
  EXPECT_NE(val1, val3);
}
