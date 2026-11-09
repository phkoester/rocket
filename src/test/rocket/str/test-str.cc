/*
 * test-str.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/str/str.h"

using namespace rocket::str;

namespace {

// Local functions ------------------------------------------------------------------------------------------

vector<vector<string>>
pars(const vector<vector<string>>& vec) {
  return vec;
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(str, lines) {
  EXPECT_EQ(lines(""), (vector<string_view>{ "" }));
  EXPECT_EQ(lines("a b"), (vector<string_view>{ "a b" }));
  EXPECT_EQ(lines("a\nb"), (vector<string_view>{ "a", "b" }));
  EXPECT_EQ(lines("a\r\nb"), (vector<string_view>{ "a", "b" }));
  EXPECT_EQ(lines("a\n\nb"), (vector<string_view>{ "a", "", "b" }));
  EXPECT_EQ(lines("\na"), (vector<string_view>{ "", "a" }));
}

TEST(str, lowerIn) {
  u32string str = U"ÄÖÜ";
  lowerIn(str);
  EXPECT_EQ(str, U"äöü");
}

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

  string_view str;
  EXPECT_EQ(removeLeading<type>(str, "hello"), "");

  str = "hello";
  EXPECT_EQ(removeLeading<type>(str, ""), "hello");

  str = "/dir";
  EXPECT_EQ(removeLeading<type>(str, "/"), "dir");

  str = "//dir";
  EXPECT_EQ(removeLeading<type>(str, "/", 1), "/dir");

  str = "///dir";
  EXPECT_EQ(removeLeading<type>(str, "/"), "dir");
}

TEST(str, removeLeadingChar32) {
  using type = char32;

  u32string_view str;
  EXPECT_EQ(removeLeading<type>(str, U"hello"), U"");

  str = U"hello";
  EXPECT_EQ(removeLeading<type>(str, U""), U"hello");

  str = U"/dir";
  EXPECT_EQ(removeLeading<type>(str, U"/"), U"dir");

  str = U"//dir";
  EXPECT_EQ(removeLeading<type>(str, U"/", 1), U"/dir");

  str = U"///dir";
  EXPECT_EQ(removeLeading<type>(str, U"/"), U"dir");
}

TEST(str, removeTrailingChar) {
  using type = char;

  string_view str;
  EXPECT_EQ(removeTrailing<type>(str, "hello"), "");

  str = "hello";
  EXPECT_EQ(removeTrailing<type>(str, ""), "hello");

  str = "dir/";
  EXPECT_EQ(removeTrailing<type>(str, "/"), "dir");

  str = "dir//";
  EXPECT_EQ(removeTrailing<type>(str, "/", 1), "dir/");

  str = "dir///";
  EXPECT_EQ(removeTrailing<type>(str, "/"), "dir");
}

TEST(str, removeTrailingChar32) {
  using type = char32;

  u32string_view str;
  EXPECT_EQ(removeTrailing<type>(str, U"hello"), U"");

  str = U"hello";
  EXPECT_EQ(removeTrailing<type>(str, U""), U"hello");

  str = U"dir//";
  EXPECT_EQ(removeTrailing<type>(str, U"/", 1), U"dir/");

  str = U"dir///";
  EXPECT_EQ(removeTrailing<type>(str, U"/"), U"dir");
}

TEST(str, replaceIn) {
  string str = "gamma gamma gamma gamma";
  EXPECT_EQ(replaceIn(str, "gamma", "beta"), 4);
  EXPECT_EQ(str, "beta beta beta beta");
}

TEST(str, trim) {
  EXPECT_EQ(trim(""sv), ""sv);
  EXPECT_EQ(trim("\t\n\v\f\r "sv), ""sv);
  EXPECT_EQ(trim("abc"sv), "abc"sv);
  EXPECT_EQ(trim(" abc "sv), "abc"sv);
  EXPECT_EQ(trim(" \t\na b c\v\f "sv), "a b c"sv);
}

TEST(str, trimLeading) {
  EXPECT_EQ(trimLeading(""sv), ""sv);
  EXPECT_EQ(trimLeading("\t\n\v\f\r "sv), ""sv);
  EXPECT_EQ(trimLeading("abc"sv), "abc"sv);
  EXPECT_EQ(trimLeading(" abc "sv), "abc "sv);
  EXPECT_EQ(trimLeading(" \t\na b c\v\f "sv), "a b c\v\f "sv);
}

TEST(str, trimTrailing) {
  EXPECT_EQ(trimTrailing(""sv), ""sv);
  EXPECT_EQ(trimTrailing("\t\n\v\f\r "sv), ""sv);
  EXPECT_EQ(trimTrailing("abc"sv), "abc"sv);
  EXPECT_EQ(trimTrailing(" abc "sv), " abc"sv);
  EXPECT_EQ(trimTrailing(" \t\na b c\v\f "sv), " \t\na b c"sv);
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

TEST(str, upperIn) {
  u32string str = U"äöü";
  upperIn(str);
  EXPECT_EQ(str, U"ÄÖÜ");
}

// EOF
