/*
 * test-native-int128.cc
 */

#ifndef ROCKET_HAS_BOOST_INT128

#include "rocket-test/rocket-test.h"

#include "rocket/format/format.h"
#include "rocket/io/io.h"

// Types ----------------------------------------------------------------------------------------------------

using int128 = __int128;
using uint128 = unsigned __int128;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(nativeInt128, literal) {
  EXPECT_TRUE((rocket::internal::validateUnsigned<uint128, 10, '1', '2', '3', '4'>()));
  EXPECT_EQ((rocket::internal::makeUnsigned<uint128, 10, '1', '2', '3', '4'>()), 1234);
  EXPECT_EQ(-1234_i128, -1234);
  EXPECT_EQ(1234_u128, 1234);
}

TEST(nativeInt128, opOutput) {
  {
    uint128 value = 1234;
    ostringstream os;
    os << value;
    EXPECT_EQ(os.str(), "1234");
  }

  {
    int128 value = numeric_limits<int128>::min();
    ostringstream os;
    os << value;
    EXPECT_EQ(os.str(), "-170141183460469231731687303715884105728");
  }

  {
    uint128 value = numeric_limits<uint128>::max();
    ostringstream os;
    os << value;
    EXPECT_EQ(os.str(), "340282366920938463463374607431768211455");
  }
}

TEST(nativeInt128, format) {
  // int128 val1 = -1234;
  // EXPECT_EQ(fmt::format("{}", val1), "-1234");
  // uint128 val2 = 1234;
  // EXPECT_EQ(fmt::format("{}", val2), "1234");
}

TEST(nativeInt128, mappedTypeConstant) {
  EXPECT_EQ((fmt::detail::mapped_type_constant<bool, char>::value), fmt::detail::type::bool_type);
  EXPECT_EQ((fmt::detail::mapped_type_constant<string, char>::value), fmt::detail::type::string_type);
  EXPECT_EQ((fmt::detail::mapped_type_constant<void*, char>::value), fmt::detail::type::pointer_type);
  // EXPECT_EQ((fmt::detail::mapped_type_constant<i128, char>::value), fmt::detail::type::int128_type);
  // EXPECT_EQ((fmt::detail::mapped_type_constant<u128, char>::value), fmt::detail::type::uint128_type);
}

#endif // ROCKET_HAS_BOOST_INT128

// EOF
