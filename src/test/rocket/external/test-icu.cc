/*
 * test-icu.cc
 *
 * Tests related to the ICU library.
 */

#include "rocket-test/rocket-test.h"

#include <boost/safe_numerics/safe_integer.hpp>

#include <unicode/utf8.h>

using boost::safe_numerics::safe;

// TEST -----------------------------------------------------------------------------------------------------

TEST(icu, u8Next) {
  auto str = "hällo"sv;
  i32 i = 0;
  UChar32 cp = 0;
  U8_NEXT(str.data(), i, safe<i32>(str.size()), cp); // NOLINT
  EXPECT_EQ(cp, 'h');
  U8_NEXT(str.data(), i, safe<i32>(str.size()), cp); // NOLINT
  EXPECT_EQ(cp, 0xE4); // U+00E4 (LATIN SMALL LETTER A WITH DIAERESIS)
}

// EOF
