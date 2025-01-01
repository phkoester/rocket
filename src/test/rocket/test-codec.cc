/*
 * test-codec.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/codec.h"
#include "rocket/io.h"

#include "rocket-gtest/match.h"

using namespace rocket;
using namespace rocket::codec;
using namespace rocket::gtest::match;
using namespace std;
using namespace testing;

// 'TEST' ---------------------------------------------------------------------------------------------------

// 'std::istream' utilities .................................................................................

TEST(codec, getBool) {
  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { getBool(io::resetg(is)); },
        throwsParseFailure<char>(0, { 0, 0 }, HasSubstr("\"\" does not match any of {\"0\", \"1\", \"false\", \"true\"}, got EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("0x");
    EXPECT_EQ(getBool(is), false);
    EXPECT_ISTREAM(is, false, false, 1);
  }

  {
    auto is = io::is("tru");
    EXPECT_THAT(
        [&] { getBool(io::resetg(is)); },
        throwsParseFailure<char>(3, { 0, 3 }, HasSubstr("\"tru\" does not match any of {\"0\", \"1\", \"false\", \"true\"}, got EOF")));
    EXPECT_ISTREAM(is, true, true, 3);
  }

  {
    auto is = io::is("true");
    EXPECT_EQ(getBool(is), true);
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("1");
    EXPECT_EQ(getBool(is), true);
    EXPECT_ISTREAM(is, false, false, 1);
  }

  {
    auto is = io::is("falsex");
    EXPECT_EQ(getBool(is), false);
    EXPECT_ISTREAM(is, false, false, 5);
  }
}

TEST(codec, getInteger_int) {
  using type = int;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { getInteger<type>(io::resetg(is)); },
        throwsParseFailure<char>(0, { 0, 1 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, got 0 and EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("-");
    EXPECT_THAT(
        [&] { getInteger<type>(io::resetg(is)); },
        throwsParseFailure<char>(1, { 1, 2 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, got 0 and EOF")));
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::is("-'");
    EXPECT_THAT(
        [&] { getInteger<type>(io::resetg(is)); },
        throwsParseFailure<char>(1, { 1, 2 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, got 0 and '\\''")));
    EXPECT_ISTREAM(is, true, false, 2);
  }

  {
    auto is = io::is("-12");
    EXPECT_EQ(getInteger<type>(is), -12);
    EXPECT_ISTREAM(is, false, false, 3);
  }

  {
    auto is = io::is("-12x");
    EXPECT_EQ(getInteger<type>(is), -12);
    EXPECT_ISTREAM(is, false, false, 3);
  }

  {
    auto is = io::is("-00012");
    EXPECT_EQ(getInteger<type>(is), -12);
    EXPECT_ISTREAM(is, false, false, 6);
  }
}

// RON parsing ..............................................................................................

TEST(codec, parseEnum) {
  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { ron::parsing::parseEnum(io::resetg(is)); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("\"");
    EXPECT_THAT(
        [&] { ron::parsing::parseEnum(io::resetg(is)); },
        throwsParseFailure<char>(1, HasSubstr("Seeking '\"', got EOF")));
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::is("\"\"");
    EXPECT_THAT(
        [&] { ron::parsing::parseEnum(io::resetg(is)); },
        throwsParseFailure<char>(1, { 1, 2 }, HasSubstr("Expected at least 1 character before '\"', got 0")));
    EXPECT_ISTREAM(is, true, false, 2);
  }

  {
    auto is = io::is("\"x\"");
    auto enumResult = ron::parsing::parseEnum(is);
    EXPECT_ISTREAM(is, false, false, 3);
    EXPECT_EQ(enumResult.actualInput, "\"x\"");
    EXPECT_EQ(enumResult.actualInputPos, 0);
    EXPECT_EQ(enumResult.input, "x");
    EXPECT_EQ(enumResult.inputPos, 1);
  }

  {
    auto is = io::is("# abc\n\"red\" \t");
    auto enumResult = ron::parsing::parseEnum(is);
    EXPECT_ISTREAM(is, false, false, 11);
    EXPECT_EQ(enumResult.actualInput, "\"red\"");
    EXPECT_EQ(enumResult.actualInputPos, 6);
    EXPECT_EQ(enumResult.input, "red");
    EXPECT_EQ(enumResult.inputPos, 7);
  }
}

TEST(codec, skip) {
  {
    auto is = io::is();
    ron::parsing::skip(is, false);
    EXPECT_ISTREAM(is, false, true, 0);
  }

  {
    auto is = io::is(" \t\r\n\v ");
    ron::parsing::skip(is, false);
    EXPECT_ISTREAM(is, false, true, 6);
  }

  {
    auto is = io::is(" \n\r # abc\n# def\v");
    ron::parsing::skip(is, false);
    EXPECT_ISTREAM(is, false, true, 16);
  }

  {
    auto is = io::is("# abc\n1");
    ron::parsing::skip(is, false);
    EXPECT_ISTREAM(is, false, false, 6);
    char c = io::getChar(is);
    EXPECT_ISTREAM(is, false, false, 7);
    EXPECT_EQ(c, '1');
  }
}

// RON printing .............................................................................................

TEST(codec, groupByThousands) {
  string s = "";
  ron::printing::groupByThousands(s, 0, s.size());
  EXPECT_EQ(s, "");

  s = "1";
  ron::printing::groupByThousands(s, 0, s.size());
  EXPECT_EQ(s, "1");

  s = "12";
  ron::printing::groupByThousands(s, 0, s.size());
  EXPECT_EQ(s, "12");

  s = "123";
  ron::printing::groupByThousands(s, 0, s.size());
  EXPECT_EQ(s, "123");

  s = "1234";
  ron::printing::groupByThousands(s, 0, s.size());
  EXPECT_EQ(s, "1'234");

  s = "12345";
  ron::printing::groupByThousands(s, 0, s.size());
  EXPECT_EQ(s, "12'345");

  s = "123456";
  ron::printing::groupByThousands(s, 0, s.size());
  EXPECT_EQ(s, "123'456");

  s = "1234567";
  ron::printing::groupByThousands(s, 0, s.size());
  EXPECT_EQ(s, "1'234'567");
}

// EOF
