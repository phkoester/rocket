/*
 * test-codec-std.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/S.h"

#include "rocket-gtest/match.h"

using namespace rocket;
using namespace rocket::gtest::match;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(codec_std, printRon_byte) {
  using type = byte;

  EXPECT_EQ(S << type(-1), "ff");
  EXPECT_EQ(S << type(0), "00");
  EXPECT_EQ(S << type(128), "80");
  EXPECT_EQ(S << type(255), "ff");
}

TEST(codec_std, printRon_string) {
  EXPECT_EQ(S << "a€b", "a€b");
  EXPECT_EQ(S << string_view("a€b"), "\"a€b\"");
  EXPECT_EQ(S << string("a€b"), "\"a€b\"");

  EXPECT_EQ(S << U"a€b", "a€b");
  EXPECT_EQ(S << u32string_view(U"a€b"), "\"a€b\"");
  EXPECT_EQ(S << u32string(U"a€b"), "\"a€b\"");

  EXPECT_EQ(S << 'c' << string_view("hi"), "'c'\"hi\"");
  EXPECT_EQ(S << raw('c') << raw(string_view("hi")), "chi");
  EXPECT_EQ(S << raw('a') << 'b', "a'b'");
  EXPECT_EQ(S << raw("hello\nworld"), "hello\nworld");
  EXPECT_EQ(S << "hello\nworld", "hello\nworld");
}

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

TEST(codec_std, printRon_optional) {
  EXPECT_EQ(S << optional<int>(), "null");
  EXPECT_EQ(S << optional<char>('a'), "'a'");
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
        throwsParseFailure<char>(2, { 0, 2 }, HasSubstr("Missing terminating '\"' character")));
    EXPECT_ISTREAM(is, true, true, 2);
  }

  {
    auto is = io::is("\"\\Q");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 3 }, HasSubstr("Invalid escape sequence")));
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
        throwsParseFailure<char>(2, { 0, 2 }, HasSubstr("Missing terminating '\"' character")));
    EXPECT_ISTREAM(is, true, true, 2);
  }

  {
    auto is = io::is("\"\\Q");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 3 }, HasSubstr("Invalid escape sequence")));
    EXPECT_ISTREAM(is, true, false, 3);
  }

  {
    auto is = io::is("\"\\€");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 5 }, HasSubstr("Invalid escape sequence")));
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

TEST(codec_std, printRon_pair_string_view) {
  using type = pair<int, optional<string_view>>;
  
  EXPECT_EQ(S << type(), "(0, null)");
  EXPECT_EQ(S << type(12, "hi"), "(12, \"hi\")");
}

TEST(codec_std, printRon_pair_u32string_view) {
  using type = pair<int, optional<u32string_view>>;
  
  EXPECT_EQ(S << type(), "(0, null)");
  EXPECT_EQ(S << type(12, U"hi"), "(12, \"hi\")");
}

TEST(codec_std, parseRon_tuple) {
  using type = tuple<int, string, double>;

  type v;
  auto is = io::is("(12, \"hi\", 2.5)");
  parseRon(is, v);
  EXPECT_EQ(v, make_tuple(12, "hi", 2.5));
}

TEST(codec_std, printRon_tuple) {
  using Tuple = tuple<int, double, bool>;
  using Vector = vector<optional<Tuple>>;
  
  EXPECT_EQ(S << Vector(), "[]");
  EXPECT_EQ(
      (S << Vector{ make_tuple(2, 3.0, false), nullopt, make_tuple(4, 2.5, true) }),
      "[(2, 3, false), null, (4, 2.5, true)]");
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
        throwsParseFailure<char>(1, { 1, 4 }, HasSubstr("Invalid index: 123")));
  }
}

TEST(codec_std, printRon_variant) {
  using type = variant<int, char, double>;

  EXPECT_EQ(S << type(1), "0:1");
  EXPECT_EQ(S << type('a'), "1:'a'");
  EXPECT_EQ(S << type(2.5), "2:2.5");
}

TEST(codec_std, printRon_vector) {
  EXPECT_EQ((S << vector{ 2, 3, 4 }), "[2, 3, 4]");
  EXPECT_EQ(S << optional<vector<int>>{}, "null");
  EXPECT_EQ((S << optional<vector<int>>(vector{ 1, 2 })), "[1, 2]");
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

TEST(codec_std, printRonIndent_pair) {
  using type = pair<pair<int, int>, pair<int, int>>;

  auto v = type(make_pair(1, 2), make_pair(3, 4));
  EXPECT_EQ(S << v, "((1, 2), (3, 4))");

  ROCKET_CODEC_RON_PRINT_PARAMS({ .indent=true });
  EXPECT_EQ(S << v, "(\n  (1, 2),\n  (3, 4)\n)");
}

TEST(codec_std, printRonIndent_vector) {
  using type = vector<vector<int>>;

  auto v = type { { 1, 2, 3 }, { 4, 5, 6 } };
  EXPECT_EQ(S << v, "[[1, 2, 3], [4, 5, 6]]");

  ROCKET_CODEC_RON_PRINT_PARAMS({ .indent=true });
  EXPECT_EQ(S << v, "[\n  [1, 2, 3],\n  [4, 5, 6]\n]");
}

// EOF
