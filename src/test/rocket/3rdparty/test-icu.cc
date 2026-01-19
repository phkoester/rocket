/*
 * test-icu.cc
 *
 * Tests related to the ICU library.
 */

#include "rocket-gtest/rocket-gtest.h"

#include <unicode/utf8.h>

// TEST -----------------------------------------------------------------------------------------------------

TEST(icu, U8_NEXT) {
  auto str = "hällo"sv;
  i32 i = 0;
  UChar32 cp;
  U8_NEXT(str.data(), i, str.size(), cp);
  EXPECT_EQ(cp, 'h');
  U8_NEXT(str.data(), i, str.size(), cp);
  EXPECT_EQ(cp, 0xE4); // U+00E4 (LATIN SMALL LETTER A WITH DIAERESIS)
}

// EOF
