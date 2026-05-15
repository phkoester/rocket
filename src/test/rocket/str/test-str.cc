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

// #TEST ----------------------------------------------------------------------------------------------------

TEST(str, lines) {
  EXPECT_EQ(lines<char>(""), (vector<string_view>{ "" }));
  EXPECT_EQ(lines<char>("a b"), (vector<string_view>{ "a b" }));
  EXPECT_EQ(lines<char>("a\nb"), (vector<string_view>{ "a", "b" }));
  EXPECT_EQ(lines<char>("a\r\nb"), (vector<string_view>{ "a", "b" }));
  EXPECT_EQ(lines<char>("a\n\nb"), (vector<string_view>{ "a", "", "b" }));
  EXPECT_EQ(lines<char>("\na"), (vector<string_view>{ "", "a" }));
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

TEST(str, split) { // NOLINT(*-complexity)
  int n = 0;
  for (const auto token : split<char>("", ",")) {
    static_assert(is_same_v<decltype(token), const string_view>);
    if (n == 0) { EXPECT_EQ(token, ""); }
    ++n;
  }
  EXPECT_EQ(n, 1);

  n = 0;
  for (const auto token : split<char>(" ", ",")) {
    if (n == 0) { EXPECT_EQ(token, " "); }
    ++n;
  }
  EXPECT_EQ(n, 1);

  n = 0;
  for (const auto token : split<char>(",", ",")) {
    if (n == 0) { EXPECT_EQ(token, ""); }
    if (n == 1) { EXPECT_EQ(token, ""); }
    ++n;
  }
  EXPECT_EQ(n, 2);

  n = 0;
  for (const auto token : split<char>(",,", ",")) {
    if (n == 0) { EXPECT_EQ(token, ""); }
    if (n == 1) { EXPECT_EQ(token, ""); }
    if (n == 2) { EXPECT_EQ(token, ""); }
    ++n;
  }
  EXPECT_EQ(n, 3);


  n = 0;
  for (const auto token : split<char>("||a||b c||||d||||", "||")) {
    if (n == 0) { EXPECT_EQ(token, ""); }
    if (n == 1) { EXPECT_EQ(token, "a"); }
    if (n == 2) { EXPECT_EQ(token, "b c"); }
    if (n == 3) { EXPECT_EQ(token, ""); }
    if (n == 4) { EXPECT_EQ(token, "d"); }
    if (n == 5) { EXPECT_EQ(token, ""); }
    if (n == 6) { EXPECT_EQ(token, ""); }
    ++n;
  }
  EXPECT_EQ(n, 7);
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
