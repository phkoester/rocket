/*
 * test-Bimap.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Bimap-codec.h"

#include <fmt/xchar.h>

using namespace rocket;

// #TEST ----------------------------------------------------------------------------------------------------

TEST(Bimap, BimapOpLt) {
  const auto map1 = makeBimap<i32, i32>({ { 1, 2 }, { 3, 4 } });
  const auto map2 = makeBimap<i32, i32>({ { 1, 2 }, { 3, 5 } });
  EXPECT_LT(map1, map2);
  EXPECT_GT(map2, map1);
}

TEST(Bimap, UnorderedBimapopEq) {
  const auto map1 = makeUnorderedBimap<i32, i32>({ { 1, 2 }, { 3, 4 } });
  const auto map2 = makeUnorderedBimap<i32, i32>({ { 1, 2 }, { 3, 4 } });
  EXPECT_EQ(map1, map2);
  EXPECT_EQ(map2, map1);
}

TEST(Bimap, UnorderedBimapopNe) {
  const auto map1 = makeUnorderedBimap<i32, i32>({ { 1, 2 } });
  const auto map2 = makeUnorderedBimap<i32, i32>({ { 1, 2 }, { 3, 4 } });
  EXPECT_NE(map1, map2);
  EXPECT_NE(map2, map1);
}

TEST(Bimap, UnorderedBimapFormat) {
  {
    const auto map = makeUnorderedBimap<i32, i32>({ { 1, 2 }, { 3, 4 } });
    EXPECT_EQ(fmt::format("{}", map), "{1: 2, 3: 4}");
    EXPECT_EQ(fmt::format(U"{}", map), U"{1: 2, 3: 4}");
  }

  {
    const auto map = makeUnorderedBimap<i32, string_view>({ { 1, "one" }, { 2, "two" } });
    EXPECT_EQ(fmt::format("{}", map), "{1: \"one\", 2: \"two\"}");
    EXPECT_EQ(fmt::format("{:i}", map),
      "{\n"
      "  1: \"one\",\n"
      "  2: \"two\"\n"
      "}");
  }
}

// EOF
