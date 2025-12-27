/*
 * test-escape.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-rocket-decl.h"
#include "rocket/codec-std-decl.h"
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

TEST(escape, CString) {
  Result result;

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
    auto val = escaped<CString>(out, params, &result);
    ss >> val;
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
    auto val = escaped<CString>(out, params, &result);
    ss >> val;
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
    auto val = escaped<CString>(out, params, &result);
    ss >> val;
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
    auto val = escaped<CString>(out, params, &result);
    ss >> val;
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
    auto val = escaped<CString>(out, params);
    EXPECT_THAT(
        [&] { io::resetg(is) >> val; },
        throwsParseFailure(5, { 0, 5 }, HasSubstr("Missing terminating '\"' character")));
  }

  {
    auto is = io::is("abc\\");
    string out;
    CString::Params params;
    auto val = escaped<CString>(out, params);
    EXPECT_THAT(
        [&] { io::resetg(is) >> val; },
        throwsParseFailure(4, { 3, 4 }, HasSubstr("Expected a Unicode grapheme, got EOF")));
  }

  {
    auto is = io::is("abc\\Ä");
    string out;
    CString::Params params;
    auto val = escaped<CString>(out, params);
    EXPECT_THAT(
        [&] { io::resetg(is) >> val; },
        throwsParseFailure(3, { 3, 6 }, HasSubstr("Invalid escape sequence")));
  }

  {
    // 🧑‍🌾: 11 bytes, 3 code points
    auto is = io::is("abc\\🧑‍🌾");
    string out;
    CString::Params params;
    auto val = escaped<CString>(out, params);
    EXPECT_THAT(
        [&] { io::resetg(is) >> val; },
        throwsParseFailure(3, { 3, 15 }, HasSubstr("Invalid escape sequence")));
  }
}

TEST(escape, Regex) {
  Result result;

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
    auto val = escaped<Regex>(out, params, &result);
    ss >> val;
    EXPECT_EQ(out, in);
    EXPECT_EQ(result.input, esc);
    EXPECT_EQ(
        result.positions,
        positions({ { 0, 0 }, { 2, 1 }, { 4, 2 }, { 10, 5 }, { 12, 6 }, { 14, 7 }, { 16, 8 }, { 17, 9 }, { 18, 10 }, { 19, 11 }, { 21, 12 } }));
  }
}

// EOF
