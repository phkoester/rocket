/*
 * test-format-global.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/log.h"
#include "rocket/format-std.h"

#include <fmt/ranges.h>
// #include <fmt/std.h>

#include <type_traits>

using namespace std;

using rocket::log::LogLevel;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(format_std, formatOptional) {
  EXPECT_EQ(fmt::format("{:0>5d}", optional<int>()), "none");
  EXPECT_EQ(fmt::format("{:0>5d}", optional<int>(3)), "00003");
  EXPECT_EQ(fmt::format("{}", optional<LogLevel>(LogLevel::info)), "info");
  // XXX Debug für Enums muss weg
  EXPECT_EQ(fmt::format("{:?}", optional<LogLevel>(LogLevel::info)), "\"info\"");
}

TEST(format_std, formatOptionalAndVectorInTypeLoop) {
  using type = optional<vector<optional<LogLevel>>>;
  type v1 = nullopt;
  EXPECT_EQ(fmt::format("{:}", v1), "none");
  type v2 = vector<optional<LogLevel>> { optional<LogLevel>(LogLevel::info), nullopt, optional<LogLevel>(LogLevel::error) };
  EXPECT_EQ(fmt::format("{::}", v2), "[info, none, error]");
}

TEST(format_std, formatString) {
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

TEST(format_std, formatStringView) {
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

TEST(format_std, formatVector) {
  EXPECT_EQ(fmt::format("{::0>5}", vector<int> { 1, 2, 3 }), "[00001, 00002, 00003]");
}

// EOF
