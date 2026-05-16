/*
 * test-escape.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Bimap-codec.h"
#include "rocket/str/escape/escape.h"

using namespace rocket::str::escape;

namespace {

// Local functions ------------------------------------------------------------------------------------------

auto
positions(initializer_list<pair<u64, u64>> list) {
  return makeUnorderedBimap(list);
}

} // namespace

// #TEST ----------------------------------------------------------------------------------------------------

TEST(escape, CString) {
  Result result;

  // Double quotes, escaping, UTF-8 'ä'
  {
    const string in = "\\ä\"b";
    const CStringConfig config { .quote='"' };
    const string escaped = escapeCString(in, config, &result);
    EXPECT_EQ(escaped, "\"\\\\ä\\\"b\"");
    EXPECT_EQ(result.positions, positions({ { 0, 1 }, { 1, 3 }, { 3, 5 }, { 4, 7 }, { 5, 8 } }));

    const string out = unescapeCString(escaped, config, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.positions, positions({ { 1, 0 }, { 3, 1 }, { 5, 3 }, { 7, 4 }, { 8, 5 } }));
  }

  // Apostrophes
  {
    const string in = "a'b";
    const CStringConfig config { .quote='\'' };
    const string escaped = escapeCString(in, config, &result);
    EXPECT_EQ(escaped, "'a\\'b'");
    EXPECT_EQ(result.positions, positions({ { 0, 1 }, { 1, 2 }, { 2, 4 }, { 3, 5 } }));
    const string out = unescapeCString(escaped, config, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.positions, positions({ { 1, 0 }, { 2, 1 }, { 4, 2 }, { 5, 3 } }));
  }

  // Null char
  {
    const string in = "a\x00" "b"s;
    EXPECT_EQ(in.size(), 3);
    const CStringConfig config;
    const string escaped = escapeCString(in, config, &result);
    EXPECT_EQ(escaped, "a\\x00b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 5 }, { 3, 6 } }));
    const string out = unescapeCString(escaped, config, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 5, 2 }, { 6, 3 } }));
  }

  // Multi-code-point characters
  {
    // ☢️:  6 bytes, 2 code points
    // 🧑‍🌾: 11 bytes, 3 code points
    const string in = "a☢️b🧑‍🌾c";
    EXPECT_EQ(in.size(), 20);
    const CStringConfig config;
    const auto escaped = escapeCString(in, config, &result);
    EXPECT_EQ(escaped, in);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 1, 1 }, { 7, 7 }, { 8, 8 }, { 19, 19 }, { 20, 20 } }));
    const string out = unescapeCString(escaped, config, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 1, 1 }, { 7, 7 }, { 8, 8 }, { 19, 19 }, { 20, 20 } }));
  }

  // Hex
  EXPECT_EQ(unescapeCString("\\x7f"), "\x7f");
  EXPECT_EQ(unescapeCString("\\u20ac"), "€");
  EXPECT_EQ(unescapeCString("\\u20AC"), "€");
  EXPECT_EQ(unescapeCString("\\U0001f971"), "🥱"); // U+1F971 (Yawning Face)

  // Tab
  {
    string in = "a\tb";
    CStringConfig config;
    auto escaped = escapeCString(in, config, &result);
    EXPECT_EQ(escaped, "a\\tb");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 3 }, { 3, 4 } }));

    config = { .tabSize=8 };
    escaped = escapeCString(in, config, &result);
    EXPECT_EQ(escaped, "a       b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 8 }, { 3, 9 } }));

    in = "a\r\n\tb";
    escaped = escapeCString(in, config, &result);
    EXPECT_EQ(escaped, "a\\r\\n   b");
    // CR/LF is one character
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 3, 5 }, { 4, 8 }, { 5, 9 } }));

    in = "\b🧑‍🌾\tb";
    escaped = escapeCString(in, config, &result);
    EXPECT_EQ(escaped, "\\b🧑‍🌾    b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 2 }, { 12, 13 }, { 13, 17 }, { 14, 18 } }));
  }

  EXPECT_THAT(
      [&] { unescapeCString("\"äbc", { .quote='"' }); },
      throwsInputFailure(5, { 0, 5 }, HasSubstr("Missing terminating '\"' character")));

  EXPECT_THAT(
      [&] { unescapeCString("abc\\"); },
      throwsInputFailure(4, HasSubstr("Expected character, got EOI")));

  EXPECT_THAT(
      [&] { unescapeCString("abc\\Ä"); },
      throwsInputFailure(3, { 3, 6 }, HasSubstr("Invalid escape sequence")));

  EXPECT_THAT(
      [&] { unescapeCString("\\x"); },
      throwsInputFailure(2, { 2,  2 }, HasSubstr("Expected 2 hexadecimal digits, got EOI")));

  EXPECT_THAT(
      [&] { unescapeCString("\\U123456"); },
      throwsInputFailure(8, { 2, 8 }, HasSubstr("Expected 8 hexadecimal digits, got EOI")));

  EXPECT_THAT(
      [&] { unescapeCString("\\x0X"); },
      throwsInputFailure(3, { 2, 4 }, HasSubstr("Expected a hexadecimal digit, got \"X\"")));

  // 🧑‍🌾: 11 bytes, 3 code points
  EXPECT_THAT(
      [&] { unescapeCString("abc\\🧑‍🌾"); },
      throwsInputFailure(3, { 3, 15 }, HasSubstr("Invalid escape sequence")));
}

TEST(escape, Regex) {
  Result result;

  {
    const string in = "\r\t\uFFFF()[a-z]";
    const auto escaped = escapeRegex(in, &result);
    EXPECT_EQ(escaped, "\\r\\t\\uFFFF\\(\\)\\[a-z\\]");
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 1, 2 }, { 2, 4 }, { 5, 10 }, { 6, 12 }, { 7, 14 }, { 8, 16 }, { 9, 17 }, { 10, 18 }, { 11, 19 }, { 12, 21 } }));
    const auto out = unescapeRegex(escaped, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 2, 1 }, { 4, 2 }, { 10, 5 }, { 12, 6 }, { 14, 7 }, { 16, 8 }, { 17, 9 }, { 18, 10 }, { 19, 11 }, { 21, 12 } }));
  }
}

// EOF
