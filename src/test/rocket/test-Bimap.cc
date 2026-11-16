/*
 * test-Bimap.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Bimap-codec.h"
#include "rocket/scan-Bimap.h"

using namespace rocket;

#define MODULE rocketBimap

// Tests ====================================================================================================

TEST(MODULE, BimapOpLt) {
  const auto map1 = makeBimap<i32, i32>({ { 1, 2 }, { 3, 4 } });
  const auto map2 = makeBimap<i32, i32>({ { 1, 2 }, { 3, 5 } });
  EXPECT_LT(map1, map2);
  EXPECT_GT(map2, map1);
}

TEST(MODULE, UnorderedBimapopEq) {
  const auto map1 = makeUnorderedBimap<i32, i32>({ { 1, 2 }, { 3, 4 } });
  const auto map2 = makeUnorderedBimap<i32, i32>({ { 1, 2 }, { 3, 4 } });
  EXPECT_EQ(map1, map2);
  EXPECT_EQ(map2, map1);
}

TEST(MODULE, UnorderedBimapopNe) {
  const auto map1 = makeUnorderedBimap<i32, i32>({ { 1, 2 } });
  const auto map2 = makeUnorderedBimap<i32, i32>({ { 1, 2 }, { 3, 4 } });
  EXPECT_NE(map1, map2);
  EXPECT_NE(map2, map1);
}

TEST(MODULE, BimapScan) {
  using type = Bimap<i32, f64>;

  // Full input
  {
    const auto val1 = makeBimap<i32, f64>({ { 1, 1.11 }, { 2, 2.22 }, { 3, 3.33 } });
    const string input = "{1: 1.11, 2: 2.22, 3: 3.33}"; // 27 chars
    const auto result = scn::scan<type>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), val1);
    EXPECT_EQ(result->begin() - input.begin(), 27);
  }

  // Whitespace, as for `std::map`
  {
    const auto result = scn::scan<type>("  { 1 : 1.5 ,2:2.5 }", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), (makeBimap<i32, f64>({ { 1, 1.5 }, { 2, 2.5 } })));
  }

  // Empty
  {
    const auto result = scn::scan<type>("{}", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->value().empty());
  }

  // Duplicate keys or values are dropped
  {
    const auto result = scn::scan<type>("{1: 1.5, 1: 2.5, 2: 1.5}", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), (makeBimap<i32, f64>({ { 1, 1.5 } })));
  }

  // Errors
  EXPECT_FALSE((scn::scan<type>("{1: 1.5", "{}")));
  EXPECT_FALSE((scn::scan<type>("{1 1.5}", "{}")));
  EXPECT_FALSE((scn::scan<type>("1: 1.5}", "{}")));
}

TEST(MODULE, UnorderedBimapScan) {
  using type = UnorderedBimap<i32, f64>;

  const auto result = scn::scan<type>("{1: 1.5, 2: 2.5}", "{}");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->value(), (makeUnorderedBimap<i32, f64>({ { 1, 1.5 }, { 2, 2.5 } })));
}

// EOF
