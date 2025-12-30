/*
 * test-format.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/format.h"
#include "rocket/random.h"

#include "rocket-gtest/matcher.h"

using namespace rocket;
using namespace rocket::format;
using namespace rocket::gtest::matcher;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(format, Format) {
  auto gen = random::gen();

  for (int i = 0; i < 100; ++i) {
    auto n = random::random(gen, 1, 11);

    nio::StringSink buf;
    buf.print("n is {}.{}", n, Format([&] {
      if (n == 11) {
        return Format::params();
      } if (n < 6) {
        return Format::params(" This {} it is less than {:d}.", "means", 6);
      } else {
        return Format::params(" This means it is greater {} {:d}.", "than", 5);
      }
    }));

    if (n == 11) {
      EXPECT_EQ(buf.str(), "n is 11.");
    } else if (n < 6) {
      EXPECT_THAT(buf.str(), matchesRegex("n is \\d+\\. This means it is less than 6\\."));
    } else {
      EXPECT_THAT(buf.str(), matchesRegex("n is \\d+\\. This means it is greater than 5\\."));
    }
  }
}

TEST(format, FormatWithTagged) {
  auto gen = random::gen();

  for (int i = 0; i < 10; ++i) {
    auto n = random::random(gen, 1, 2);

    nio::StringSink buf;
    buf.print("{}:{}{}", __FILE__, __LINE__, Format([&] {
      if (n == 1) {
        auto params = Format::params(": First case: The {0} is `@@`{1} Again, the {0} is `@@`{1} But here comes another one: `⊕`{1}", "command line", ".");
        params.tag("@@", "grep {} {} {}", "-i", "foo", "bar");
        params.tag("⊕", "{} -l", "ls");
        return params;
      } else {
        auto params = Format::params(": Second {}: \\1.", "case");
        params.tag("\\1", "This {} doesn't mean much", "sentence");
        return params;
      }
    }));

    if (n == 1) {
      EXPECT_THAT(buf.str(), matchesRegex(".*\\.cc:\\d+: First case: The command line is `grep -i foo bar`\\. Again, the command line is `grep -i foo bar`\\. But here comes another one: `ls -l`\\."));
    } else {
      EXPECT_THAT(buf.str(), matchesRegex(".*\\.cc:\\d+: Second case: This sentence doesn't mean much\\."));
    }
  }
}

// EOF
