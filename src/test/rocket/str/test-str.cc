/*
 * test-str.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/str/str.h"

using namespace rocket::str;

// Local functions ------------------------------------------------------------------------------------------

namespace {

vector<vector<string>>
pars(const vector<vector<string>>& v) {
  return v;
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

#define NBSP "\u00A0"

TEST(str, paragraphs) {
  EXPECT_EQ(paragraphs(""), (pars({{}})));
  EXPECT_EQ(paragraphs("a b"), (pars({{ "a", "b" }})));
  EXPECT_EQ(
      paragraphs(NBSP "a" NBSP NBSP "b      cd e" NBSP "f"),
      (pars({{ " a  b", "cd", "e f" }})));
  EXPECT_EQ(paragraphs("a \t\t b"), (pars({{ "a", "b" }})));
  EXPECT_EQ(paragraphs("a\nb c"), (pars({{"a"}, { "b", "c" }})));
  EXPECT_EQ(paragraphs("a\r\nb c"), (pars({{"a"}, { "b", "c" }})));
  EXPECT_EQ(paragraphs("a\n\nb"), (pars({{"a"}, {}, { "b" }})));
  EXPECT_EQ(paragraphs("\na"), (pars({{}, {"a"} })));
}

TEST(str, removeLeadingChar) {
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

TEST(str, removeLeadingChar32) {
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

TEST(str, removeTrailingChar) {
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

TEST(str, removeTrailingChar32) {
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

TEST(str, replaceInChar) {
  using type = char;

  string s = "(abc)(abc)(abc)";
  replaceIn<type>(s, "(abc)", "(a)");
  EXPECT_EQ(s, "(a)(a)(a)");
}

TEST(str, replaceInChar32) {
  using type = char32_t;

  u32string s = U"(abc)(abc)(abc)";
  replaceIn<type>(s, U"(abc)", U"(a)");
  EXPECT_EQ(s, U"(a)(a)(a)");
}

TEST(str, upperChar) {
  EXPECT_EQ(str::upper("debug"), "DEBUG");
  EXPECT_EQ(str::upper("DEBUG"), "DEBUG");
  EXPECT_EQ(str::upper("äöü"), "ÄÖÜ");
}

TEST(str, upperChar32) {
  EXPECT_EQ(str::upper(U"debug"), U"DEBUG");
  EXPECT_EQ(str::upper(U"DEBUG"), U"DEBUG");
  EXPECT_EQ(str::upper(U"äöü"), U"ÄÖÜ");
}

// EOF
