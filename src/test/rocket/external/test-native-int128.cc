/*
 * test-native-int128.cc
 */

#include "rocket-test/rocket-test.h"

#ifdef ROCKET_HAS_NATIVE_INT128

// Types ----------------------------------------------------------------------------------------------------

using int128 = __int128;
using uint128 = unsigned __int128;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(nativeInt128, literal) {
  EXPECT_TRUE((rocket::internal::validateUnsigned<uint128, 10, '1', '2', '3', '4'>()));
  EXPECT_EQ((rocket::internal::makeUnsigned<uint128, 10, '1', '2', '3', '4'>()), 1234);
}

#endif // ROCKET_HAS_NATIVE_INT128

// EOF
