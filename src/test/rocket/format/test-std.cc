/*
 * test-std.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/assert.h"
#include "rocket/format/std.h"
#include "rocket/nio/nio.h"

#include <span>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

// #TEST ----------------------------------------------------------------------------------------------------

TEST(std, byteFormat) {
  EXPECT_EQ(fmt::format("{}", byte { 0 }), "0");
  EXPECT_EQ(fmt::format("{:#x}", byte { 255 }), "0xff");
}

TEST(std, exceptionFormat) { // NOLINT(*-complexity)
  try  {
    ROCKET_FAIL("oops1");
  } catch (const exception& ex1) {
    EXPECT_THAT(fmt::format("{}", ex1), matchesRegex(".*\\.cc:\\d+: oops1"));
    EXPECT_THAT(fmt::format("{:t}", ex1), matchesRegex("`rocket::InvalidState`: .*\\.cc:\\d+: oops1"));

    const u32string s32 = fmt::format(U"{}", ex1);
    EXPECT_NE(s32.find(U"oops1"), u32string::npos);

    auto msg = fmt::format("{:?}", ex1);
    std::replace(msg.begin(), msg.end(), '\n', '|');
    EXPECT_THAT(msg, matchesRegex(".*\\.cc:\\d+: oops1\\|.*|.*|.*"));

    msg = fmt::format("{:?t}", ex1);
    std::replace(msg.begin(), msg.end(), '\n', '|');
    EXPECT_THAT(msg, matchesRegex("`rocket::InvalidState`: .*\\.cc:\\d+: oops1\\|.*|.*|.*"));

    try {
      throw_with_nested(InvalidArgument("name", "oops2"));
    } catch (const exception& ex2) {
      EXPECT_THAT(
        fmt::format("{}", ex2),
        matchesRegex(".*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
      EXPECT_THAT(
        fmt::format("{:t}", ex2),
        matchesRegex("`std::_.*ested.*<rocket::InvalidArgument>`: .*\\.cc:\\d+: Parameter `name`: oops2 \\(Because: .*\\.cc:\\d+: oops1\\)"));
    }
  }
}

TEST(std, initializerListFormat) {
  EXPECT_EQ(fmt::format("{}", initializer_list<i32> { 1, 2, 3 }), "[1, 2, 3]");
}

TEST(std, mapFormat) {
  EXPECT_EQ(
    fmt::format("{}", map<i32, string> { { 1, "one" }, { 2, "two" }, { 3, "three" } }),
    "{1: \"one\", 2: \"two\", 3: \"three\"}");
}

TEST(std, monostateFormat) {
  EXPECT_EQ(fmt::format("{}", monostate {}), "<monostate>");
  EXPECT_EQ(fmt::format(U"{}", monostate {}), U"<monostate>");
}

TEST(std, optionalFormat) {
  EXPECT_EQ(fmt::format("{:0>5d}", optional<i32>()), "<none>");
  EXPECT_EQ(fmt::format("{:0>5d}", optional<i32>(3)), "00003");

  EXPECT_EQ(fmt::format(U"{:0>5d}", optional<i32>()), U"<none>");
  EXPECT_EQ(fmt::format(U"{:0>5d}", optional<i32>(3)), U"00003");

  optional<string> opt = "hello";
  EXPECT_EQ(fmt::format("{}", opt), "hello");
  EXPECT_EQ(fmt::format("{: >{}}", opt, 10), "     hello");
  EXPECT_EQ(fmt::format("{: >{}?}", opt, 10), "   \"hello\"");

  vector<optional<string>> opts = { opt, opt, opt };
  EXPECT_EQ(fmt::format("{}", opts), "[\"hello\", \"hello\", \"hello\"]");
  EXPECT_EQ(fmt::format("{::}", opts), "[hello, hello, hello]");
}

TEST(std, optionalAndVectorInTypeLoopFormat) {
  using type = optional<vector<optional<i32>>>;
  type val1 = nullopt;
  EXPECT_EQ(fmt::format("{:}", val1), "<none>");
  type val2 = vector<optional<i32>> { optional<i32>(1), nullopt, optional<i32>(3) };
  EXPECT_EQ(fmt::format("{}", val2), "[1, <none>, 3]");
}

TEST(std, pairFormat) {
  EXPECT_EQ(fmt::format("{}", pair<i32, string> { 1, "one" }), "(1, \"one\")");
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

  vector<path> vec = { path("file1"), path("file2") };
  EXPECT_EQ(fmt::format("{}", vec), "[\"file1\", \"file2\"]");
  EXPECT_EQ(fmt::format("{::}", vec), "[file1, file2]");
  EXPECT_EQ(fmt::format("{::~>10}", vec), "[~~~~~file1, ~~~~~file2]");
  EXPECT_EQ(fmt::format("{::~>10?}", vec), "[~~~\"file1\", ~~~\"file2\"]");
}

TEST(std, setFormat) {
  EXPECT_EQ(fmt::format("{}", set<string> { "one", "two", "three" }), "{\"one\", \"three\", \"two\"}");
  EXPECT_EQ(fmt::format("{}", set<i32> { 1, 2, 3 }), "{1, 2, 3}");
  EXPECT_EQ(fmt::format("{::}", set<string> { "one", "two", "three" }), "{one, three, two}");
}

TEST(std, spanFormat) {
  auto vec = vector<i32> { 1, 2, 3 };
  EXPECT_EQ(fmt::format("{}", span<i32>(vec.begin(), vec.end())), "[1, 2, 3]");
}

TEST(std, tupleFormat) {
  EXPECT_EQ(fmt::format("{}", tuple<i32, string, i64> { 1, "one", -1L }), "(1, \"one\", -1)");
}

TEST(std, unorderedMapFormat) {
  EXPECT_THAT(
    fmt::format("{}", unordered_map<i32, string> { { 3, "three" }, { 2, "two" }, { 1, "one" } }),
    matchesRegex("\\{\\d: \".*\", \\d: \".*\", \\d: \".*\"\\}"));
}

TEST(std, unorderedSetFormat) {
  EXPECT_THAT(fmt::format("{}", unordered_set<i32> { 1, 2, 3 }), matchesRegex("\\{\\d, \\d, \\d\\}"));
}

TEST(std, stringFormat) {
  static_assert(is_same_v<decltype("hello"s), string>);
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
  static_assert(is_same_v<decltype("hello"sv), string_view>);
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
  EXPECT_EQ(fmt::format("{}", variant<i32, string, i64> { 1 }), "0:1");
  EXPECT_EQ(fmt::format("{}", variant<i32, string, i64> { "one" }), "1:one");
  EXPECT_EQ(fmt::format("{:?}", variant<i32, string, i64> { "one" }), "1:\"one\"");
  EXPECT_EQ(fmt::format("{}", variant<i32, string, i64> { -1_i64 }), "2:-1");
  EXPECT_EQ(fmt::format(U"{}", variant<i32, u32string, i64> { -1_i64 }), U"2:-1");
  EXPECT_EQ(fmt::format(U"{}", variant<i32, u32string, i64> { U"hello" }), U"1:hello");
  EXPECT_EQ(fmt::format(U"{:?}", variant<i32, u32string, i64> { U"hello" }), U"1:\"hello\"");
}

TEST(std, vectorFormat) {
  EXPECT_EQ(fmt::format("{::0>5}", vector<i32> { 1, 2, 3 }), "[00001, 00002, 00003]");
  EXPECT_EQ(fmt::format("{}", vector<string> { "one", "two", "three" }), "[\"one\", \"two\", \"three\"]");
  EXPECT_EQ(fmt::format("{::}", vector<string> { "one", "two", "three" }), "[one, two, three]");
}

TEST(std, vectorAndOptionalInTypeLoopFormat) {
  using type = vector<optional<vector<i32>>>;
  type val1 = {};
  EXPECT_EQ(fmt::format("{}", val1), "[]");
  type val2 = type { vector<i32> { vector<i32> { 1, 2 } } };
  EXPECT_EQ(fmt::format("{}", val2), "[[1, 2]]");
}

// EOF
