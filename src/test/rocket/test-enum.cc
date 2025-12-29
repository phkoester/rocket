/*
 * test-enum.cc
 */

#include "rocket-gtest/testing.h"

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

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(enum, toType) {
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

TEST(enum, opOutput) {
  ostringstream os;
  os << fröb;
  EXPECT_EQ(os.str(), "fröb");
}

TEST(enum, MyEnumFormat) {
  EXPECT_EQ(fmt::format("{}", fröb), "fröb");
  EXPECT_EQ(fmt::format("{}", fröber), "fröber");
  EXPECT_EQ(fmt::format("{}", fröberer), "fröberer");
  EXPECT_EQ(fmt::format("{}", pörk), "pörk");
  EXPECT_EQ(fmt::format("{}", pörker), "pörker");
  EXPECT_EQ(fmt::format("{}", pörkerer), "pörkerer");
  EXPECT_EQ(fmt::format("{}", static_cast<MyEnum>(10)), "<invalid>");
  EXPECT_EQ(fmt::format("{: >10}", fröber), "    fröber"); // Tests UTF-8 alignment; 4 spaces expected
}

// EOF
