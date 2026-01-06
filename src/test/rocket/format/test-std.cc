/*
 * test-std.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/format/std.h"
#include "rocket/nio/nio.h"

#include "rocket-gtest/matcher/matcher.h"

#include <span>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(std, byteFormat) {
  EXPECT_EQ(fmt::format("{}", byte { 0 }), "0");
  EXPECT_EQ(fmt::format("{:#x}", byte { 255 }), "0xff");
}

TEST(std, exceptionFormat) {
  try  {
    throw InvalidState("oops1");
  } catch (const exception& ex1) {
    EXPECT_THAT(fmt::format("{}", ex1), matchesRegex(".*\\.cc:\\d+: oops1"));
    EXPECT_THAT(fmt::format("{:t}", ex1), matchesRegex("`rocket::InvalidState`: .*\\.cc:\\d+: oops1"));

    u32string s32 = fmt::format(U"{}", ex1);
    EXPECT_NE(s32.find(U"oops1"), u32string::npos);

    auto msg = regex_replace(fmt::format("{:?}", ex1), regex("\\n"), "|");
    EXPECT_THAT(msg, matchesRegex(".*\\.cc:\\d+: oops1\\|   0# .*.*# "));

    msg = regex_replace(fmt::format("{:?t}", ex1), regex("\\n"), "|");
    EXPECT_THAT(msg, matchesRegex("`rocket::InvalidState`: .*\\.cc:\\d+: oops1\\|   0# .*.*# "));

    try {
      throw_with_nested(InvalidArgument("name", "oops2"));
    } catch (const exception& ex2) {
      EXPECT_THAT(fmt::format("{}", ex2), matchesRegex(".*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
      EXPECT_THAT(fmt::format("{:t}", ex2), matchesRegex("`std::_Nested_exception<rocket::InvalidArgument>`: .*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
    }
  }
}

TEST(std, initializerListFormat) {
  EXPECT_EQ(fmt::format("{}", initializer_list<int> { 1, 2, 3 }), "[1, 2, 3]");
}

TEST(std, mapFormat) {
  EXPECT_EQ(fmt::format("{}", map<int, string> { { 1, "one" }, { 2, "two" }, { 3, "three" } }), "{1: \"one\", 2: \"two\", 3: \"three\"}");
}

TEST(std, monostateFormat) {
  EXPECT_EQ(fmt::format("{}", monostate {}), "<monostate>");
  EXPECT_EQ(fmt::format(U"{}", monostate {}), U"<monostate>");
}

TEST(std, optionalFormat) {
  EXPECT_EQ(fmt::format("{:0>5d}", optional<int>()), "<none>");
  EXPECT_EQ(fmt::format("{:0>5d}", optional<int>(3)), "00003");

  EXPECT_EQ(fmt::format(U"{:0>5d}", optional<int>()), U"<none>");
  EXPECT_EQ(fmt::format(U"{:0>5d}", optional<int>(3)), U"00003");

  optional<string> opt = "hello";
  EXPECT_EQ(fmt::format("{}", opt), "hello");
  EXPECT_EQ(fmt::format("{: >{}}", opt, 10), "     hello");
  EXPECT_EQ(fmt::format("{: >{}?}", opt, 10), "   \"hello\"");

  vector<optional<string>> opts = { opt, opt, opt };
  EXPECT_EQ(fmt::format("{}", opts), "[\"hello\", \"hello\", \"hello\"]");
  EXPECT_EQ(fmt::format("{::}", opts), "[hello, hello, hello]");
}

TEST(std, optionalAndVectorInTypeLoopFormat) {
  using type = optional<vector<optional<int>>>;
  type v1 = nullopt;
  EXPECT_EQ(fmt::format("{:}", v1), "<none>");
  type v2 = vector<optional<int>> { optional<int>(1), nullopt, optional<int>(3) };
  EXPECT_EQ(fmt::format("{}", v2), "[1, <none>, 3]");
}

TEST(std, pairFormat) {
  EXPECT_EQ(fmt::format("{}", pair<int, string> { 1, "one" }), "(1, \"one\")");
}

TEST(std, pathFormat) {
  using namespace filesystem;

  auto p = path("file");

  EXPECT_EQ(fmt::format("{}", p), "file");
  EXPECT_EQ(fmt::format("{:g}", p), "file");
  EXPECT_EQ(fmt::format("{:?}", p), "\"file\"");
  EXPECT_EQ(fmt::format("{:?g}", p), "\"file\"");

  EXPECT_EQ(fmt::format("{: >10}", p), "      file");
  EXPECT_EQ(fmt::format("{: >10g}", p), "      file");
  EXPECT_EQ(fmt::format("{: >10?}", p), "    \"file\"");
  EXPECT_EQ(fmt::format("{: >10?g}", p), "    \"file\"");

  vector<path> v = { path("file1"), path("file2") };
  EXPECT_EQ(fmt::format("{}", v), "[\"file1\", \"file2\"]");
  EXPECT_EQ(fmt::format("{::}", v), "[file1, file2]");
  EXPECT_EQ(fmt::format("{::~>10}", v), "[~~~~~file1, ~~~~~file2]");
  EXPECT_EQ(fmt::format("{::~>10?}", v), "[~~~\"file1\", ~~~\"file2\"]");
}

TEST(std, setFormat) {
  EXPECT_EQ(fmt::format("{}", set<string> { "one", "two", "three" }), "{\"one\", \"three\", \"two\"}");
  EXPECT_EQ(fmt::format("{}", set<int> { 1, 2, 3 }), "{1, 2, 3}");
  EXPECT_EQ(fmt::format("{::}", set<string> { "one", "two", "three" }), "{one, three, two}");
}

TEST(std, spanFormat) {
  auto v = vector<int> { 1, 2, 3 };
  EXPECT_EQ(fmt::format("{}", span<int>(v.begin(), v.end())), "[1, 2, 3]");
}

TEST(std, tupleFormat) {
  EXPECT_EQ(fmt::format("{}", tuple<int, string, long> { 1, "one", -1L }), "(1, \"one\", -1)");
}

TEST(std, unorderedMapFormat) {
  EXPECT_EQ(fmt::format("{}", unordered_map<int, string> { { 3, "three" }, { 2, "two" }, { 1, "one" } }), "{1: \"one\", 2: \"two\", 3: \"three\"}");
}

TEST(std, unorderedSetFormat) {
  EXPECT_THAT(fmt::format("{}", unordered_set<int> { 1, 2, 3 }), matchesRegex("\\{\\d, \\d, \\d\\}"));
}

TEST(std, stringFormat) {
  static_assert(is_same_v<decltype("hello"s), string> == true);
  EXPECT_EQ(fmt::format("{}", "hello"s), "hello");
  EXPECT_EQ(fmt::format("{:?}", "hello"s), "\"hello\"");
  EXPECT_EQ(fmt::format("{}", "a\bc"s), "a\bc");
  EXPECT_EQ(fmt::format("{:?}", "a\bc"s), "\"a\\x08c\"");
  EXPECT_EQ(fmt::format("{}", "⊕"s), "⊕");
  EXPECT_EQ(fmt::format("{:?}", "⊕"s), "\"⊕\"");
  // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
  EXPECT_EQ(fmt::format("{}", "🧑‍🌾"s), "🧑‍🌾");
  EXPECT_EQ(fmt::format("{:?}", "🧑‍🌾"s), "\"🧑\\u200d🌾\"");
}

TEST(std, stringViewFormat) {
  static_assert(is_same_v<decltype("hello"sv), string_view> == true);
  EXPECT_EQ(fmt::format("{}", "hello"sv), "hello");
  EXPECT_EQ(fmt::format("{:?}", "hello"sv), "\"hello\"");
  EXPECT_EQ(fmt::format("{}", "a\bc"sv), "a\bc");
  EXPECT_EQ(fmt::format("{:?}", "a\bc"sv), "\"a\\x08c\"");
  EXPECT_EQ(fmt::format("{}", "⊕"sv), "⊕");
  EXPECT_EQ(fmt::format("{:?}", "⊕"sv), "\"⊕\"");
  // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
  EXPECT_EQ(fmt::format("{}", "🧑‍🌾"sv), "🧑‍🌾");
  EXPECT_EQ(fmt::format("{:?}", "🧑‍🌾"sv), "\"🧑\\u200d🌾\"");
}

TEST(std, variantFormat) {
  EXPECT_EQ(fmt::format("{}", variant<int, string, long> { 1 }), "0:1");
  EXPECT_EQ(fmt::format("{}", variant<int, string, long> { "one" }), "1:one");
  EXPECT_EQ(fmt::format("{:?}", variant<int, string, long> { "one" }), "1:\"one\"");
  EXPECT_EQ(fmt::format("{}", variant<int, string, long> { -1L }), "2:-1");
  EXPECT_EQ(fmt::format(U"{}", variant<int, u32string, long> { -1L }), U"2:-1");
  EXPECT_EQ(fmt::format(U"{}", variant<int, u32string, long> { U"hello" }), U"1:hello");
  EXPECT_EQ(fmt::format(U"{:?}", variant<int, u32string, long> { U"hello" }), U"1:\"hello\"");
}

TEST(std, vectorFormat) {
  EXPECT_EQ(fmt::format("{::0>5}", vector<int> { 1, 2, 3 }), "[00001, 00002, 00003]");
  EXPECT_EQ(fmt::format("{}", vector<string> { "one", "two", "three" }), "[\"one\", \"two\", \"three\"]");
  EXPECT_EQ(fmt::format("{::}", vector<string> { "one", "two", "three" }), "[one, two, three]");
}

TEST(std, vectorAndOptionalInTypeLoopFormat) {
  using type = vector<optional<vector<int>>>;
  type v1 = {};
  EXPECT_EQ(fmt::format("{}", v1), "[]");
  type v2 = type { vector<int> { vector<int> { 1, 2 } } };
  EXPECT_EQ(fmt::format("{}", v2), "[[1, 2]]");
}

// EOF
