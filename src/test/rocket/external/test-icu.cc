/*
 * test-icu.cc
 *
 * Tests related to the ICU library.
 */

#define ROCKET_TEST_NO_USING_NAMESPACE

#include "rocket-test/rocket-test.h"

#ifdef ROCKET_OS_WINDOWS
  #include <Windows.h>
  #include <icu.h> // Must come after `<Windows.h>`, which defines `NTDDI_VERSION`
#else
  #include <unicode/utf8.h>
  #include <unicode/uversion.h>
#endif

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

  // Require at least ICU 72.1, the system ICU on Windows 11 at the time of writing. `info` holds major,
  // minor, milli, and micro

  EXPECT_TRUE(tuple(info[0], info[1]) >= tuple(72, 1)) <<
    "ICU version " << str.data() << " is older than 72.1";
}

// EOF
