/*
 * test-escape.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/escape/escape.h"

#include "rocket-gtest/matcher/matcher.h"

using namespace rocket;
using namespace rocket::escape;
using namespace rocket::gtest;
using namespace rocket::gtest::matcher;
using namespace std;
using namespace testing;

// Functions ------------------------------------------------------------------------------------------------

auto
positions(initializer_list<pair<size_t, size_t>> list) {
  return rocket::container::makeUnorderedBimap(list);
}

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(escape, CString) {
  Result result;

  // Double quotes, escaping, UTF-8 'ä'
  {
    string in = "\\ä\"b";
    CStringParams params { .quote='"' };
    string escaped = escape::escapeCString(in, params, &result);
    EXPECT_EQ(escaped, "\"\\\\ä\\\"b\"");
    EXPECT_EQ(result.positions, positions({ { 0, 1 }, { 1, 3 }, { 3, 5 }, { 4, 7 }, { 5, 8 } }));

    string out = escape::unescapeCString(escaped, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.positions, positions({ { 1, 0 }, { 3, 1 }, { 5, 3 }, { 7, 4 }, { 8, 5 } }));
  }

  // Apostrophes
  {
    string in = "a'b";
    CStringParams params { .quote='\'' };
    string escaped = escape::escapeCString(in, params, &result);
    EXPECT_EQ(escaped, "'a\\'b'");
    EXPECT_EQ(result.positions, positions({ { 0, 1 }, { 1, 2 }, { 2, 4 }, { 3, 5 } }));
    string out = escape::unescapeCString(escaped, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.positions, positions({ { 1, 0 }, { 2, 1 }, { 4, 2 }, { 5, 3 } }));
  }

  // Null char
  {
    string in = "a\x00" "b"s;
    EXPECT_EQ(in.size(), 3);
    CStringParams params;
    auto escaped = escape::escapeCString(in, params, &result);
    EXPECT_EQ(escaped, "a\\x00b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 5 }, { 3, 6 } }));
    string out = escape::unescapeCString(escaped, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 5, 2 }, { 6, 3 } }));
  }

  // Multi-code-point graphemes
  {
    // ☢️:  6 bytes, 2 code points
    // 🧑‍🌾: 11 bytes, 3 code points
    string in = "a☢️b🧑‍🌾c";
    EXPECT_EQ(in.size(), 20);
    CStringParams params;
    auto escaped = escape::escapeCString(in, params, &result);
    EXPECT_EQ(escaped, in);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 1, 1 }, { 7, 7 }, { 8, 8 }, { 19, 19 }, { 20, 20 } }));
    string out = escape::unescapeCString(escaped, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 1, 1 }, { 7, 7 }, { 8, 8 }, { 19, 19 }, { 20, 20 } }));
  }

  // Hex
  EXPECT_EQ(escape::unescapeCString("\\x7f"), "\x7f");
  EXPECT_EQ(escape::unescapeCString("\\u20AC"), "€");
  EXPECT_EQ(escape::unescapeCString("\\U0001f971"), "🥱"); // U+1F971 (Yawning Face)

  // Tab
  {
    string in = "a\tb";
    CStringParams params;
    auto escaped = escape::escapeCString(in, params, &result);
    EXPECT_EQ(escaped, "a\\tb");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 3 }, { 3, 4 } }));

    params = { .tabSize=8 };
    escaped = escape::escapeCString(in, params, &result);
    EXPECT_EQ(escaped, "a       b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 8 }, { 3, 9 } }));

    in = "a\r\n\tb";
    escaped = escape::escapeCString(in, params, &result);
    EXPECT_EQ(escaped, "a\\r\\n   b");
    // CRLF is one grapheme
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 3, 5 }, { 4, 8 }, { 5, 9 } }));

    in = "\b🧑‍🌾\tb";
    escaped = escape::escapeCString(in, params, &result);
    EXPECT_EQ(escaped, "\\b🧑‍🌾    b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 2 }, { 12, 13 }, { 13, 17 }, { 14, 18 } }));
  }

  {
    string in = "\"äbc";
    CStringParams params { .quote='"' };
    EXPECT_THAT(
        [&] { escape::unescapeCString(in, params); },
        throwsInputFailure(5, { 0, 5 }, HasSubstr("Missing terminating '\"' character")));
  }

  {
    string in = "abc\\";
    CStringParams params;
    EXPECT_THAT(
        [&] { escape::unescapeCString(in, params); },
        throwsInputFailure(4, HasSubstr("Expected a UTF-8 grapheme")));
  }

  {
    string in = "abc\\Ä";
    CStringParams params;
    EXPECT_THAT(
        [&] { escape::unescapeCString(in, params); },
        throwsInputFailure(3, { 3, 6 }, HasSubstr("Invalid escape sequence")));
  }

  {
    string in = "\\x";
    CStringParams params;
    EXPECT_THAT(
        [&] { escape::unescapeCString(in, params); },
        throwsInputFailure(2, HasSubstr("Expected a hexadecimal digit, got EOF")));
  }

  {
    string in = "\\x0\xff";
    CStringParams params;
    EXPECT_THAT(
        [&] { escape::unescapeCString(in, params); },
        throwsInputFailure(4, HasSubstr("Expected a hexadecimal digit, got '\\xff'")));
  }

  {
    // 🧑‍🌾: 11 bytes, 3 code points
    string in = "abc\\🧑‍🌾";
    CStringParams params;
    EXPECT_THAT(
        [&] { escape::unescapeCString(in, params); },
        throwsInputFailure(3, { 3, 15 }, HasSubstr("Invalid escape sequence")));
  }
}

#if 0 // XXX Escape ist kaputt, erstmal Source schreiben, um EOF-Problematik zu beheben

TEST(escape, Regex) {
  Result result;

  {
    stringstream ss;
    string in = "\r\t\uFFFF()[a-z]";
    Regex::Params params;
    ss << escaped<Regex>(in, params, &result);
    string esc = "\\r\\t\\uFFFF\\(\\)\\[a-z\\]";
    EXPECT_EQ(ss.str(), esc);
    EXPECT_EQ(result.input, in);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 1, 2 }, { 2, 4 }, { 5, 10 }, { 6, 12 }, { 7, 14 }, { 8, 16 }, { 9, 17 }, { 10, 18 }, { 11, 19 }, { 12, 21 } }));
    string out;
    ss >> escaped<Regex>(out, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 2, 1 }, { 4, 2 }, { 10, 5 }, { 12, 6 }, { 14, 7 }, { 16, 8 }, { 17, 9 }, { 18, 10 }, { 19, 11 }, { 21, 12 } }));
  }
}

#endif

// EOF
