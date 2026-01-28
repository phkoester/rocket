/*
 * test-enum.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/enum.h"

// #MyEnum --------------------------------------------------------------------------------------------------

enum MyEnum { fröb, fröber, fröberer, pörk, pörker, pörkerer };

ROCKET_ENUM_DECLARE(, MyEnum, MyEnum);
ROCKET_ENUM_DEFINE(, MyEnum, MyEnum, (fröb)(fröber)(fröberer)(pörk)(pörker)(pörkerer));

// #MyEnumClass ---------------------------------------------------------------------------------------------

enum class MyEnumClass { hürx, hürxer, hürxerer };

ROCKET_ENUM_DECLARE(, MyEnumClass, MyEnumClass);
ROCKET_ENUM_DEFINE(, MyEnumClass, MyEnumClass, (hürx)(hürxer)(hürxerer));

// #MyEnumInNamespace ---------------------------------------------------------------------------------------

namespace mynamespace {

enum MyEnumInNamespace { red, green, blue };

} // namespace my_namespace

ROCKET_ENUM_DECLARE(mynamespace, MyEnumInNamespace, MyEnumInNamespace);
ROCKET_ENUM_DEFINE(mynamespace, MyEnumInNamespace, MyEnumInNamespace, (red)(green)(blue));

// #TEST ----------------------------------------------------------------------------------------------------

TEST(enum, MyEnumOpOutput) {
  ostringstream os;
  os << fröb;
  EXPECT_EQ(os.str(), "fröb");
}

TEST(enum, MyEnumFormat) {
  EXPECT_EQ(fmt::format("{}", fröber), "fröber");
  EXPECT_EQ(fmt::format("{}", fröberer), "fröberer");
  EXPECT_EQ(fmt::format("{}", pörk), "pörk");
  EXPECT_EQ(fmt::format("{}", pörker), "pörker");
  EXPECT_EQ(fmt::format("{}", pörkerer), "pörkerer");
  EXPECT_EQ(fmt::format("{}", static_cast<MyEnum>(10)), "<invalid>");
  EXPECT_EQ(fmt::format("{: >10}", fröber), "    fröber"); // Tests UTF-8 alignment; 4 spaces expected

  EXPECT_EQ(fmt::format(U"{}", fröb), U"fröb");
  EXPECT_EQ(fmt::format(U"{}", static_cast<MyEnum>(10)), U"<invalid>");
}

TEST(enum, MyEnumToType) {
  EXPECT_EQ(Enum<MyEnum>::toType("fröb"), fröb);
  EXPECT_EQ(Enum<MyEnum>::toType("fröber"), fröber);
  EXPECT_EQ(Enum<MyEnum>::toType("fröberer"), fröberer);
  EXPECT_EQ(Enum<MyEnum>::toType("pörk"), pörk);
  EXPECT_EQ(Enum<MyEnum>::toType("pörker"), pörker);
  EXPECT_EQ(Enum<MyEnum>::toType("pörkerer"), pörkerer);

  EXPECT_THAT(
    [] { Enum<MyEnum>::toType("foo"); },
    ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"foo\" as `MyEnum`"))
  );
  EXPECT_THAT(
    [] { Enum<MyEnum>::toType("fröbx"); },
    ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"fröbx\" as `MyEnum`"))
  );
}

TEST(enum, MyEnumClassOpOutput) {
  ostringstream os;
  os << MyEnumClass::hürx;
  EXPECT_EQ(os.str(), "hürx");
}

TEST(enum, MyEnumFClassFormat) {
  EXPECT_EQ(fmt::format("{}", MyEnumClass::hürx), "hürx");
}

TEST(enum, MyEnumClassToType) {
  EXPECT_EQ(Enum<MyEnumClass>::toType("hürxer"), MyEnumClass::hürxer);

  EXPECT_THAT(
    [] { Enum<MyEnumClass>::toType("foo"); },
    ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"foo\" as `MyEnumClass`")));
  EXPECT_THAT(
    [] { Enum<MyEnumClass>::toType("hürxerx"); },
    ThrowsMessage<InvalidState>(HasSubstr("Cannot scan \"hürxerx\" as `MyEnumClass`")));
}

// EOF
