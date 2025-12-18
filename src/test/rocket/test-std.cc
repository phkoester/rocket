/*
 * test-std.cc
 *
 * Tests related to the STL.
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/Guard.h"
#include "rocket/io.h"

#include "rocket-gtest/matcher.h"

#include <chrono>
#include <format>
#include <regex>

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace std;
using namespace testing;

// TEST -----------------------------------------------------------------------------------------------------

TEST(std, chrono) {
  {
    // Local time in ISO-8601, with microseconds
    chrono::time_point tp = time_point_cast<chrono::microseconds>(chrono::system_clock::now());
    // const auto zt { chrono::zoned_time{ chrono::current_zone(), tp } };
    chrono::zoned_time zt { chrono::current_zone(), tp };
    string s = format("{:%FT%T%Ez}", zt);
    EXPECT_THAT(s, matchesRegex("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}\\.\\d{6}[+-]\\d{2}:\\d{2}"));
  }

  {
    // UTC time in ISO-8601, with microseconds
    chrono::time_point tp = time_point_cast<chrono::microseconds>(chrono::utc_clock::now());
    string s = format("{:%FT%TZ}", tp);
    EXPECT_THAT(s, matchesRegex("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}\\.\\d{6}Z"));
  }
}

/**
 * This test might require configuring the `en_US.UTF8` locale. On Ubuntu, this can be done with
 *
 * ```bash
 * sudo locale-gen en_US.UTF-8
 * sudo dpkg-reconfigure locales
 * ```
 */
