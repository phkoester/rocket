/*
 * test-std.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/assert.h"
#include "rocket/nio/nio.h"

#include <fmt/ranges.h>
#include <fmt/std.h>

#include <span>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

// #TEST ----------------------------------------------------------------------------------------------------

TEST(std, byteFormat) {
  EXPECT_EQ(fmt::format("{}", byte { 0 }), "0");
  EXPECT_EQ(fmt::format("{:#x}", byte { 255 }), "0xff");
}

TEST(std, initializerListFormat) {
  EXPECT_EQ(fmt::format("{}", initializer_list<i32> { 1, 2, 3 }), "[1, 2, 3]");
}

TEST(std, mapFormat) {
  EXPECT_EQ(
    fmt::format("{}", map<i32, string> { { 1, "one" }, { 2, "two" }, { 3, "three" } }),
    "{1: \"one\", 2: \"two\", 3: \"three\"}");
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

TEST(std, vectorFormat) {
  EXPECT_EQ(fmt::format("{::0>5}", vector<i32> { 1, 2, 3 }), "[00001, 00002, 00003]");
  EXPECT_EQ(fmt::format("{}", vector<string> { "one", "two", "three" }), "[\"one\", \"two\", \"three\"]");
  EXPECT_EQ(fmt::format("{::}", vector<string> { "one", "two", "three" }), "[one, two, three]");
}

// EOF
