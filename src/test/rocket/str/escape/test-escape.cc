/*
 * test-escape.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Bimap-codec.h"
#include "rocket/str/escape/escape.h"

using namespace rocket::str::escape;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(escape, CString) {
  Positions pos;

  // Double quotes, escaping, UTF-8 'ä'
  {
    const string in = "\\ä\"b";
    const CStringConfig config { .quote='"' };
    const string escaped = escapeCString(in, config, &pos);
    EXPECT_EQ(escaped, "\"\\\\ä\\\"b\"");
    EXPECT_EQ(pos, makePositions({ { 0, 1 }, { 1, 3 }, { 3, 5 }, { 4, 7 }, { 5, 8 } }));

    const string out = unescapeCString(escaped, config, &pos);
    EXPECT_EQ(out, in);
    EXPECT_EQ(pos, makePositions({ { 1, 0 }, { 3, 1 }, { 5, 3 }, { 7, 4 }, { 8, 5 } }));
  }

  // Apostrophes
  {
    const string in = "a'b";
    const CStringConfig config { .quote='\'' };
    const string escaped = escapeCString(in, config, &pos);
    EXPECT_EQ(escaped, "'a\\'b'");
    EXPECT_EQ(pos, makePositions({ { 0, 1 }, { 1, 2 }, { 2, 4 }, { 3, 5 } }));
    const string out = unescapeCString(escaped, config, &pos);
    EXPECT_EQ(out, in);
    EXPECT_EQ(pos, makePositions({ { 1, 0 }, { 2, 1 }, { 4, 2 }, { 5, 3 } }));
  }

  // Null char
  {
    const string in = "a\x00" "b"s;
    EXPECT_EQ(in.size(), 3);
    const CStringConfig config;
    const string escaped = escapeCString(in, config, &pos);
    EXPECT_EQ(escaped, "a\\x00b");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 2, 5 }, { 3, 6 } }));
    const string out = unescapeCString(escaped, config, &pos);
    EXPECT_EQ(out, in);
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 5, 2 }, { 6, 3 } }));
  }

  // Multi-code-point characters
  {
    // ☢️:  6 bytes, 2 code points
    // 🧑‍🌾: 11 bytes, 3 code points
    const string in = "a☢️b🧑‍🌾c";
    EXPECT_EQ(in.size(), 20);
    const CStringConfig config;
    const auto escaped = escapeCString(in, config, &pos);
    EXPECT_EQ(escaped, in);
    EXPECT_EQ(
        pos,
        makePositions({ { 0, 0 }, { 1, 1 }, { 7, 7 }, { 8, 8 }, { 19, 19 }, { 20, 20 } }));
    const string out = unescapeCString(escaped, config, &pos);
    EXPECT_EQ(out, in);
    EXPECT_EQ(
        pos,
        makePositions({ { 0, 0 }, { 1, 1 }, { 7, 7 }, { 8, 8 }, { 19, 19 }, { 20, 20 } }));
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
    auto escaped = escapeCString(in, config, &pos);
    EXPECT_EQ(escaped, "a\\tb");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 2, 3 }, { 3, 4 } }));

    config = { .tabSize=8 };
    escaped = escapeCString(in, config, &pos);
    EXPECT_EQ(escaped, "a       b");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 2, 8 }, { 3, 9 } }));

    in = "a\r\n\tb";
    escaped = escapeCString(in, config, &pos);
    EXPECT_EQ(escaped, "a\\r\\n   b");
    // CR/LF is one character
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 1 }, { 3, 5 }, { 4, 8 }, { 5, 9 } }));

    in = "\b🧑‍🌾\tb";
    escaped = escapeCString(in, config, &pos);
    EXPECT_EQ(escaped, "\\b🧑‍🌾    b");
    EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 1, 2 }, { 12, 13 }, { 13, 17 }, { 14, 18 } }));
  }

  EXPECT_THAT(
      [&] { unescapeCString("\"äbc", { .quote='"' }); },
      throwsInputFailure(5, { 0, 5 }, EndsWith("Missing terminating '\"' character")));

  EXPECT_THAT(
      [&] { unescapeCString("abc\\"); },
      throwsInputFailure(4, EndsWith("Expected a character, got EOI")));

  EXPECT_THAT(
      [&] { unescapeCString("abc\\Ä"); },
      throwsInputFailure(3, { 3, 6 }, EndsWith("Invalid escape sequence")));

  EXPECT_THAT(
      [&] { unescapeCString("\\x"); },
      throwsInputFailure(2, { 2,  2 }, EndsWith("Expected 2 hexadecimal digits, got EOI")));

  EXPECT_THAT(
      [&] { unescapeCString("\\U123456"); },
      throwsInputFailure(8, { 2, 8 }, EndsWith("Expected 8 hexadecimal digits, got EOI")));

  EXPECT_THAT(
      [&] { unescapeCString("\\x0X"); },
      throwsInputFailure(3, { 2, 4 }, EndsWith("Expected a hexadecimal digit, got \"X\"")));

  // 🧑‍🌾: 11 bytes, 3 code points
  EXPECT_THAT(
      [&] { unescapeCString("abc\\🧑‍🌾"); },
      throwsInputFailure(3, { 3, 15 }, EndsWith("Invalid escape sequence")));
}

// In the following tests, U+0301 (COMBINING ACUTE ACCENT) joins the preceding character, and U+0600 (ARABIC
// NUMBER SIGN) joins the following character, to form a single grapheme cluster

TEST(escape, CStringClusterQuoteWithCombiningMark) {
  Positions pos;

  const string in = "\"\u0301x";
  const CStringConfig config { .quote='"' };
  const string escaped = escapeCString(in, config, &pos);
  EXPECT_EQ(escaped, "\"\\\"\u0301x\"");
  EXPECT_EQ(pos, makePositions({ { 0, 1 }, { 3, 5 }, { 4, 6 } }));

  const string out = unescapeCString(escaped, config, &pos);
  EXPECT_EQ(out, in);
  EXPECT_EQ(pos, makePositions({ { 1, 0 }, { 5, 3 }, { 6, 4 } }));
}

TEST(escape, CStringClusterBackslashWithCombiningMark) {
  Positions pos;

  const string in = "\\\u0301";
  const string escaped = escapeCString(in, {}, &pos);
  EXPECT_EQ(escaped, "\\\\\u0301");
  EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 3, 4 } }));

  const string out = unescapeCString(escaped, {}, &pos);
  EXPECT_EQ(out, in);
  EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 4, 3 } }));
}

TEST(escape, CStringClusterOtherQuoteStaysVerbatim) {
  // Without a configured quote, a quote needs no escaping
  const string in1 = "\"\u0301";
  EXPECT_EQ(escapeCString(in1), in1);
  EXPECT_EQ(unescapeCString(in1), in1);

  // The same goes for a quote other than the configured one
  const CStringConfig config { .quote='"' };
  const string in2 = "'\u0301";
  const string escaped = escapeCString(in2, config);
  EXPECT_EQ(escaped, "\"'\u0301\"");
  EXPECT_EQ(unescapeCString(escaped, config), in2);
}

TEST(escape, CStringClusterPrependedQuote) {
  Positions pos;

  const string in = "\u0600\"x";
  const CStringConfig config { .quote='"' };
  const string escaped = escapeCString(in, config, &pos);
  EXPECT_EQ(escaped, "\"\u0600\\\"x\"");
  EXPECT_EQ(pos, makePositions({ { 0, 1 }, { 3, 5 }, { 4, 6 } }));

  const string out = unescapeCString(escaped, config, &pos);
  EXPECT_EQ(out, in);
  EXPECT_EQ(pos, makePositions({ { 1, 0 }, { 5, 3 }, { 6, 4 } }));
}

TEST(escape, CStringClusterPrependedBackslash) {
  Positions pos;

  const string in = "\u0600\\";
  const string escaped = escapeCString(in, {}, &pos);
  EXPECT_EQ(escaped, "\u0600\\\\");
  EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 3, 4 } }));

  const string out = unescapeCString(escaped, {}, &pos);
  EXPECT_EQ(out, in);
  EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 4, 3 } }));
}

TEST(escape, CStringClusterAtQuotes) {
  const CStringConfig config { .quote='"' };

  // A combining mark at the beginning forms a cluster with the opening quote
  const string in = "\u0301x";
  const string escaped = escapeCString(in, config);
  EXPECT_EQ(escaped, "\"\u0301x\"");
  EXPECT_EQ(unescapeCString(escaped, config), in);

  // A prepending character at the end forms a cluster with the closing quote
  EXPECT_EQ(unescapeCString("\"a\u0600\"", config), "a\u0600");
}

TEST(escape, CStringClusterColumn) {
  // The cluster has a width of 1, and the added backslash makes it 2, so the tab expands to 6 spaces
  const string escaped = escapeCString("\"\u0301\t", { .quote='"', .tabSize=8 });
  EXPECT_EQ(escaped, "\"\\\"\u0301      \"");
}

TEST(escape, CStringClusterInvalidEscape) {
  EXPECT_THAT(
      [&] { unescapeCString("\\a\u0301"); },
      throwsInputFailure(0, { 0, 4 }, EndsWith("Invalid escape sequence")));

  EXPECT_THAT(
      [&] { unescapeCString("\u0600\\q"); },
      throwsInputFailure(0, { 0, 4 }, EndsWith("Invalid escape sequence")));
}

TEST(escape, CStringInvalidUtf8) {
  EXPECT_THAT(
      [&] { unescapeCString("a\xFF"); },
      throwsInputFailure(1, EndsWith("Invalid UTF-8 byte sequence")));

  // Truncated sequence
  EXPECT_THAT(
      [&] { unescapeCString("a\xE2\x82"); },
      throwsInputFailure(1, EndsWith("Invalid UTF-8 byte sequence")));

  // Inside quotes
  EXPECT_THAT(
      [&] { unescapeCString("\"\xFF\"", { .quote='"' }); },
      throwsInputFailure(1, EndsWith("Invalid UTF-8 byte sequence")));
}

TEST(escape, CStringJsonSurrogatePair) {
  const CStringConfig config { .json=true };

  // A nonprintable code point greater than U+FFFF is written as a surrogate pair
  const string in = "a\U0010FFFFb";
  const string escaped = escapeCString(in, config);
  EXPECT_EQ(escaped, "a\\uDBFF\\uDFFFb");
  EXPECT_EQ(unescapeCString(escaped, config), in);

  EXPECT_EQ(escapeCString("\U000F0000", config), "\\uDB80\\uDC00"); // A private-use code point

  // A printable one stays verbatim
  EXPECT_EQ(escapeCString("\U0001F971", config), "\U0001F971"); // U+1F971 (Yawning Face)

  // Without JSON, `\U` is used
  EXPECT_EQ(escapeCString(in), "a\\U0010FFFFb");
}

TEST(escape, CStringTabSize) {
  EXPECT_THAT(
      [&] { escapeCString("a\tb", { .tabSize=0 }); },
      ThrowsMessage<InvalidArgument>(EndsWith("Tab size may not be 0")));

  // The zero check does not apply if there is no tab size, and the smallest tab size is valid
  EXPECT_EQ(escapeCString("a\tb", { .tabSize=nullopt }), "a\\tb");
  EXPECT_EQ(escapeCString("a\tb", { .tabSize=1 }), "a b");
}

TEST(escape, Regex) {
  Positions pos;

  {
    const string in = "\r\t\uFFFF()[a-z]";
    const auto escaped = escapeRegex(in, &pos);
    EXPECT_EQ(escaped, "\\r\\t\\uFFFF\\(\\)\\[a-z\\]");
    EXPECT_EQ(
        pos,
        makePositions({ { 0, 0 }, { 1, 2 }, { 2, 4 }, { 5, 10 }, { 6, 12 }, { 7, 14 }, { 8, 16 }, { 9, 17 }, { 10, 18 }, { 11, 19 }, { 12, 21 } }));
    const auto out = unescapeRegex(escaped, &pos);
    EXPECT_EQ(out, in);
    EXPECT_EQ(
        pos,
        makePositions({ { 0, 0 }, { 2, 1 }, { 4, 2 }, { 10, 5 }, { 12, 6 }, { 14, 7 }, { 16, 8 }, { 17, 9 }, { 18, 10 }, { 19, 11 }, { 21, 12 } }));
  }
}

TEST(escape, RegexClusterSpecialCharWithCombiningMark) {
  Positions pos;

  const string in = "(\u0301x";
  const string escaped = escapeRegex(in, &pos);
  EXPECT_EQ(escaped, "\\(\u0301x");
  EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 3, 4 }, { 4, 5 } }));

  const string out = unescapeRegex(escaped, &pos);
  EXPECT_EQ(out, in);
  EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 4, 3 }, { 5, 4 } }));
}

TEST(escape, RegexClusterAllSpecialChars) {
  for (const char c : "$()*+.?[\\]^{|}"sv) {
    SCOPED_TRACE(string(1, c));

    const string in = string(1, c) + "\u0301";
    const string escaped = escapeRegex(in);
    EXPECT_EQ(escaped, "\\" + in);
    EXPECT_EQ(unescapeRegex(escaped), in);
  }
}

TEST(escape, RegexClusterPrependedSpecialChar) {
  Positions pos;

  const string in = "\u0600(";
  const string escaped = escapeRegex(in, &pos);
  EXPECT_EQ(escaped, "\u0600\\(");
  EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 3, 4 } }));

  const string out = unescapeRegex(escaped, &pos);
  EXPECT_EQ(out, in);
  EXPECT_EQ(pos, makePositions({ { 0, 0 }, { 4, 3 } }));
}

TEST(escape, RegexClusterPrependedBackslash) {
  const string in = "\u0600\\";
  const string escaped = escapeRegex(in);
  EXPECT_EQ(escaped, "\u0600\\\\");
  EXPECT_EQ(unescapeRegex(escaped), in);
}

TEST(escape, RegexClusterStaysVerbatim) {
  // Neither character is special
  for (const string& in : { "a\u0301"s, "-\u0301"s }) {
    SCOPED_TRACE(in);

    EXPECT_EQ(escapeRegex(in), in);
    EXPECT_EQ(unescapeRegex(in), in);
  }

  // U+1F9D1 U+200D U+1F33E (Farmer): the ZERO WIDTH JOINER must not become a hexadecimal sequence
  const string farmer = "\U0001F9D1\u200D\U0001F33E";
  EXPECT_EQ(escapeRegex(farmer), farmer);
  EXPECT_EQ(unescapeRegex(farmer), farmer);
}

TEST(escape, RegexClusterInvalidEscape) {
  EXPECT_THAT(
      [&] { unescapeRegex("\\a\u0301"); },
      throwsInputFailure(0, { 0, 4 }, EndsWith("Invalid escape sequence")));

  EXPECT_THAT(
      [&] { unescapeRegex("\u0600\\q"); },
      throwsInputFailure(0, { 0, 4 }, EndsWith("Invalid escape sequence")));
}

TEST(escape, RegexInvalidUtf8) {
  EXPECT_THAT(
      [&] { unescapeRegex("a\xFF"); },
      throwsInputFailure(1, EndsWith("Invalid UTF-8 byte sequence")));

  // Truncated sequence
  EXPECT_THAT(
      [&] { unescapeRegex("a\xE2\x82"); },
      throwsInputFailure(1, EndsWith("Invalid UTF-8 byte sequence")));
}

// EOF
