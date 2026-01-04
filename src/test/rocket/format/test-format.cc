/*
 * test-format.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/format/format.h"
#include "rocket/nio/nio.h"

#include "rocket-gtest/matcher/matcher.h"

using namespace rocket;
using namespace rocket::format;
using namespace rocket::gtest::matcher;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(format, FormatChar) {
  auto lambda = [](int n) -> string {
    nio::StringSink buf;
    buf.print("n is {}.{}", n, Format<char>([&] {
      if (n == 11) {
        return Format<char>::params();
      } if (n < 6) {
        return Format<char>::params(" This {} it is less than {:d}.", "means", 6);
      } else {
        return Format<char>::params(" This means it is greater {} {:d}.", "than", 5);
      }
    }));
    return buf.str();
  };

  EXPECT_EQ(lambda(5), "n is 5. This means it is less than 6.");
  EXPECT_EQ(lambda(6), "n is 6. This means it is greater than 5.");
  EXPECT_EQ(lambda(11), "n is 11.");
}

TEST(format, FormatCharWithTagged) {
  auto lambda = [](int n) -> string {
    nio::StringSink buf;
    buf.print("{}:{}{}", __FILE__, __LINE__, Format<char>([&] {
      if (n == 1) {
        auto params = Format<char>::params(": First case: The {0} is `@@`{1} Again, the {0} is `@@`{1} But here comes another one: `⊕`{1}", "command line", ".");
        params.tag("@@", "grep {} {} {}", "-i", "foo", "bar");
        params.tag("⊕", "{} -l", "ls");
        return params;
      } else {
        auto params = Format<char>::params(": Second {}: \\1.", "case");
        params.tag("\\1", "This {} doesn't mean much", "sentence");
        return params;
      }
    }));
    return buf.str();
  };

  EXPECT_THAT(lambda(1), matchesRegex(".*\\.cc:\\d+: First case: The command line is `grep -i foo bar`\\. Again, the command line is `grep -i foo bar`\\. But here comes another one: `ls -l`\\."));
  EXPECT_THAT(lambda(2), matchesRegex(".*\\.cc:\\d+: Second case: This sentence doesn't mean much\\."));
}

TEST(format, FormatChar32) {
  auto s = fmt::format(U"a {} d", Format<char32_t>([&] {
    return Format<char32_t>::params(fmt::format(U"{} {}", U'b', U'c'));
  }));
  EXPECT_EQ(s, U"a b c d");
}

// EOF
