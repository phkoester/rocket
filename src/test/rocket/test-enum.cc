/*
 * test-enum.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/enum.h"

using namespace rocket;
using namespace std;
using namespace testing;

// `MyEnum` -------------------------------------------------------------------------------------------------

enum MyEnum { fröb, fröber, fröberer, pörk, pörker, pörkerer };

ROCKET_ENUM_DECLARE_LOCAL(MyEnum);
ROCKET_ENUM_DECLARE_GLOBAL(MyEnum);

ROCKET_ENUM_DEFINE_LOCAL(MyEnum, MyEnum, (fröb)(fröber)(fröberer)(pörk)(pörker)(pörkerer));
ROCKET_ENUM_DEFINE_GLOBAL(, MyEnum, MyEnum);

// `MyEnumClass` --------------------------------------------------------------------------------------------

enum class MyEnumClass { hürx, hürxer, hürxerer };

ROCKET_ENUM_DECLARE_LOCAL(MyEnumClass);
ROCKET_ENUM_DECLARE_GLOBAL(MyEnumClass);

ROCKET_ENUM_DEFINE_LOCAL(MyEnumClass, MyEnumClass, (hürx)(hürxer)(hürxerer));
ROCKET_ENUM_DEFINE_GLOBAL(, MyEnumClass, MyEnumClass);

// `MyEnumInNamespace` --------------------------------------------------------------------------------------

namespace my_namespace {

enum MyEnumInNamespace { red, green, blue };

ROCKET_ENUM_DECLARE_LOCAL(MyEnumInNamespace);

} // namespace my_namespace

ROCKET_ENUM_DECLARE_GLOBAL(my_namespace::MyEnumInNamespace);

namespace my_namespace {

ROCKET_ENUM_DEFINE_LOCAL(MyEnumInNamespace, MyEnumInNamespace, (red)(green)(blue));

} // namespace my_namespace

ROCKET_ENUM_DEFINE_GLOBAL(my_namespace, MyEnumInNamespace, MyEnumInNamespace);

// `TEST` ---------------------------------------------------------------------------------------------------

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
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"foo\" as `MyEnum`")));
  EXPECT_THAT(
      [] { Enum<MyEnum>::toType("fröbx"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"fröbx\" as `MyEnum`")));
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
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"foo\" as `MyEnumClass`")));
  EXPECT_THAT(
      [] { Enum<MyEnumClass>::toType("hürxerx"); },
      ThrowsMessage<InvalidState>(HasSubstr("Cannot parse \"hürxerx\" as `MyEnumClass`")));
}

// EOF
