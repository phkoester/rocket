/*
 * test-codec-std.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket-gtest/matcher.h"

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(codec_std, parseRon_optional) {
  using type = optional<char>;

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    v = 'x';
    auto is = io::is("null");
    parseRon(is, v);
    EXPECT_EQ(v, nullopt);
  }

  {
    v = nullopt;
    auto is = io::is("'x'");
    parseRon(is, v);
    EXPECT_EQ(v, 'x');
  }
}

TEST(codec_std, parseRon_pair) {
  using type = pair<pair<int, int>, pair<int, int>>;

  type v;
  auto is = io::is(
    "#\n"
    "# A comment\n"
    "#\n"
    "\n"
    "(\n"
    "  (1 , 2) , # The first element\n"
    "  (1'000 , 2'000) # The second element\n"
    ")this_must_be_ignored");
  parseRon(is, v);
  EXPECT_EQ(v, (type({ 1, 2 }, { 1000, 2000 })));
}

TEST(codec_std, parseRon_string) {
  using type = string;

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("\"a");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure(2, { 0, 2 }, HasSubstr("Missing terminating '\"' character")));
    EXPECT_ISTREAM(is, true, true, 2);
  }

  {
    auto is = io::is("\"\\Q");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure(1, { 1, 3 }, HasSubstr("Invalid escape sequence")));
    EXPECT_ISTREAM(is, true, false, 3);
  }

  {
    auto is = io::is("\"\\u00");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(5, HasSubstr("Expected any of {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'a', 'b', 'c', 'd', 'e', 'f'}, got EOF")));
    EXPECT_ISTREAM(is, true, true, 5);
  }

  {
    auto is = io::is("\"a b\"");
    parseRon(is, v);
    EXPECT_EQ(v, "a b");
  }

  {
    auto is = io::is("  \t # abc\n\"\\U000020acäöü\"");
    parseRon(is, v);
    EXPECT_EQ(v, "€äöü");
  }
}

TEST(codec_std, parseRon_u32string) {
  using type = u32string;

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("\"a");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure(2, { 0, 2 }, HasSubstr("Missing terminating '\"' character")));
    EXPECT_ISTREAM(is, true, true, 2);
  }

  {
    auto is = io::is("\"\\Q");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure(1, { 1, 3 }, HasSubstr("Invalid escape sequence")));
    EXPECT_ISTREAM(is, true, false, 3);
  }

  {
    auto is = io::is("\"\\€");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure(1, { 1, 5 }, HasSubstr("Invalid escape sequence")));
    EXPECT_ISTREAM(is, true, false, 5);
  }

  {
    auto is = io::is("\"\\u00");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(5, HasSubstr("Expected any of {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'a', 'b', 'c', 'd', 'e', 'f'}, got EOF")));
    EXPECT_ISTREAM(is, true, true, 5);
  }

  {
    auto is = io::is("\"a b\"");
    parseRon(is, v);
    EXPECT_EQ(v, U"a b");
  }

  {
    auto is = io::is("  \t # abc\n\"\\U000020acäöü\"");
    parseRon(is, v);
    EXPECT_EQ(v, U"€äöü");
  }
}

TEST(codec_std, parseRon_tuple) {
  using type = tuple<int, string, double>;

  type v;
  auto is = io::is("(12, \"hi\", 2.5)");
  parseRon(is, v);
  EXPECT_EQ(v, make_tuple(12, "hi", 2.5));
}

TEST(codec_std, parseRon_unordered_map) {
  using type = unordered_map<int, string>;

  type v;
  auto is = io::is("{1: \"one\", 2: \"two\", 3: \"three\"}");
  parseRon(is, v);
  EXPECT_EQ(v, (type { { 1, "one" }, { 2, "two" }, { 3, "three" } }));
}

TEST(codec_std, parseRon_unordered_set) {
  using type = unordered_set<int>;

  type v;
  auto is = io::is("{1, 2, 3}");
  parseRon(is, v);
  EXPECT_EQ(v, (type { 3, 2, 1 }));
}

TEST(codec_std, parseRon_variant) {
  using type = variant<int, bool, char>;

  type v;

  {
    auto is = io::is("0 : 2");
    parseRon(is, v);
    EXPECT_EQ(v.index(), 0);
    EXPECT_EQ(std::get<0>(v), 2);
  }

  {
    auto is = io::is("1 : true");
    parseRon(is, v);
    EXPECT_EQ(v.index(), 1);
    EXPECT_EQ(std::get<1>(v), true);
  }

  {
    auto is = io::is("2 : '\x64'"); // One backslash
    parseRon(is, v);
    EXPECT_EQ(v.index(), 2);
    EXPECT_EQ(std::get<2>(v), '\x64');
  }

  {
    auto is = io::is("2 : '\\x64'"); // Two backslashes
    parseRon(is, v);
    EXPECT_EQ(v.index(), 2);
    EXPECT_EQ(std::get<2>(v), '\x64');
  }

  {
    auto is = io::is(" 123: something");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure(1, { 1, 4 }, HasSubstr("Invalid index: 123")));
  }
}

TEST(codec_std, parseRon_vector) {
  using type = vector<pair<bool, int>>;

  type v;

  {
    auto is = io::is("[]");
    v.emplace_back(false, 7);
    parseRon(is, v);
    EXPECT_TRUE(v.empty());
  }

  {
    auto is = io::is("[(false, 0), (true, 1)]");
    parseRon(is, v);
    EXPECT_EQ(v, (type { { false, 0 }, { true, 1 } }));
  }
}

// EOF
