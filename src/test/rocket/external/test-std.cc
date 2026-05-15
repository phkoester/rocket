/*
 * test-std.cc
 *
 * Tests related to the STL.
 */

#include "rocket-test/rocket-test.h"

#include "rocket/io/io.h"
#include "rocket/nio/nio.h"

#include <chrono>
#include <filesystem>
#include <regex>
#include <vector>

// TEST -----------------------------------------------------------------------------------------------------

TEST(std, f32OpCmp) {
  using type = f32;
  using limits = numeric_limits<type>;
  constexpr auto nan = limits::quiet_NaN();

  static_assert(std::is_lt(-limits::infinity() <=> 0.0F));
  static_assert(std::is_lt(0.0F <=> limits::infinity()));
  static_assert(std::is_lteq(0.0F <=> 0.0F)); // NOLINT
  static_assert(std::is_gteq(0.0F <=> 0.0F)); // NOLINT
  static_assert(std::is_lt(1.0F <=> 2.0F));
  static_assert(std::is_eq(1.0F <=> 1.0F)); // NOLINT
  static_assert(std::is_gt(2.0F <=> 1.0F));
  static_assert((nan <=> 0.0F) == std::partial_ordering::unordered);
  static_assert((0.0F <=> nan) == std::partial_ordering::unordered);
  static_assert((nan <=> nan) == std::partial_ordering::unordered); // NOLINT
}

// Segfaults on Windows with Clang 20.1.8
TEST(std, rethrowException) {
  try {
    throw runtime_error("oops");
  } catch (const runtime_error&) {
    try {
      cout << "throwing nested\n";
      throw_with_nested(runtime_error("oopsers"));
    } catch (const runtime_error& ex) {
      cout << "caught runtime error: " << ex.what() << endl; // NOLINT
      try {
        cout << "rethrowing nested" << endl; // NOLINT
        // Segfaults
        rethrow_exception(current_exception());
      } catch (const runtime_error& ex) {
        cout << "caught runtime error: " << ex.what() << endl; // NOLINT
      } catch (...) {
        cout << "caught ..." << endl; // NOLINT
      }
    }
  }
}

TEST(std, chronoFormat) {
  {
    // Local time in ISO-8601, with microseconds
    const chrono::time_point tp = time_point_cast<chrono::microseconds>(chrono::system_clock::now());
    // const auto zt { chrono::zoned_time{ chrono::current_zone(), tp } };
    const chrono::zoned_time zt { chrono::current_zone(), tp };
    const string str = std::format("{:%FT%T%Ez}", zt);
    EXPECT_THAT(str, matchesRegex("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}\\.\\d{6}[+-]\\d{2}:\\d{2}"));
  }

  {
    // UTC time in ISO-8601, with microseconds
    const chrono::time_point tp = time_point_cast<chrono::microseconds>(chrono::utc_clock::now());
    const string str = std::format("{:%FT%TZ}", tp);
    EXPECT_THAT(str, matchesRegex("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}\\.\\d{6}Z"));
  }
}

/// This is expected to work on all platforms.
TEST(std, filesystemPath) {
  using filesystem::path;

  const path unixPath("/path/to/file");
  EXPECT_EQ(unixPath.filename(), path("file"));
  EXPECT_EQ(unixPath.parent_path(), path("/path/to"));
}

#ifdef ROCKET_OS_WINDOWS
TEST(std, filesystemPathWindows) {
  using filesystem::path;

  const path windowsPath("C:\\path\\to\\file");
  EXPECT_EQ(windowsPath.filename(), path("file"));
  EXPECT_EQ(windowsPath.parent_path(), path("C:\\path\\to"));

  const path mixedPath("a/b\\c/d\\e");
  EXPECT_EQ(mixedPath.filename(), path("e"));
  EXPECT_EQ(mixedPath.parent_path(), path("a/b/c/d"));
  EXPECT_EQ(mixedPath.parent_path(), path("a\\b\\c\\d"));
}
#endif // ROCKET_OS_WINDOWS

