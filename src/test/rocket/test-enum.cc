/*
 * test-enum.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/enum.h"

#define MODULE rocketEnum

// `MyEnum` -------------------------------------------------------------------------------------------------

enum MyEnum : u8 { fröb, fröber, fröberer, pörk, pörker, pörkerer };

ROCKET_ENUM_DECLARE(, MyEnum, MyEnum); // NOLINT(*-internal-linkage)
ROCKET_ENUM_DEFINE(, MyEnum, MyEnum, (fröb)(fröber)(fröberer)(pörk)(pörker)(pörkerer));

// `MyEnumClass` --------------------------------------------------------------------------------------------

enum class MyEnumClass : u8 { hürx, hürxer, hürxerer };

ROCKET_ENUM_DECLARE(, MyEnumClass, MyEnumClass); // NOLINT(*-internal-linkage)
ROCKET_ENUM_DEFINE(, MyEnumClass, MyEnumClass, (hürx)(hürxer)(hürxerer));

// `MyEnumInNamespace` --------------------------------------------------------------------------------------

namespace mynamespace {

enum MyEnumInNamespace : u8 { red, green, blue };

} // namespace my_namespace

ROCKET_ENUM_DECLARE(mynamespace, MyEnumInNamespace, MyEnumInNamespace); // NOLINT(*-internal-linkage)
ROCKET_ENUM_DEFINE(mynamespace, MyEnumInNamespace, MyEnumInNamespace, (red)(green)(blue));

// Tests ====================================================================================================

TEST(MODULE, MyEnumToType) {
  EXPECT_EQ(Enum<MyEnum>::toType("fröb", true), make_pair(5_u64, fröb ));
  EXPECT_EQ(Enum<MyEnum>::toType("fröbx", false), make_pair(5_u64, fröb ));
  EXPECT_EQ(Enum<MyEnum>::toType("fröber", true), make_pair(7_u64, fröber));
  EXPECT_EQ(Enum<MyEnum>::toType("fröberer", true), make_pair(9_u64, fröberer));
  EXPECT_EQ(Enum<MyEnum>::toType("pörk", true), make_pair(5_u64, pörk));
  EXPECT_EQ(Enum<MyEnum>::toType("pörker", true), make_pair(7_u64, pörker));
  EXPECT_EQ(Enum<MyEnum>::toType("pörkerer", true), make_pair(9_u64, pörkerer));

  EXPECT_EQ(Enum<MyEnum>::toType("foo", true).error().message , "cannot scan \"foo\" as `MyEnum`");
  EXPECT_EQ(Enum<MyEnum>::toType("fröbx", true).error().message, "cannot scan \"fröbx\" as `MyEnum`");
}

TEST(MODULE, MyEnumClassToType) {
  EXPECT_EQ(Enum<MyEnumClass>::toType("hürxer", true), make_pair(7_u64, MyEnumClass::hürxer));

  EXPECT_EQ(Enum<MyEnumClass>::toType("foo", true).error().message, "cannot scan \"foo\" as `MyEnumClass`");
  EXPECT_EQ(Enum<MyEnumClass>::toType("hürxerx", true).error().message, "cannot scan \"hürxerx\" as `MyEnumClass`");
}

// EOF
