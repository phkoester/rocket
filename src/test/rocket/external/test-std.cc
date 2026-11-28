// test-std.cc
//
// Tests related to the STL.

#include "rocket-test/rocket-test.h"

#include "rocket/io/io.h"
#include "rocket/nio/nio.h"

#include <filesystem>
#include <regex>
#include <vector>

using namespace rocket;

#define MODULE rocketExternalStd

// Tests ====================================================================================================

TEST(MODULE, f32OpCmp) {
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

/// Segfaults on Windows with Clang 22.1.3.
///
/// - https://github.com/llvm/llvm-project/issues/158302
/// - https://github.com/llvm/llvm-project/issues/191462
///
/// The crash happens when #std::current_exception() is called inside a nested exception handler.
TEST(MODULE, rethrowException) {
  try {
    throw runtime_error("oops");
  } catch (const runtime_error&) {
    try {
      cout << "Throwing nested\n";
      throw_with_nested(runtime_error("oopsers"));
    } catch (const runtime_error& ex) {
      cout << "Caught runtime error: " << ex.what() << '\n'; // NOLINT
      try {
        cout << "Rethrowing nested\n"; // NOLINT
        // Segfaults
        rethrow_exception(current_exception());
      } catch (const runtime_error& ex) {
        cout << "Caught runtime error: " << ex.what() << '\n'; // NOLINT
      } catch (...) {
        cout << "Caught ...\n"; // NOLINT
      }
    }
  }
}

TEST(MODULE, filesystemPath) {
  namespace fs = std::filesystem;

  const fs::path unixPath("/path/to/file");
  EXPECT_EQ(unixPath.filename(), fs::path("file"));
  EXPECT_EQ(unixPath.parent_path(), fs::path("/path/to"));

#ifdef ROCKET_OS_WINDOWS
  const fs::path windowsPath("C:\\path\\to\\file");
  EXPECT_EQ(windowsPath.filename(), fs::path("file"));
  EXPECT_EQ(windowsPath.parent_path(), fs::path("C:\\path\\to"));

  const fs::path mixedPath("a/b\\c/d\\e");
  EXPECT_EQ(mixedPath.filename(), fs::path("e"));
  EXPECT_EQ(mixedPath.parent_path(), fs::path("a/b/c/d"));
  EXPECT_EQ(mixedPath.parent_path(), fs::path("a\\b\\c\\d"));
#endif // ROCKET_OS_WINDOWS
}

TEST(MODULE, istreamDefaultExceptions) {
  auto is = io::testing::is();
  EXPECT_EQ(is.exceptions(), 0);
}

TEST(MODULE, istreamEof) {
  {
    auto is = io::testing::is();
    // Check 'tellg()' once
    EXPECT_EQ(is.tellg(), 0);
    // Check 'tellg()' twice
    EXPECT_EQ(is.tellg(), 0);
    is.setstate(ios::eofbit);
    // Setting 'eofbit' doesn’t immediately affect 'fail()'
    EXPECT_FALSE(is.fail());
    // 'tellg()' affects() 'fail()' when 'eofbit' is set
    EXPECT_EQ(is.tellg(), -1);
    EXPECT_TRUE(is.fail());
  }

  {
    auto is = io::testing::is();
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

TEST(MODULE, istreamBool) {
  using type = bool;

  {
    auto is = io::testing::is("tru");
    type val = false;
    is >> boolalpha >> val;
    EXPECT_EQ(val, false);
    // After reading an incomplete 'bool', there is a failure and an EOF
    EXPECT_ISTREAM(is, true, true, 3);
    EXPECT_EQ(is.tellg(), -1);
  }

  {
    auto is = io::testing::is("true");
    type val = false;
    is >> boolalpha >> val;
    EXPECT_EQ(val, true);
  }
}

TEST(MODULE, istreamChar) {
  auto is = io::testing::is("a");
  char c = '\0';
  is >> c;
  EXPECT_EQ(c, 'a');
}

TEST(MODULE, istreamI32) {
  using type = i32;

  {
    auto is = io::testing::is();
    type val = 0;
    is >> val;
    EXPECT_EQ(val, 0);
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::testing::is("a");
    type val = 0;
    is >> val;
    EXPECT_EQ(val, 0);
    EXPECT_ISTREAM(is, true, false, 0);
  }

  {
    auto is = io::testing::is("-");
    type val = 0;
    is >> val;
    EXPECT_EQ(val, 0);
    // After reading an incomplete 'i32', there is a failure and an EOF
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::testing::is("-12");
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
    auto is = io::testing::is(buf.str());
    type val = 0;
    is >> val;
#ifdef ROCKET_OS_WINDOWS
    // Windows: If the value is greater than the maximum value, there is a failure
    EXPECT_ISTREAM(is, true, true, 10);
    EXPECT_EQ(is.tellg(), -1);
#else
    // Linux, macOS: If the value is greater than the maximum value, then the result is capped
    EXPECT_EQ(val, numeric_limits<i32>::max());
    EXPECT_ISTREAM(is, true, true, 10);
    EXPECT_EQ(is.tellg(), -1);
#endif
  }
}

TEST(MODULE, istreamF64) {
  using type = f64;

  ostringstream os;
  os << -numeric_limits<type>::infinity();
  EXPECT_EQ(os.str(), "-inf");

  {
    // Reading symbols doesn’t work
    auto is = io::testing::is("-inf");
    type val = 0;
    is >> val;
    EXPECT_EQ(val, 0);
  }
}

TEST(MODULE, regexGreedy) {
  string str = "1: 2: 3: 4";
  const regex re(".*: ");
  vector<string> matches;
  for (smatch match; regex_search(str, match, re);) {
    matches.push_back(match.str());
    str = match.suffix();
  }
  EXPECT_EQ(matches, vector<string> { "1: 2: 3: " });
}

TEST(MODULE, regexNongreedy) {
  string str = "1: 2: 3: 4";
  const regex re(".*?: ");
  vector<string> matches;
  for (smatch match; regex_search(str, match, re);) {
    matches.push_back(match.str());
    str = match.suffix();
  }
  EXPECT_EQ(matches, (vector<string> { "1: ", "2: ", "3: " }));
}

TEST(MODULE, regexLineFeed) {
  const string str = "aaa\nbbb";
  EXPECT_TRUE(regex_match(str, regex("a+\nb+")));
  EXPECT_FALSE(regex_match(str, regex("a.*b"))); // '.' does not match line feed
  EXPECT_TRUE(regex_match(str, regex("a[^]*b"))); // '[^]' (not nothing) matches line feed
}

TEST(MODULE, vectorOpCmp) {
  using type = vector<i32>;

  EXPECT_LT((type { 1, 2, 3 }), (type { 1, 2, 4 }));
  EXPECT_LT((type { 1, 2, 3 }), (type { 1, 2, 3, 4 }));
  EXPECT_GT((type { 4, 3, 2 }), (type { 3, 2, 1, 0 }));
  EXPECT_GT((type { 4, 3, 2, 1 }), (type { 4, 3, 2 }));
}

// EOF
