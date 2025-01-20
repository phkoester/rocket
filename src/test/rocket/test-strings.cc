/*
 * test-strings.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/strings.h"

using namespace rocket;
using namespace rocket::strings;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(strings, removeLeading_char) {
  using type = char;

  string s = "";
  EXPECT_EQ(removeLeading<type>(s, "hello"), "");

  s = "hello";
  EXPECT_EQ(removeLeading<type>(s, ""), "hello");

  s = "/dir";
  EXPECT_EQ(removeLeading<type>(s, "/"), "dir");

  s = "//dir";
  EXPECT_EQ(removeLeading<type>(s, "/", 1), "/dir");

  s = "///dir";
  EXPECT_EQ(removeLeading<type>(s, "/"), "dir");
}

TEST(strings, removeLeading_char32_t) {
  using type = char32_t;
  
  u32string s = U"";
  EXPECT_EQ(removeLeading<type>(s, U"hello"), U"");

  s = U"hello";
  EXPECT_EQ(removeLeading<type>(s, U""), U"hello");

  s = U"/dir";
  EXPECT_EQ(removeLeading<type>(s, U"/"), U"dir");

  s = U"//dir";
  EXPECT_EQ(removeLeading<type>(s, U"/", 1), U"/dir");

  s = U"///dir";
  EXPECT_EQ(removeLeading<type>(s, U"/"), U"dir");
}

TEST(strings, removeTrailing_char) {
  using type = char;

  string s = "";
  EXPECT_EQ(removeTrailing<type>(s, "hello"), "");

  s = "hello";
  EXPECT_EQ(removeTrailing<type>(s, ""), "hello");

  s = "dir/";
  EXPECT_EQ(removeTrailing<type>(s, "/"), "dir");

  s = "dir//";
  EXPECT_EQ(removeTrailing<type>(s, "/", 1), "dir/");

  s = "dir///";
  EXPECT_EQ(removeTrailing<type>(s, "/"), "dir");
}

TEST(strings, removeTrailing_char32_t) {
  using type = char32_t;

  u32string s = U"";
  EXPECT_EQ(removeTrailing<type>(s, U"hello"), U"");

  s = U"hello";
  EXPECT_EQ(removeTrailing<type>(s, U""), U"hello");

  s = U"dir//";
  EXPECT_EQ(removeTrailing<type>(s, U"/", 1), U"dir/");

  s = U"dir///";
  EXPECT_EQ(removeTrailing<type>(s, U"/"), U"dir");
}

TEST(strings, replaceIn_char) {
  using type = char;

  string s = "(abc)(abc)(abc)";
  replaceIn<type>(s, "(abc)", "(a)");
  EXPECT_EQ(s, "(a)(a)(a)");
}

TEST(strings, replaceIn_char32_t) {
  using type = char32_t;

  u32string s = U"(abc)(abc)(abc)";
  replaceIn<type>(s, U"(abc)", U"(a)");
  EXPECT_EQ(s, U"(a)(a)(a)");
}

TEST(strings, upper_char) {
  EXPECT_EQ(strings::upper("debug"), "DEBUG");
  EXPECT_EQ(strings::upper("DEBUG"), "DEBUG");
  EXPECT_EQ(strings::upper("äöü"), "ÄÖÜ");
}

TEST(strings, upper_char32_t) {
  EXPECT_EQ(strings::upper(U"debug"), U"DEBUG");
  EXPECT_EQ(strings::upper(U"DEBUG"), U"DEBUG");
  EXPECT_EQ(strings::upper(U"äöü"), U"ÄÖÜ");
}

// EOF
