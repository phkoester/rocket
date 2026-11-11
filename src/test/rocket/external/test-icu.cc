/*
 * test-icu.cc
 *
 * Tests related to the ICU library.
 */

#define ROCKET_TEST_NO_USING_NAMESPACE

#include "rocket-test/rocket-test.h"

#include "rocket/icu.h"

#include <boost/safe_numerics/safe_integer.hpp>

#include <array>
#include <tuple>

using namespace std;
using namespace rocket;

using boost::safe_numerics::safe;

// TEST -----------------------------------------------------------------------------------------------------

TEST(icu, u8Next) {
  const auto str = "hällo"sv;
  i32 i = 0;
  UChar32 cp = 0;
  U8_NEXT(str.data(), i, safe<i32>(str.size()), cp); // NOLINT
  EXPECT_EQ(cp, 'h');
  U8_NEXT(str.data(), i, safe<i32>(str.size()), cp); // NOLINT
  EXPECT_EQ(cp, 0xE4); // U+00E4 (LATIN SMALL LETTER A WITH DIAERESIS)
}

TEST(icu, version) {
  // The version of the ICU library loaded at runtime. On Windows, that is the system's `icu.dll`, whose
  // version depends on the Windows build

  UVersionInfo info {};
  u_getVersion(info);
  array<char, U_MAX_VERSION_STRING_LENGTH> str {};
  u_versionToString(info, str.data());

  // Require at least `ROCKET_ICU_VERSION`. `u_versionFromString` fills missing trailing fields with zeros

  constexpr auto minStr = ROCKET_ICU_VERSION;

  UVersionInfo minInfo {};
  u_versionFromString(minInfo, minStr);
  ASSERT_NE(minInfo[0], 0) << "Cannot parse `ROCKET_ICU_VERSION` \"" << minStr << '"';

  // `UVersionInfo` holds major, minor, milli, and micro; compare them lexicographically

  const auto curVer = tuple(info[0], info[1], info[2], info[3]);
  const auto minVer = tuple(minInfo[0], minInfo[1], minInfo[2], minInfo[3]);
  EXPECT_TRUE(curVer >= minVer) << "ICU version " << str.data() << " is older than " << minStr;
}

// EOF
