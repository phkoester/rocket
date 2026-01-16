/*
 * test-Cow.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Cow.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Cow, differentTypes) {
  string s = "hi";
  Cow<string_view, string> cow(s);
  s = "hey";
  EXPECT_EQ(cow.get(), "he");
  EXPECT_FALSE(cow.modified());

  cow = "hello";
  EXPECT_EQ(cow.get(), "hello");
  EXPECT_TRUE(cow.modified());
}

TEST(Cow, sameTypes) {
  i32 n = 3;
  Cow<i32> cow(n);
  n = 4;
  EXPECT_EQ(cow.get(), 4);
  EXPECT_FALSE(cow.modified());

  cow = 5;
  EXPECT_EQ(cow.get(), 5);
  EXPECT_TRUE(cow.modified());
}

// EOF
