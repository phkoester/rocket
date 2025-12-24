/*
 * test-escape.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-boost-decl.h"
#include "rocket/codec-rocket-decl.h"
#include "rocket/codec-std-decl.h"
#include "rocket/codec-boost.h"
#include "rocket/codec-rocket.h"
#include "rocket/codec-std.h"

#include "rocket/escape.h"

#include "rocket-gtest/matcher.h"
#include "rocket-gtest/PrintTo.h"

using namespace rocket;
using namespace rocket::escape;
using namespace rocket::gtest;
using namespace rocket::gtest::matcher;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(escape, CString_char) {
  using type = char;

  Result<type> result;

  // Test double quotes, escaping, UTF-8 'ä'
  {
    stringstream ss;
    string in = "\\ä\"b";
    CString::Params params { .enclosed=true, .quote='"' };
    ss << escaped<CString>(in, params, &result);
    string esc = "\"\\\\ä\\\"b\"";
    EXPECT_EQ(ss.str(), esc);
    EXPECT_EQ(result.input, in);
    EXPECT_EQ(result.positions, positions({ { 0, 1 }, { 1, 3 }, { 3, 5 }, { 4, 7 }, { 5, 8 } }));
    string out;
    ss >> escaped<CString>(out, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(result.positions, positions({ { 1, 0 }, { 3, 1 }, { 5, 3 }, { 7, 4 }, { 8, 5 } }));
  }

  // Test apostrophes
  {
    stringstream ss;
    string in = "a'b";
    CString::Params params { .enclosed=true, .quote='\'' };
    ss << escaped<CString>(in, params, &result);
    string esc = "'a\\'b'";
    EXPECT_EQ(ss.str(), esc);
    EXPECT_EQ(result.input, in);
    EXPECT_EQ(result.positions, positions({ { 0, 1 }, { 1, 2 }, { 2, 4 }, { 3, 5 } }));
    string out;
    ss >> escaped<CString>(out, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(result.positions, positions({ { 1, 0 }, { 2, 1 }, { 4, 2 }, { 5, 3 } }));
  }

  // Test null char
  {
    stringstream ss;
    string in = "a\x00" "b"s;
    EXPECT_EQ(in.size(), 3);
    CString::Params params;
    ss << escaped<CString>(in, params, &result);
    string esc = "a\\x00b";
    EXPECT_EQ(ss.str(), esc);
    EXPECT_EQ(result.input, in);
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 5 }, { 3, 6 } }));
    string out;
    ss >> escaped<CString>(out, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 5, 2 }, { 6, 3 } }));
  }

  // Test multi-code-point graphemes
  {
    stringstream ss;
    // ☢️:  6 bytes, 2 code points
    // 🧑‍🌾: 11 bytes, 3 code points
    string in = "a☢️b🧑‍🌾c";
    EXPECT_EQ(in.size(), 20);
    CString::Params params;
    ss << escaped<CString>(in, params, &result);
    string esc = in;
    EXPECT_EQ(ss.str(), esc);
    EXPECT_EQ(result.input, in);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 1, 1 }, { 7, 7 }, { 8, 8 }, { 19, 19 }, { 20, 20 } }));
    string out;
    ss >> escaped<CString>(out, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 1, 1 }, { 7, 7 }, { 8, 8 }, { 19, 19 }, { 20, 20 } }));
  }

  // Test tab
  {
    ostringstream os;
    string in = "a\tb";
    CString::Params params;
    os << escaped<CString>(in, params, &result);
    EXPECT_EQ(os.str(), "a\\tb");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 3 }, { 3, 4 } }));

    os.str("");
    params = { .tabSize=8 };
    os << escaped<CString>(in, params, &result);
    EXPECT_EQ(os.str(), "a       b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 8 }, { 3, 9 } }));

    os.str("");
    in = "a\r\n\tb";
    os << escaped<CString>(in, params, &result);
    EXPECT_EQ(os.str(), "a\\r\\n   b");
    // CRLF is one grapheme
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 3, 5 }, { 4, 8 }, { 5, 9 } }));

    os.str("");
    in = "\b🧑‍🌾\tb";
    os << escaped<CString>(in, params, &result);
    EXPECT_EQ(os.str(), "\\b🧑‍🌾    b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 2 }, { 12, 13 }, { 13, 17 }, { 14, 18 } }));
  }

  {
    auto is = io::is("\"äbc");
    string out;
    CString::Params params { .enclosed=true, .quote='"' };
    EXPECT_THAT(
        [&] { io::resetg(is) >> escaped<CString>(out, params); },
        throwsParseFailure<type>(5, { 0, 5 }, HasSubstr("Missing terminating '\"' character")));
  }

  {
    auto is = io::is("abc\\");
    string out;
    CString::Params params;
    EXPECT_THAT(
        [&] { io::resetg(is) >> escaped<CString>(out, params); },
        throwsParseFailure<type>(4, { 3, 4 }, HasSubstr("Expected a Unicode grapheme, got EOF")));
  }

  {
    auto is = io::is("abc\\Ä");
    string out;
    CString::Params params;
    EXPECT_THAT(
        [&] { io::resetg(is) >> escaped<CString>(out, params); },
        throwsParseFailure<type>(3, { 3, 6 }, HasSubstr("Invalid escape sequence")));
  }

  {
    // 🧑‍🌾: 11 bytes, 3 code points
    auto is = io::is("abc\\🧑‍🌾");
    string out;
    CString::Params params;
    EXPECT_THAT(
        [&] { io::resetg(is) >> escaped<CString>(out, params); },
        throwsParseFailure<type>(3, { 3, 15 }, HasSubstr("Invalid escape sequence")));
  }
}

TEST(escape, CString_char32_t) {
  using type = char32_t;

  Result<type> result;

  // Test double quotes, escaping, UTF-8 'ä'
  {
    u32stringstream ss;
    u32string in = U"\\ä\"b";
    CString::Params params { .enclosed=true, .quote='"' };
    ss << escaped<CString>(in, params, &result);
    u32string esc = U"\"\\\\ä\\\"b\"";
    EXPECT_EQ(ss.str(), esc);
    EXPECT_EQ(result.input, in);
    EXPECT_EQ(result.positions, positions({ { 0, 1 }, { 1, 3 }, { 2, 4 }, { 3, 6 }, { 4, 7 } }));
    u32string out;
    ss >> escaped<CString>(out, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(result.positions, positions({ { 1, 0 }, { 3, 1 }, { 4, 2 }, { 6, 3 }, { 7, 4 } }));
  }

  // Test apostrophes
  {
    u32stringstream ss;
    u32string in = U"a'b";
    CString::Params params { .enclosed=true, .quote='\'' };
    ss << escaped<CString>(in, params, &result);
    u32string esc = U"'a\\'b'";
    EXPECT_EQ(ss.str(), esc);
    EXPECT_EQ(result.input, in);
    EXPECT_EQ(result.positions, positions({ { 0, 1 }, { 1, 2 }, { 2, 4 }, { 3, 5 } }));
    u32string out;
    ss >> escaped<CString>(out, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(result.positions, positions({ { 1, 0 }, { 2, 1 }, { 4, 2 }, { 5, 3 } }));
  }

  // Test null char
  {
    u32stringstream ss;
    u32string in = U"a\x00" "b"s;
    EXPECT_EQ(in.size(), 3);
    CString::Params params;
    ss << escaped<CString>(in, params, &result);
    u32string esc = U"a\\x00b";
    EXPECT_EQ(ss.str(), esc);
    EXPECT_EQ(result.input, in);
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 5 }, { 3, 6 } }));
    u32string out;
    // This needs the `char32_t` facets in the process locale ...
    ss >> escaped<CString>(out, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 5, 2 }, { 6, 3 } }));
  }

  // Test multi-code-point graphemes
  {
    u32stringstream ss;
    // ☢️: 2 code points
    // 🧑‍🌾: 3 code points
    u32string in = U"a☢️b🧑‍🌾c";
    EXPECT_EQ(in.size(), 8);
    CString::Params params;
    ss << escaped<CString>(in, params, &result);
    u32string esc = in;
    EXPECT_EQ(ss.str(), esc);
    EXPECT_EQ(result.input, in);
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 3, 3 }, { 4, 4 }, { 7, 7 }, { 8, 8 } }));
    u32string out;
    ss >> escaped<CString>(out, params, &result);
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 3, 3 }, { 4, 4 }, { 7, 7 }, { 8, 8 } }));
  }

  // Test tab
  {
    u32ostringstream os;
    u32string in = U"a\tb";
    CString::Params params;
    os << escaped<CString>(in, params, &result);
    EXPECT_EQ(os.str(), U"a\\tb");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 3 }, { 3, 4 } }));

    os.str(U"");
    params = { .tabSize=8 };
    os << escaped<CString>(in, params, &result);
    EXPECT_EQ(os.str(), U"a       b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 2, 8 }, { 3, 9 } }));

    os.str(U"");
    in = U"a\r\n\tb";
    os << escaped<CString>(in, params, &result);
    EXPECT_EQ(os.str(), U"a\\r\\n   b");
    // CRLF is one grapheme
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 1 }, { 3, 5 }, { 4, 8 }, { 5, 9 } }));

    os.str(U"");
    in = U"\b🧑‍🌾\tb";
    os << escaped<CString>(in, params, &result);
    EXPECT_EQ(os.str(), U"\\b🧑‍🌾    b");
    EXPECT_EQ(result.positions, positions({ { 0, 0 }, { 1, 2 }, { 4, 5 }, { 5, 9 }, { 6, 10 } }));
  }

  {
    auto is = io::is(U"\"äbc");
    u32string out;
    CString::Params params { .enclosed=true, .quote='"' };
    EXPECT_THAT(
        [&] { io::resetg(is) >> escaped<CString>(out, params); },
        throwsParseFailure<type>(4, { 0, 4 }, HasSubstr("Missing terminating '\"' character")));
  }

  {
    auto is = io::is(U"abc\\");
    u32string out;
    CString::Params params;
    EXPECT_THAT(
        [&] { io::resetg(is) >> escaped<CString>(out, params); },
        throwsParseFailure<type>(4, { 3, 4 }, HasSubstr("Expected a Unicode grapheme, got EOF")));
  }

  {
    auto is = io::is(U"abc\\Ä");
    u32string out;
    CString::Params params;
    EXPECT_THAT(
        [&] { io::resetg(is) >> escaped<CString>(out, params); },
        throwsParseFailure<type>(3, { 3, 5 }, HasSubstr("Invalid escape sequence")));
  }

  {
    // 🧑‍🌾: 3 code points
    auto is = io::is(U"abc\\🧑‍🌾");
    u32string out;
    CString::Params params;
    EXPECT_THAT(
        [&] { io::resetg(is) >> escaped<CString>(out, params); },
        throwsParseFailure<type>(3, { 3, 7 }, HasSubstr("Invalid escape sequence")));
  }
}

TEST(escape, Regex_char) {
  using type = char;

  Result<type> result;

  {
    stringstream ss;
    string in = "\r\t\uffff()[a-z]";
    Regex::Params params;
    ss << escaped<Regex>(in, params, &result);
    string esc = "\\r\\t\\uffff\\(\\)\\[a-z\\]";
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

// EOF
