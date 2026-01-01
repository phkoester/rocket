/*
 * test-container.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/container/container.h"

using namespace rocket;
using namespace rocket::container;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(UnorderedMap, opEq) {
  auto map1 = makeUnorderedBimap<int, int>({ { 1, 2 }, { 3, 4 } });
  auto map2 = makeUnorderedBimap<int, int>({ { 1, 2 }, { 3, 4 } });
  EXPECT_EQ(map1, map2);
  EXPECT_EQ(map2, map1);
}

TEST(UnorderedMap, opNe) {
  auto map1 = makeUnorderedBimap<int, int>({ { 1, 2 } });
  auto map2 = makeUnorderedBimap<int, int>({ { 1, 2 }, { 3, 4 } });
  EXPECT_NE(map1, map2);
  EXPECT_NE(map2, map1);
}

// EOF
