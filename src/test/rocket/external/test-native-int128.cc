/*
 * test-native-int128.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/io/io.h"

#ifdef ROCKET_HAS_NATIVE_INT128

// Types ----------------------------------------------------------------------------------------------------

using int128 = __int128;
using uint128 = unsigned __int128;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(nativeInt128, literal) {
  EXPECT_TRUE((rocket::internal::validateUnsigned<uint128, 10, '1', '2', '3', '4'>()));
  EXPECT_EQ((rocket::internal::makeUnsigned<uint128, 10, '1', '2', '3', '4'>()), 1234);
}

TEST(nativeInt128, opOutput) {
  uint128 value = 1234;
  ostringstream os;
  os << value;
  EXPECT_EQ(os.str(), "1234");

  value = rocket::internal::UnsignedLimit<uint128>::value;
  os.str("");
  os << value;
  EXPECT_EQ(os.str(), "340282366920938463463374607431768211455");

  auto max = numeric_limits<uint128>::max();
  cout << "max=" << max << endl;
}

#endif // ROCKET_HAS_NATIVE_INT128

// EOF
