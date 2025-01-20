/*
 * test-enum.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-rocket-decl.h"
#include "rocket/codec-std-decl.h"
#include "rocket/codec-rocket.h"
#include "rocket/codec-std.h"

#include "rocket/enum.h"

#include "rocket-gtest/match.h"

using namespace rocket;
using namespace rocket::gtest::match;
using namespace std;
using namespace testing;

// `MyEnum` -------------------------------------------------------------------------------------------------

enum MyEnum { fröb, fröber, fröberer, pörk, pörker, pörkerer };

ROCKET_ENUM_DEFINE(MyEnum, MyEnum, (fröb)(fröber)(fröberer)(pörk)(pörker)(pörkerer));

ROCKET_ENUM_DECLARE_STD_FORMATTER(MyEnum);
ROCKET_ENUM_DEFINE_STD_FORMATTER(, MyEnum, MyEnum);

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(enum, parse_MyEnum) {
  EXPECT_EQ(codec::ron::parse<MyEnum>("\"fröb\""), fröb);
  EXPECT_EQ(codec::ron::parse<MyEnum>("\"pörkerer\""), pörkerer);

  EXPECT_THAT(
      [&] { codec::ron::parse<MyEnum>(""); },
      throwsParseFailure<char>(0, HasSubstr("EOF")));

  EXPECT_THAT(
      [&] { codec::ron::parse<MyEnum>("\"\""); },
      throwsParseFailure<char>(1, { 1, 2 }, HasSubstr("Expected at least 1 character before '\"', got 0")));

  EXPECT_THAT(
      [&] { codec::ron::parse<MyEnum>("\"foo\""); },
      throwsParseFailure<char>(0, { 0, 5 }, HasSubstr("Cannot parse \"\\\"foo\\\"\" as `MyEnum`")));
}

TEST(enum, print_MyEnum) {
  EXPECT_EQ(codec::ron::print(fröb), "\"fröb\"");
  EXPECT_EQ(codec::ron::print(static_cast<MyEnum>(1)), "\"fröber\"");
  EXPECT_EQ(codec::ron::print(pörkerer), "\"pörkerer\"");

  EXPECT_THAT(
      [] { codec::ron::print(static_cast<MyEnum>(6)); },
      ThrowsMessage<except::InvalidArgument>(HasSubstr("Invalid `MyEnum`: 6")));
}

TEST(enum, opInput_MyEnum) {
  using type = MyEnum;

  type v;

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

TEST(enum, opOutput_MyEnum) {
  using type = MyEnum;

  {
    ostringstream os;
    os << fröb;
    EXPECT_EQ(os.str(), "fröb");
  }

  {
    ostringstream os;
    os << static_cast<type>(1);
    EXPECT_EQ(os.str(), "fröber");
  }
  
  {
    ostringstream os;
    os << static_cast<type>(5);
    EXPECT_EQ(os.str(), "pörkerer");
  }

  {
    ostringstream os;
    EXPECT_THAT(
        [&] { os << static_cast<type>(6); },
        ThrowsMessage<except::InvalidArgument>(HasSubstr("Invalid `MyEnum`: 6")));
  }
}

TEST(enum, parseRon_MyEnum) {
  using type = MyEnum;

  type v;

  {
    auto is = io::is("\"fröb\"");
    parseRon(is, v);
    EXPECT_ISTREAM(is, false, false, 7);
    EXPECT_EQ(v, fröb);
  }

  {
    auto is = io::is("\"pörkerer\"");
    parseRon(is, v);
    EXPECT_ISTREAM(is, false, false, 11);
    EXPECT_EQ(v, pörkerer);
  }

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("\"\"");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 2 }, HasSubstr("Expected at least 1 character before '\"', got 0")));
    EXPECT_ISTREAM(is, true, false, 2);
  }

  {
    auto is = io::is("\"foo\"");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 5 }, HasSubstr("Cannot parse \"\\\"foo\\\"\" as `MyEnum`")));
    EXPECT_ISTREAM(is, true, false, 5);
  }
}

TEST(enum, printRon_MyEnum) {
  using type = MyEnum;

  {
    ostringstream os;
    printRon(os, fröb);
    EXPECT_EQ(os.str(), "\"fröb\"");
  }

  {
    ostringstream os;
    printRon(os, static_cast<type>(1));
    EXPECT_EQ(os.str(), "\"fröber\"");
  }
  
  {
    ostringstream os;
    printRon(os, pörkerer);
    EXPECT_EQ(os.str(), "\"pörkerer\"");
  }

  {
    ostringstream os;
    EXPECT_THAT(
        [&] { os << static_cast<type>(6); },
        ThrowsMessage<except::InvalidArgument>(HasSubstr("Invalid `MyEnum`: 6")));
  }
}

/**
 * The size of the string is 10 here, but the code-point size is not. `std::format` is not UTF-8-aware.
 */
TEST(enum, format_MyEnum) {
  EXPECT_EQ(format("{: >10}", fröber), "   fröber");
}

// EOF
