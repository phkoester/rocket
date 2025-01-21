/*
 * test-codec-global.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-global.h"

#include "rocket/S.h"
#include "rocket/codec.h"
#include "rocket/io.h"

#include "rocket-gtest/match.h"

using namespace rocket;
using namespace rocket::gtest::match;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(codec_global, opOutput_double) {
  using type = double;

  const numeric_limits<type> limits;

  EXPECT_EQ(S << raw(123456.7890123), "123457");
  EXPECT_EQ(S << raw(limits.lowest()), "-1.79769e+308");
  EXPECT_EQ(S << raw(limits.denorm_min()), "4.94066e-324");
  EXPECT_EQ(S << raw(-limits.denorm_min()), "-4.94066e-324");
  EXPECT_EQ(S << raw(limits.infinity()), "inf");
  EXPECT_EQ(S << raw(-limits.infinity()), "-inf");
  EXPECT_EQ(S << raw(limits.signaling_NaN()), "nan");
  EXPECT_EQ(S << raw(limits.quiet_NaN()), "nan");
}

TEST(codec_global, parseRon_bool) {
  using type = bool;

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("foo");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 0, 2 }, HasSubstr("\"fo\" does not match any of {\"0\", \"1\", \"false\", \"true\"}")));
    EXPECT_ISTREAM(is, true, false, 2);
  }

  {
    auto is = io::is("truex");
    parseRon(is, v);
    EXPECT_EQ(v, true);
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("0x");
    parseRon(is, v);
    EXPECT_EQ(v, false);
    EXPECT_ISTREAM(is, false, false, 1);
  }
}

TEST(codec_global, printRon_bool) {
  using type = bool;

  EXPECT_EQ(S << false, "false");
  EXPECT_EQ(S << true, "true");
  EXPECT_EQ(S << static_cast<type>(2), "true");
}

TEST(codec_global, parseRon_char) {
  using type = char;

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("foo");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("Expected '\\'', got 'f'")));
    EXPECT_ISTREAM(is, true, false, 1);
  }

  {
    auto is = io::is("'");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 0, 1 }, HasSubstr("Missing terminating '\\'' character")));
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::is("'\\q");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 3 }, HasSubstr("Invalid escape sequence")));
    EXPECT_ISTREAM(is, true, false, 3);
  }

  {
    auto is = io::is("'a");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(2, { 0, 2 }, HasSubstr("Missing terminating '\\'' character")));
    EXPECT_ISTREAM(is, true, true, 2);
  }
  {
    auto is = io::is("'\\xP");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(3, HasSubstr("Expected any of {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'a', 'b', 'c', 'd', 'e', 'f'}, got 'P'")));
    EXPECT_ISTREAM(is, true, false, 4);
  }

  {
    auto is = io::is("'a'");
    parseRon(is, v);
    EXPECT_EQ(v, 'a');
    EXPECT_ISTREAM(is, false, false, 3);
  }

  {
    auto is = io::is("'\\''");
    parseRon(is, v);
    EXPECT_EQ(v, '\'');
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("'\\\"'");
    parseRon(is, v);
    EXPECT_EQ(v, '"');
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("'\\\\'");
    parseRon(is, v);
    EXPECT_EQ(v, '\\');
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("'\\a'");
    parseRon(is, v);
    EXPECT_EQ(v, '\a');
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("'\\x80'");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 6 }, HasSubstr("Cannot parse \"'\\\\x80'\" as `char`")));
    EXPECT_ISTREAM(is, true, false, 6);
  }

  {
    auto is = io::is("'\\xfF'");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 6 }, HasSubstr("Cannot parse \"'\\\\xfF'\" as `char`")));
    EXPECT_ISTREAM(is, true, false, 6);
  }
}

TEST(codec_global, printRon_char) {
  using type = char;

  EXPECT_EQ(S << 'A', "'A'");
  EXPECT_EQ(S << '\'', "'\\\''");
  EXPECT_EQ(S << '\n', "'\\n'");
  EXPECT_EQ(S << '\\', "'\\\\'");
  EXPECT_EQ(S << type(127), "'\\x7f'");
  EXPECT_EQ(S << type(128), "'\\x80'");
  EXPECT_EQ(S << type(-1), "'\\xff'");
}

TEST(codec_global, parseRon_unsigned_char) {
  using type = unsigned char;

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("foo");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, HasSubstr("Expected any of {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'a', 'b', 'c', 'd', 'e', 'f'}, got 'o'")));
    EXPECT_ISTREAM(is, true, false, 2);
  }

  {
    auto is = io::is("ffa");
    parseRon(is, v);
    EXPECT_EQ(v, 255);
    EXPECT_ISTREAM(is, false, false, 2);
  }

  {
    auto is = io::is("fFa");
    parseRon(is, v);
    EXPECT_EQ(v, 255);
    EXPECT_ISTREAM(is, false, false, 2);
  }

  {
    auto is = io::is("0Ab");
    parseRon(is, v);
    EXPECT_EQ(v, 10);
    EXPECT_ISTREAM(is, false, false, 2);
  }
}

TEST(codec_global, printRon_unsigned_char) {
  using type = unsigned char;

  EXPECT_EQ(S << type(-1), "ff");
  EXPECT_EQ(S << type(0), "00");
  EXPECT_EQ(S << type(128), "80");
  EXPECT_EQ(S << type(255), "ff");
}

TEST(codec_global, parseRon_char32_t) {
  using type = char32_t;

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("foo");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("Expected '\\'', got 'f'")));
    EXPECT_ISTREAM(is, true, false, 1);
  }

  {
    auto is = io::is("'");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 0, 1 }, HasSubstr("Missing terminating '\\'' character")));
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::is("'\\q");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 3 }, HasSubstr("Invalid escape sequence")));
    EXPECT_ISTREAM(is, true, false, 3);
  }

  {
    auto is = io::is("'a");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(2, { 0, 2 }, HasSubstr("Missing terminating '\\'' character")));
    EXPECT_ISTREAM(is, true, true, 2);
  }
  {
    auto is = io::is("'\\xP");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(3, HasSubstr("Expected any of {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'a', 'b', 'c', 'd', 'e', 'f'}, got 'P'")));
    EXPECT_ISTREAM(is, true, false, 4);
  }

  {
    auto is = io::is("'a'");
    parseRon(is, v);
    EXPECT_EQ(v, 'a');
    EXPECT_ISTREAM(is, false, false, 3);
  }

  {
    auto is = io::is("'\\''");
    parseRon(is, v);
    EXPECT_EQ(v, '\'');
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("'\\\"'");
    parseRon(is, v);
    EXPECT_EQ(v, '"');
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("'\\\\'");
    parseRon(is, v);
    EXPECT_EQ(v, '\\');
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("'\\a'");
    parseRon(is, v);
    EXPECT_EQ(v, '\a');
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("'\\x80'");
    parseRon(is, v);
    EXPECT_EQ(static_cast<unsigned char>(v), 128);
    EXPECT_ISTREAM(is, false, false, 6);
  }

  {
    auto is = io::is("'\\xfF'");
    parseRon(is, v);
    EXPECT_EQ(static_cast<unsigned char>(v), 255);
    EXPECT_ISTREAM(is, false, false, 6);
  }

  {
    auto is = io::is("'\\ö'");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 4 }, HasSubstr("Invalid escape sequence")));
    EXPECT_ISTREAM(is, true, false, 4);
  }

  {
    auto is = io::is("'ö'");
    parseRon(is, v);
    EXPECT_EQ(v, U'ö');
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("'€'");
    parseRon(is, v);
    EXPECT_EQ(v, U'€');
    EXPECT_ISTREAM(is, false, false, 5);
  }

  {
    auto is = io::is("'\\ufe10'");
    parseRon(is, v);
    EXPECT_EQ(v, 0xfe10);
    EXPECT_ISTREAM(is, false, false, 8);
  }

  {
    auto is = io::is("'\\ufe10'");
    parseRon(is, v);
    EXPECT_EQ(v, 0xfe10);
    EXPECT_ISTREAM(is, false, false, 8);
  }

  {
    auto is = io::is("'\\U0010ffff'");
    parseRon(is, v);
    EXPECT_EQ(v, 0x0010ffff);
  }

  {
    auto is = io::is("'\\U00110000'");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 12 }, HasSubstr("Cannot parse \"'\\\\U00110000'\" as `char32_t`")));
    EXPECT_ISTREAM(is, true, false, 12);
  }

  {
    auto is = io::is("'\\Ufe1001ef'");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 12 }, HasSubstr("Cannot parse \"'\\\\Ufe1001ef'\" as `char32_t`")));
    EXPECT_ISTREAM(is, true, false, 12);
  }
}

TEST(codec_global, printRon_char32_t) {
  using type = char32_t;

  EXPECT_EQ(S << type(128), "'\\x80'");
  EXPECT_EQ(S << type(0x20ac), "'€'");
}

TEST(codec_global, parseRon_int32_t) {
  using type = int32_t; // Prints as `int`
  using biggerType = int64_t;

  const numeric_limits<type> limits;

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("foo");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 1 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, got 0 and 'f'")));
    EXPECT_ISTREAM(is, true, false, 1);
  }

  {
    auto is = io::is("++foo");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 2 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, got 0 and '+'")));
    EXPECT_ISTREAM(is, true, false, 2);
  }

  {
    auto is = io::is("++12x");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 2 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, got 0 and '+'")));
    EXPECT_ISTREAM(is, true, false, 2);
  }

  {
    auto is = io::is("+12x");
    parseRon(is, v);
    EXPECT_EQ(v, 12);
    EXPECT_ISTREAM(is, false, false, 3);
  }

  {
    auto is = io::is("+'");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 2 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, got 0 and '\\''")));
    EXPECT_ISTREAM(is, true, false, 2);
  }

  {
    auto is = io::is("-12'3''4''56''");
    parseRon(is, v);
    EXPECT_EQ(v, -123456);
    EXPECT_ISTREAM(is, false, false, 14);
  }

  {
    auto is = io::is("-123456");
    parseRon(is, v);
    EXPECT_EQ(v, -123456);
    EXPECT_ISTREAM(is, false, false, 7);
  }

  {
    auto is = io::is("-123456--");
    parseRon(is, v);
    EXPECT_EQ(v, -123456);
    EXPECT_ISTREAM(is, false, false, 7);
  }

  {
    biggerType value = static_cast<biggerType>(limits.min()) - 1;
    string s = codec::ron::print(value);
    auto is = io::is(s);
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 14 }, HasSubstr("Cannot parse \"-2'147'483'649\" as `int`")));
  }

  {
    auto value = limits.min();
    string s = codec::ron::print(value);
    auto is = io::is(s);
    parseRon(is, v);
    EXPECT_EQ(v, value);
  }

  {
    auto value = limits.max();
    string s = codec::ron::print(value);
    auto is = io::is(s);
    parseRon(is, v);
    EXPECT_EQ(v, value);
  }

  {
    biggerType value = static_cast<biggerType>(limits.max()) + 1;
    string s = codec::ron::print(value);
    auto is = io::is(s);
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 13 }, HasSubstr("Cannot parse \"2'147'483'648\" as `int`")));
  }
}

TEST(codec_global, printRon_int) {
  EXPECT_EQ(S << -4'711, "-4'711");
  EXPECT_EQ(S << 4'711, "4'711");
  EXPECT_EQ(S << -1'000'000, "-1'000'000");
}

TEST(codec_global, printRon_unsigned_int) {
  EXPECT_EQ(S << 4'711U, "4'711");
  EXPECT_EQ(S << 1'000'000U, "1'000'000");
}

TEST(codec_global, printRon_long) {
  EXPECT_EQ(S << -4'711L, "-4'711");
  EXPECT_EQ(S << 4'711L, "4'711");
  EXPECT_EQ(S << -1'000'000L, "-1'000'000");
}

TEST(codec_global, printRon_unsigned_long) {
  EXPECT_EQ(S << 4'711UL, "4'711");
  EXPECT_EQ(S << 1'000'000UL, "1'000'000");
}

TEST(codec_global, parseRon_int128_t) {
  using type = int128_t; // Prints as `__int128`

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("-");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(1, { 1, 2 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, got 0 and EOF")));
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::is("-999'999");
    parseRon(is, v);
    EXPECT_EQ(v, -999'999);
    EXPECT_ISTREAM(is, false, false, 8);
  }
}

TEST(codec_global, printRon_int128_t) {
  using type = int128_t;

  EXPECT_EQ(S << type(-1000000), "-1'000'000");
  EXPECT_EQ(S << type(1000000), "1'000'000");
}

TEST(codec_global, parseRon_uint128_t) {
  using type = uint128_t; // Prints as `unsigned __int128`

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("-");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 1 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, got 0 and '-'")));
    EXPECT_ISTREAM(is, true, false, 1);
  }

  {
    auto is = io::is("999'999");
    parseRon(is, v);
    EXPECT_EQ(v, 999'999);
    EXPECT_ISTREAM(is, false, false, 7);
  }
}

TEST(codec_global, printRon_uint128_t) {
  using type = uint128_t;

  EXPECT_EQ(S << type(1000000), "1'000'000");
}

TEST(codec_global, parseRon_float) {
  using type = float;

  const numeric_limits<type> limits;

  type v;

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("a");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 1 }, HasSubstr("Cannot parse \"a\" as `float`")));
    EXPECT_ISTREAM(is, true, false, 1);
  }

  {
    auto is = io::is(".2");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, .2F);
    EXPECT_ISTREAM(is, false, false, 2);
  }

  {
    auto is = io::is("2.");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, 2.F);
    EXPECT_ISTREAM(is, false, false, 2);
  }

  {
    auto is = io::is("1.2");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, 1.2F);
    EXPECT_ISTREAM(is, false, false, 3);
  }

  {
    auto is = io::is("+1.2");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, +1.2F);
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("-1.2");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, -1.2F);
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is(".");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 1 }, HasSubstr("Cannot parse \".\" as `float`")));
    EXPECT_ISTREAM(is, true, false, 1);
  }

  {
    auto is = io::is("1.e+3");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, 1.e+3F);
    EXPECT_ISTREAM(is, false, false, 5);
  }

  {
    auto is = io::is("1.2e-3");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, 0.0012F);
    EXPECT_ISTREAM(is, false, false, 6);
  }

  {
    auto is = io::is("10'000.12345");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, 10'000.12345);
    EXPECT_ISTREAM(is, false, false, 12);
  }

  {
    auto is = io::is("10'000.123'45");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, 10'000.123'45F);
    EXPECT_ISTREAM(is, false, false, 13);
  }

  {
    auto is = io::is("10'000.'123'45");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, 10'000.F);
    EXPECT_ISTREAM(is, false, false, 7);
  }

  {
    auto is = io::is("1..e+3");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, 1.F);
    EXPECT_ISTREAM(is, false, false, 2);
  }

  {
    auto is = io::is("inf");
    parseRon(is, v);
    EXPECT_EQ(v, limits.infinity());
    EXPECT_ISTREAM(is, false, false, 3);
  }

  {
    auto is = io::is("+inf");
    parseRon(is, v);
    EXPECT_EQ(v, limits.infinity());
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("-inf");
    parseRon(is, v);
    EXPECT_EQ(v, -limits.infinity());
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("qnanx");
    parseRon(is, v);
    EXPECT_TRUE(quietNan(v));
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("snan|");
    parseRon(is, v);
    EXPECT_TRUE(signalingNan(v));
    EXPECT_ISTREAM(is, false, false, 4);
  }

  {
    auto is = io::is("-3.4e");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(5, { 5, 6 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}")));
    EXPECT_ISTREAM(is, true, true, 5);
  }

  {
    auto is = io::is("-3.4E+");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(6, { 6, 7 }, HasSubstr("Expected at least 1 character contained in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}")));
    EXPECT_ISTREAM(is, true, true, 6);
  }

  {
    auto is = io::is("-3.4E3");
    parseRon(is, v);
    EXPECT_FLOAT_EQ(v, -3.4E3F);
    EXPECT_ISTREAM(is, false, false, 6);
  }

  {
    type value = -limits.max();
    string s = codec::ron::print(value);
    auto is = io::is(s);
    parseRon(is, v);
    EXPECT_EQ(v, value);
  }

  {
    type value = limits.max();
    string s = codec::ron::print(value);
    auto is = io::is(s);
    parseRon(is, v);
    EXPECT_EQ(v, value);
  }

  {
    auto is = io::is("3.4e+39");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(0, { 0, 7 }, HasSubstr("Cannot parse \"3.4e+39\" as `float`")));
    EXPECT_ISTREAM(is, true, false, 7);
  }
}

TEST(codec_global, printRon_float) {
  using type = float;

  EXPECT_THAT(S << 123456.78901F, matchesRegex("123'456\\.78.*"));
  EXPECT_THAT(S << -123456.78901F, matchesRegex("-123'456\\.78.*"));
  EXPECT_EQ(S << 1.0F / 3, "0.3333333432674407959");

  numeric_limits<type> limits;

  EXPECT_EQ(S << limits.signaling_NaN(), "snan");
  EXPECT_EQ(S << limits.quiet_NaN(), "qnan");
  auto qnan1 = nanf("1");
  static_assert(is_same_v<decltype(qnan1), type>);
  EXPECT_EQ(S << qnan1, "qnan");
  EXPECT_EQ(S << -limits.infinity(), "-∞");
  EXPECT_EQ(S << limits.infinity(), "∞");
}

TEST(codec_global, printRon_double) {
  using type = double;
  
  EXPECT_THAT(S << 123456.78901, StartsWith("123'456.7890"));
  EXPECT_THAT(S << -123456.78901, StartsWith("-123'456.7890"));
  EXPECT_EQ(S << 1.0 / 3, "0.33333333333333331483");

  EXPECT_THAT(S << 123456.789012, StartsWith("123'456.78901"));

  numeric_limits<type> limits;

  EXPECT_EQ(S << limits.signaling_NaN(), "snan");
  EXPECT_EQ(S << limits.quiet_NaN(), "qnan");
  auto qnan1 = nan("1");
  static_assert(is_same_v<decltype(qnan1), type>);
  EXPECT_EQ(S << qnan1, "qnan");
  EXPECT_EQ(S << -limits.infinity(), "-∞");
  EXPECT_EQ(S << limits.infinity(), "∞");
}

TEST(codec_global, printRon_long_double) {
  using type = long double;

  EXPECT_THAT(S << 123456.78901L, matchesRegex("123'456.7890.*"));
  EXPECT_THAT(S << -123456.78901L, matchesRegex("-123'456.7890.*"));
  EXPECT_EQ(S << 1.0L / 3, "0.33333333333333333334");

  EXPECT_EQ(S << 123456.789012L, "123'456.789012");

  numeric_limits<type> limits;

  EXPECT_EQ(S << limits.signaling_NaN(), "snan");
  EXPECT_EQ(S << limits.quiet_NaN(), "qnan");
  auto qnan1 = nanl("1");
  static_assert(is_same_v<decltype(qnan1), type>);
  EXPECT_EQ(S << qnan1, "qnan");
  EXPECT_EQ(S << -limits.infinity(), "-∞");
  EXPECT_EQ(S << limits.infinity(), "∞");
}

// EOF
