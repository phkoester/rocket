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

ROCKET_ENUM_DECLARE(MyEnum);
ROCKET_ENUM_DECLARE_FMT_FORMATTER(MyEnum);

ROCKET_ENUM_DEFINE(MyEnum, MyEnum, (fröb)(fröber)(fröberer)(pörk)(pörker)(pörkerer));
ROCKET_ENUM_DEFINE_FMT_FORMATTER(, MyEnum, MyEnum);

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(enum, MyEnumOpInput) {
  MyEnum v;

  {
    auto is = io::is("fröb");
    is >> v;
    EXPECT_EQ(v, fröb);
    EXPECT_ISTREAM(is, false, false, 5);
  }

  {
    auto is = io::is("fröbx");
    is >> v;
    EXPECT_EQ(v, fröb);
    EXPECT_ISTREAM(is, false, false, 5);
  }

  {
    auto is = io::is("pörkerer");
    is >> v;
    EXPECT_EQ(v, pörkerer);
    EXPECT_ISTREAM(is, false, false, 9);
  }

  {
    auto is = io::is();
    is >> v;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("frö");
    is >> v;
    EXPECT_ISTREAM(is, true, true, 4);
  }

  {
    auto is = io::is("foo");
    is >> v;
    EXPECT_ISTREAM(is, true, false, 2);
  }
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
