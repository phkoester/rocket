/*
 * test-format-std.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/format/std.h"
#include "rocket/log/log.h"

#include <span>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

using namespace std;

using rocket::log::LogLevel;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(format_std, byteFormat) {
  EXPECT_EQ(fmt::format("{}", byte { 0 }), "0");
  EXPECT_EQ(fmt::format("{:#x}", byte { 255 }), "0xff");
}

TEST(format_std, initializerListFormat) {
  EXPECT_EQ(fmt::format("{}", initializer_list<int> { 1, 2, 3 }), "[1, 2, 3]");
}

TEST(format_std, mapFormat) {
  EXPECT_EQ(fmt::format("{}", map<int, string> { { 1, "one" }, { 2, "two" }, { 3, "three" } }), "{1: \"one\", 2: \"two\", 3: \"three\"}");
}

TEST(format_std, optionalFormat) {
  EXPECT_EQ(fmt::format("{:0>5d}", optional<int>()), "<none>");
  EXPECT_EQ(fmt::format("{:0>5d}", optional<int>(3)), "00003");
  EXPECT_EQ(fmt::format("{}", optional<LogLevel>(LogLevel::info)), "info");
}

TEST(format_std, optionalAndVectorInTypeLoopFormat) {
  using type = optional<vector<optional<LogLevel>>>;
  type v1 = nullopt;
  EXPECT_EQ(fmt::format("{:}", v1), "<none>");
  type v2 = vector<optional<LogLevel>> { optional<LogLevel>(LogLevel::info), nullopt, optional<LogLevel>(LogLevel::error) };
  EXPECT_EQ(fmt::format("{}", v2), "[info, <none>, error]");
}

TEST(format_std, pairFormat) {
  EXPECT_EQ(fmt::format("{}", pair<int, string> { 1, "one" }), "(1, \"one\")");
}

TEST(format_std, setFormat) {
  EXPECT_EQ(fmt::format("{}", set<string> { "one", "two", "three" }), "{\"one\", \"three\", \"two\"}");
  EXPECT_EQ(fmt::format("{}", set<int> { 1, 2, 3 }), "{1, 2, 3}");
  EXPECT_EQ(fmt::format("{::}", set<string> { "one", "two", "three" }), "{one, three, two}");
}

TEST(format_std, spanFormat) {
  auto v = vector<int> { 1, 2, 3 };
  EXPECT_EQ(fmt::format("{}", span<int>(v.begin(), v.end())), "[1, 2, 3]");
}

TEST(format_std, tupleFormat) {
  EXPECT_EQ(fmt::format("{}", tuple<int, string, long> { 1, "one", -1L }), "(1, \"one\", -1)");
}

TEST(format_std, unorderedMapFormat) {
  EXPECT_EQ(fmt::format("{}", unordered_map<int, string> { { 3, "three" }, { 2, "two" }, { 1, "one" } }), "{1: \"one\", 2: \"two\", 3: \"three\"}");
}

TEST(format_std, unorderedSetFormat) {
  EXPECT_EQ(fmt::format("{}", unordered_set<int> { 1, 2, 3 }), "{3, 2, 1}");
}

TEST(format_std, stringFormat) {
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

TEST(format_std, stringViewFormat) {
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

TEST(format_std, variantFormat) {
  EXPECT_EQ(fmt::format("{}", variant<int, string, long> { 1 }), "0:1");
  EXPECT_EQ(fmt::format("{}", variant<int, string, long> { "one" }), "1:\"one\"");
  EXPECT_EQ(fmt::format("{}", variant<int, string, long> { -1L }), "2:-1");
}

TEST(format_std, vectorFormat) {
  EXPECT_EQ(fmt::format("{::0>5}", vector<int> { 1, 2, 3 }), "[00001, 00002, 00003]");
  EXPECT_EQ(fmt::format("{}", vector<string> { "one", "two", "three" }), "[\"one\", \"two\", \"three\"]");
  EXPECT_EQ(fmt::format("{::}", vector<string> { "one", "two", "three" }), "[one, two, three]");
}

TEST(format_std, vectorAndOptionalInTypeLoopFormat) {
  using type = vector<optional<vector<LogLevel>>>;
  type v1 = {};
  EXPECT_EQ(fmt::format("{}", v1), "[]");
  type v2 = type { vector<LogLevel> { vector<LogLevel> { LogLevel::info, LogLevel::warn } } };
  EXPECT_EQ(fmt::format("{}", v2), "[[info, warn]]");
}

// EOF