TEST(std, formatLocale) {
  EXPECT_EQ(std::format("{:L}", 123'456), "123,456");
  EXPECT_EQ(std::format("{:L}", 123456.789012), "123,456.789012");
}

TEST(std, istreamDefaultExceptions) {
  auto is = io::is();
  EXPECT_EQ(is.exceptions(), 0);
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
    char c = '\0';
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
    type val = false;
    is >> boolalpha >> val;
    EXPECT_EQ(val, false);
    // After reading an incomplete 'bool', there is a failure and an EOF
    EXPECT_ISTREAM(is, true, true, 3);
    EXPECT_EQ(is.tellg(), -1);
  }

  {
    auto is = io::is("true");
    type val = false;
    is >> boolalpha >> val;
    EXPECT_EQ(val, true);
#ifdef ROCKET_OS_WINDOWS
    // After reading a complete 'bool', there is no failure, but an EOF
    EXPECT_ISTREAM(is, false, true, 4);
    EXPECT_EQ(is.tellg(), -1);
#else
    // After reading a complete 'bool', there is no failure and no EOF
    EXPECT_ISTREAM(is, false, false, 4);
    EXPECT_EQ(is.tellg(), 4);
#endif
  }
}

TEST(std, istreamChar) {
  auto is = io::is("a");
  char c = '\0';
  is >> c;
  EXPECT_EQ(c, 'a');
}

TEST(std, istreamI32) {
  using type = i32;

  {
    auto is = io::is();
    type val = 0;
    is >> val;
    EXPECT_EQ(val, 0);
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("a");
    type val = 0;
    is >> val;
    EXPECT_EQ(val, 0);
    EXPECT_ISTREAM(is, true, false, 0);
  }

  {
    auto is = io::is("-");
    type val = 0;
    is >> val;
    EXPECT_EQ(val, 0);
    // After reading an incomplete 'i32', there is a failure and an EOF
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::is("-12");
    type val = 0;
    is >> val;
    EXPECT_EQ(val, -12);
    // After reading a complete 'i32', there is no failure but an EOF
    EXPECT_ISTREAM(is, false, true, 3);
    EXPECT_EQ(is.tellg(), -1);
  }

  {
    nio::StringSink buf;
    buf.print("{}", numeric_limits<i32>::max() + static_cast<i64>(1));
    auto is = io::is(buf.str());
    type val = 0;
    is >> val;
#ifdef ROCKET_OS_WINDOWS
    // If the value is greater than the maximum value, there is a failure
    EXPECT_ISTREAM(is, true, true, 10);
    EXPECT_EQ(is.tellg(), -1);
#else
    // If the value is greater than the maximum value, then the result is capped
    EXPECT_EQ(val, numeric_limits<i32>::max());
    EXPECT_ISTREAM(is, true, true, 10);
    EXPECT_EQ(is.tellg(), -1);
#endif
  }
}

TEST(std, istreamF64) {
  using type = f64;

  ostringstream os;
  os << -numeric_limits<type>::infinity();
  EXPECT_EQ(os.str(), "-inf");

  {
    // Reading symbols doesn't work
    auto is = io::is("-inf");
    type val = 0;
    is >> val;
    EXPECT_EQ(val, 0);
  }
}

#ifdef ROCKET_HAS_128

TEST(std, istreamF128) {
  using type = f128;

  {
    auto is = io::is("1.2");
    type val = 0;
    is >> val;
    EXPECT_EQ(val, 1.2L);
    EXPECT_ISTREAM(is, false, true, 3);
  }
}

#endif // ROCKET_HAS_128

TEST(std, regexGreedy) {
  string str = "1: 2: 3: 4";
  const regex re(".*: ");
  vector<string> matches;
  for (smatch match; regex_search(str, match, re);) {
    matches.push_back(match.str());
    str = match.suffix();
  }
  EXPECT_EQ(matches, vector<string> { "1: 2: 3: " });
}

TEST(std, regexNongreedy) {
  string str = "1: 2: 3: 4";
  const regex re(".*?: ");
  vector<string> matches;
  for (smatch match; regex_search(str, match, re);) {
    matches.push_back(match.str());
    str = match.suffix();
  }
  EXPECT_EQ(matches, (vector<string> { "1: ", "2: ", "3: " }));
}

TEST(std, regexLineFeed) {
  const string str = "aaa\nbbb";
  EXPECT_TRUE(regex_match(str, regex("a+\nb+")));
  EXPECT_FALSE(regex_match(str, regex("a.*b"))); // '.' does not match line feed
  EXPECT_TRUE(regex_match(str, regex("a[^]*b"))); // '[^]' (not nothing) matches line feed
}

TEST(std, vectorOpLt) {
  using type = vector<i32>;

  EXPECT_TRUE((type { 1, 2, 3 }) < (type { 1, 2, 4 }));
  EXPECT_TRUE((type { 1, 2, 3 }) < (type { 1, 2, 3, 4 }));
  EXPECT_TRUE((type { 4, 3, 2 }) > (type { 3, 2, 1, 0 }));
  EXPECT_TRUE((type { 4, 3, 2, 1 }) > (type { 4, 3, 2 }));
}

// EOF
