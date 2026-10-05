/*
 * test-Cow.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Cow.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Cow, differentTypes) {
  string str = "hi";
  Cow<string_view, string> cow(str);
  static_assert((std::same_as<decltype(cow.get()), const string_view>));
  static_assert((std::same_as<decltype(cow.owned()), string&>));
  str = "hey";
  EXPECT_EQ(cow.get(), "he");
  EXPECT_FALSE(cow.modified());

  cow = "hello";
  EXPECT_EQ(cow.get(), "hello");
  EXPECT_TRUE(cow.modified());
}

TEST(Cow, sameTypes) {
  i32 n = 3;
  Cow<i32> cow(n);
  static_assert((std::same_as<decltype(cow.get()), const i32&>));
  static_assert((std::same_as<decltype(cow.owned()), i32&>));
  n = 4; // NOLINT
  EXPECT_EQ(cow.get(), 4);
  EXPECT_FALSE(cow.modified());

  cow = 5;
  EXPECT_EQ(cow.get(), 5);
  EXPECT_TRUE(cow.modified());
}

// EOF
