/*
 * test-split.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/str/split.h"

using namespace rocket::str;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(str, split) { // NOLINT(*-complexity)
  int n = 0;
  for (const auto token : split<char>("", ",")) {
    static_assert(same_as<decltype(token), const string_view>);
    if (n == 0) { EXPECT_EQ(token, ""); }
    ++n;
  }
  EXPECT_EQ(n, 1);

  n = 0;
  for (const auto token : split<char>(" ", ",")) {
    if (n == 0) { EXPECT_EQ(token, " "); }
    ++n;
  }
  EXPECT_EQ(n, 1);

  n = 0;
  for (const auto token : split<char>(",", ",")) {
    if (n == 0) { EXPECT_EQ(token, ""); }
    if (n == 1) { EXPECT_EQ(token, ""); }
    ++n;
  }
  EXPECT_EQ(n, 2);

  n = 0;
  for (const auto token : split<char>(",,", ",")) {
    if (n == 0) { EXPECT_EQ(token, ""); }
    if (n == 1) { EXPECT_EQ(token, ""); }
    if (n == 2) { EXPECT_EQ(token, ""); }
    ++n;
  }
  EXPECT_EQ(n, 3);


  n = 0;
  for (const auto token : split<char>("||a||b c||||d||||", "||")) {
    if (n == 0) { EXPECT_EQ(token, ""); }
    if (n == 1) { EXPECT_EQ(token, "a"); }
    if (n == 2) { EXPECT_EQ(token, "b c"); }
    if (n == 3) { EXPECT_EQ(token, ""); }
    if (n == 4) { EXPECT_EQ(token, "d"); }
    if (n == 5) { EXPECT_EQ(token, ""); }
    if (n == 6) { EXPECT_EQ(token, ""); }
    ++n;
  }
  EXPECT_EQ(n, 7);
}

TEST(str, splitEmptySep) {
  const auto throwsEmptySep =
    ThrowsMessage<InvalidArgument>(EndsWith("Parameter `sep`: Check `not sep.empty()` failed"));

  EXPECT_THAT([] { static_cast<void>(split<char>("abc", "").begin()); }, throwsEmptySep);
  EXPECT_THAT([] { static_cast<void>(split<char>("abc", "").end()); }, throwsEmptySep);

  // The input doesn't matter
  EXPECT_THAT([] { static_cast<void>(split<char>("", "").begin()); }, throwsEmptySep);

  EXPECT_THAT([] { static_cast<void>(split<char32>(U"abc", U"").begin()); }, throwsEmptySep);
}

TEST(str, splitPosOutOfBounds) {
  const auto throwsOutOfBounds =
    ThrowsMessage<InvalidArgument>(
      EndsWith("Parameter `pos`: Check `pos == NPOS || pos <= str.size()` failed"));

  EXPECT_THAT([] { SplitIterator<char>("abc", 4, ","); }, throwsOutOfBounds);
  EXPECT_THAT([] { SplitIterator<char>("", 1, ","); }, throwsOutOfBounds);
  EXPECT_THAT([] { SplitIterator<char>("abc", NPOS - 1, ","); }, throwsOutOfBounds);
  EXPECT_THAT([] { SplitIterator<char32>(U"abc", 4, U","); }, throwsOutOfBounds);

  // The largest valid position yields an empty token at the end
  EXPECT_EQ(*SplitIterator<char>("abc", 3, ","), "");
  EXPECT_EQ(*SplitIterator<char>("", 0, ","), "");

  // `NPOS` is valid, too: it makes the end iterator
  EXPECT_FALSE(SplitIterator<char>("abc", NPOS, ",") != split<char>("abc", ",").end());
}

// EOF
