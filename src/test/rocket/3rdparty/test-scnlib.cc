/*
 * test-scnlib.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include <scn/scan.h>
#include <scn/regex.h>

using namespace scn;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(scnlib, scanI32) {
  auto result = scan<i32>("123", "{}");
  auto i = result->value();
  EXPECT_EQ(i, 123);
}

TEST(scnlib, scanI128) {
  auto result = scan<i128>("12345678901234567890", "{}");
  auto i = result->value();
  EXPECT_EQ(fmt::format("{}", i), "12345678901234567890");
}

TEST(scnlib, scanRegex) {
  auto result = scan<regex_matches>("abc123", "{:/([a-z]+)([0-9]+)/}");
  const basic_regex_matches<char>& matches = result->value();
  EXPECT_EQ(matches.size(), 3);
  for (size_t i = 0; i < matches.size(); ++i) {
    optional<basic_regex_match<char>> match = matches.at(i);
    if (i == 0) { EXPECT_EQ(match->get(), "abc123"); }
    if (i == 1) { EXPECT_EQ(match->get(), "abc"); }
    if (i == 2) { EXPECT_EQ(match->get(), "123"); }
  }
}

// EOF
