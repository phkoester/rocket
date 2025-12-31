/*
 * test-std.cc
 *
 * Tests related to the STL.
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/io.h"

#include "rocket-gtest/matcher.h"

#include <chrono>
#include <regex>

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace std;
using namespace testing;

// TEST -----------------------------------------------------------------------------------------------------

TEST(std, chronoFormat) {
  {
    // Local time in ISO-8601, with microseconds
    chrono::time_point tp = time_point_cast<chrono::microseconds>(chrono::system_clock::now());
    // const auto zt { chrono::zoned_time{ chrono::current_zone(), tp } };
    chrono::zoned_time zt { chrono::current_zone(), tp };
    string s = std::format("{:%FT%T%Ez}", zt);
    EXPECT_THAT(s, matchesRegex("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}\\.\\d{6}[+-]\\d{2}:\\d{2}"));
  }

  {
    // UTC time in ISO-8601, with microseconds
    chrono::time_point tp = time_point_cast<chrono::microseconds>(chrono::utc_clock::now());
    string s = std::format("{:%FT%TZ}", tp);
    EXPECT_THAT(s, matchesRegex("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}\\.\\d{6}Z"));
  }
}

TEST(std, formatLocale) {
  EXPECT_EQ(std::format("{:L}", 123'456), "123,456");
  EXPECT_EQ(std::format("{:L}", 123456.789012), "123,456.789012");
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

TEST(std, istreamBool) {
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

TEST(std, istreamChar) {
  auto is = io::is("a");
  char c;
  is >> c;
  EXPECT_EQ(c, 'a');
}

TEST(std, istreamInt) {
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
    nio::StringSink buf;
    buf.print("{}", numeric_limits<int>::max() + 1L);
    auto is = io::is(buf.str());
    type v = 0;
    is >> v;
    // If the value is greater than the maximum value, then the result is capped!
    EXPECT_EQ(v, numeric_limits<int>::max());
    EXPECT_ISTREAM(is, true, true, 10);
    EXPECT_EQ(is.tellg(), -1);
  }
}

TEST(std, istreamDouble) {
  using type = double;

  ostringstream os;
  os << -numeric_limits<type>::infinity();
  EXPECT_EQ(os.str(), "-inf");

  {
    // Reading symbols doesn't work
    auto is = io::is("-inf");
    type v = 0;
    is >> v;
    EXPECT_EQ(v, 0);
  }
}

TEST(std, istreamLongDouble) {
  using type = long double;

  {
    auto is = io::is("1.2");
    type v;
    is >> v;
    EXPECT_EQ(v, 1.2L);
    EXPECT_ISTREAM(is, false, true, 3);
  }
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

TEST(std, regexNongreedy) {
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
