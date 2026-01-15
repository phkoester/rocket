/*
 * test-UnorderedBimap.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/UnorderedBimap.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(UnorderedBimap, opEq) {
  auto map1 = makeUnorderedBimap<int, int>({ { 1, 2 }, { 3, 4 } });
  auto map2 = makeUnorderedBimap<int, int>({ { 1, 2 }, { 3, 4 } });
  EXPECT_EQ(map1, map2);
  EXPECT_EQ(map2, map1);
}

TEST(UnorderedBimap, opNe) {
  auto map1 = makeUnorderedBimap<int, int>({ { 1, 2 } });
  auto map2 = makeUnorderedBimap<int, int>({ { 1, 2 }, { 3, 4 } });
  EXPECT_NE(map1, map2);
  EXPECT_NE(map2, map1);
}

TEST(UnorderedBimap, format) {
  {
    auto map = makeUnorderedBimap<int, int>({ { 1, 2 }, { 3, 4 } });
    EXPECT_EQ(fmt::format("{}", map), "{1: 2, 3: 4}");
    EXPECT_EQ(fmt::format(U"{}", map), U"{1: 2, 3: 4}");
  }

  {
    auto map = makeUnorderedBimap<int, string_view>({ { 1, "one" }, { 2, "two" } });
    EXPECT_EQ(fmt::format("{}", map), "{1: \"one\", 2: \"two\"}");
    // @todo Why are the strings escaped?
    EXPECT_EQ(fmt::format("{::}", map), "{1: \"one\", 2: \"two\"}");
  }
}

// EOF