TEST(std, formatEnUsUtf8) {
  locale l = locale::global(locale("en_US.UTF8"));
  ROCKET_GUARD([&] { locale::global(l); });
  EXPECT_EQ(format("{:L}", 123'456), "123,456");
  EXPECT_EQ(format("{:L}", 123456.789012), "123,456.789012");
}

TEST(std, istreamDefaultExceptions) {
  {
    auto is = istringstream();
    EXPECT_EQ(is.exceptions(), 0);
  }

  {
    auto is = io::is();
    EXPECT_EQ(is.exceptions(), ios::badbit);
  }
}

TEST(std, istreamEof) {
  {
    auto is = io::is();
    // Check 'tellg()' once
    EXPECT_EQ(is.tellg(), 0);
    // Check 'tellg()' twice
    EXPECT_EQ(is.tellg(), 0);
    is.setstate(ios::eofbit);
    // Setting 'eofbit' doesn't immediately affect 'fail()'
    EXPECT_FALSE(is.fail());
    // 'tellg()' affects() 'fail()' when 'eofbit' is set
    EXPECT_EQ(is.tellg(), -1);
    EXPECT_TRUE(is.fail());
  }

  {
    auto is = io::is();
    char c;
    is >> c;
    EXPECT_TRUE(is.eof());
    // 'eof()' implies 'fail()'
    EXPECT_TRUE(is.fail());
    EXPECT_EQ(is.tellg(), -1);
    // 'seekg' resets 'eof()', but not 'fail()'
    is.seekg(0);
    EXPECT_FALSE(is.eof());
    EXPECT_TRUE(is.fail());
  }
}

TEST(std, istream_bool) {
  using type = bool;

  {
    auto is = io::is("tru");
    type v = false;
    is >> boolalpha >> v;
    EXPECT_EQ(v, false);
    // After reading an incomplete 'bool', there is a failure and an EOF
    EXPECT_ISTREAM(is, true, true, 3);
    EXPECT_EQ(is.tellg(), -1);
  }

  {
    auto is = io::is("true");
    type v = false;
    is >> boolalpha >> v;
    EXPECT_EQ(v, true);
    // After reading a complete 'bool', there is no failure and no EOF
    EXPECT_ISTREAM(is, false, false, 4);
    EXPECT_EQ(is.tellg(), 4);
  }
}

TEST(std, istream_char_char) {
  auto is = basic_istringstream<char>("a");
  char c;
  is >> c;
  EXPECT_EQ(c, 'a');
}

TEST(std, istream_char_int) {
  auto is = basic_istringstream<char>("12");
  int i;
  is >> i;
  EXPECT_EQ(i, 12);
}

/**
 * This tests `char32_t` support from `locale-char32_t.h`.
 */
TEST(std, istream_char32_t_char32_t) {
  auto is = basic_istringstream<char32_t>(U"a");
  char32_t c(U'\0'); // GNU needs initialization here
  is >> c;
  EXPECT_EQ(c, U'a');
}

/**
 * This tests `char32_t` support from `locale-char32_t.h`.
 */
TEST(std, istream_char32_t_int) {
  auto is = basic_istringstream<char32_t>(U"12");
  int i;
  is >> i;
  EXPECT_EQ(i, 12);
}

TEST(std, istream_wchar_t_wchar_t) {
  auto is = basic_istringstream<wchar_t>(L"a");
  wchar_t c;
  is >> c;
  EXPECT_EQ(c, L'a');
}

TEST(std, istream_wchar_t_int) {
  auto is = basic_istringstream<wchar_t>(L"12");
  int i;
  is >> i;
  EXPECT_EQ(i, 12);
}

TEST(std, istream_int) {
  using type = int;

  {
    auto is = io::is();
    type v = 0;
    is >> v;
    EXPECT_EQ(v, 0);
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("a");
    type v = 0;
    is >> v;
    EXPECT_EQ(v, 0);
    EXPECT_ISTREAM(is, true, false, 0);
  }

  {
    auto is = io::is("-");
    type v = 0;
    is >> v;
    EXPECT_EQ(v, 0);
    // After reading an incomplete 'int', there is a failure and an EOF
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::is("-12");
    type v = 0;
    is >> v;
    EXPECT_EQ(v, -12);
    // After reading a complete 'int', there is no failure but an EOF
    EXPECT_ISTREAM(is, false, true, 3);
    EXPECT_EQ(is.tellg(), -1);
  }

  {
    long l = numeric_limits<int>::max() + 1L;
    ostringstream os;
    os.imbue(locale::classic());
    os << l;
    auto is = io::is(os.str());
    type v = 0;
    is >> v;
    // If the value is greater than the maximum value, then the result is capped!
    EXPECT_EQ(v, numeric_limits<int>::max());
    EXPECT_ISTREAM(is, true, true, 10);
    EXPECT_EQ(is.tellg(), -1);
  }
}

TEST(std, istream_double) {
  using type = double;

  // Reading symbols doesn't work
  auto is = io::is("-inf");
  type v = 0;
  is >> v;
  EXPECT_EQ(v, 0);
}

TEST(std, istream_long_double) {
  using type = long double;

  {
    auto is = io::is("1.2");
    type v;
    is >> v;
    EXPECT_EQ(v, 1.2L);
    EXPECT_ISTREAM(is, false, true, 3);
  }
}

TEST(std, ostream_char_int) {
  basic_ostringstream<char> os;
  os << 42;
  EXPECT_EQ(os.str(), "42");
}

/**
 * This tests `char32_t` support from `locale-char32_t.h`.
 */
TEST(std, ostream_char32_t_int) {
  basic_ostringstream<char32_t> os;
  os << 42;
  EXPECT_EQ(os.str(), U"42");
}

TEST(std, ostream_wchar_t_int) {
  basic_ostringstream<wchar_t> os;
  os << 42;
  EXPECT_EQ(os.str(), L"42");
}

TEST(std, ostream_double) {
  EXPECT_EQ(S << raw(-numeric_limits<double>::infinity()), "-inf");
  EXPECT_EQ(S << raw(numeric_limits<double>::infinity()), "inf");
  EXPECT_EQ(S << raw(numeric_limits<double>::quiet_NaN()), "nan");
  EXPECT_EQ(S << raw(numeric_limits<double>::signaling_NaN()), "nan");
}

TEST(std, ostream_long_double) {
  EXPECT_EQ(S << raw(1.2L), "1.2");
}

TEST(std, regexGreedy) {
  string s = "1: 2: 3: 4";
  regex re(".*: ");
  vector<string> matches;
  for (smatch match; regex_search(s, match, re);) {
    matches.push_back(match.str());
    s = match.suffix();
  }
  EXPECT_EQ(matches, vector<string> { "1: 2: 3: " });
}

TEST(std, regexNonGreedy) {
  string s = "1: 2: 3: 4";
  regex re(".*?: ");
  vector<string> matches;
  for (smatch match; regex_search(s, match, re);) {
    matches.push_back(match.str());
    s = match.suffix();
  }
  EXPECT_EQ(matches, (vector<string> { "1: ", "2: ", "3: " }));
}

TEST(std, regexLineFeed) {
  string s = "aaa\nbbb";
  EXPECT_TRUE(regex_match(s, regex("a+\nb+")));
  EXPECT_FALSE(regex_match(s, regex("a.*b"))); // '.' does not match line feed
  EXPECT_TRUE(regex_match(s, regex("a[^]*b"))); // '[^]' (not nothing) matches line feed
}

// EOF
