/*
 * test-nio.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/nio.h"

#include "rocket-gtest/matcher.h"

#include <random>

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace rocket::nio;
using namespace std;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(nio, Format) {
  random_device dev;
  mt19937 rng(dev());
  uniform_int_distribution<mt19937::result_type> dist(1, 11); // Distribution in range [1, 11]

  for (int i = 0; i < 100; ++i) {
    auto n = dist(rng);

    string buf;
    StringSink sink(buf);
    sink.print("n is {}.{}", n, nio::Format([&] {
      if (n == 11) {
        return Format::params();
      } if (n < 6) {
        return nio::Format::params(" This {} it is less than {:d}.", "means", 6);
      } else {
        return nio::Format::params(" This means it is greater {} {:d}.", "than", 5);
      }
    }));

    if (n == 11) {
      EXPECT_EQ(buf, "n is 11.");
    } else if (n < 6) {
      EXPECT_THAT(buf, matchesRegex("n is \\d+\\. This means it is less than 6\\."));
    } else {
      EXPECT_THAT(buf, matchesRegex("n is \\d+\\. This means it is greater than 5\\."));
    }
  }
}

TEST(nio, FormatWithTagged) {
  random_device dev;
  mt19937 rng(dev());
  uniform_int_distribution<mt19937::result_type> dist(1, 2); // Distribution in range [1, 2]

  for (int i = 0; i < 10; ++i) {
    auto n = dist(rng);

    string buf;
    StringSink sink(buf);
    sink.print("{}:{}{}", __FILE__, __LINE__, nio::Format([&] {
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
      EXPECT_THAT(buf, matchesRegex(".*:\\d+: First case: The command line is `grep -i foo bar`\\. Again, the command line is `grep -i foo bar`\\. But here comes another one: `ls -l`\\."));
    } else {
      EXPECT_THAT(buf, matchesRegex(".*:\\d+: Second case: This sentence doesn't mean much\\."));
    }
  }
}

TEST(nio, FileSink) {
  FileSink sink("/does/not/exist", FileSink::Params { .append=true });

  EXPECT_EQ(sink.error(), ENOENT);
  EXPECT_EQ(sink.good(), false);
  EXPECT_EQ(sink.open(), false);
  EXPECT_EQ(sink.file_, nullptr);

  sink.write("a");
  EXPECT_EQ(sink.error(), ENOENT);

  sink.close();
  EXPECT_EQ(sink.error(), ENOENT);

  sink.write("b");
  EXPECT_EQ(sink.error(), ENOENT);
}

TEST(nio, StreamSink) {
  ostringstream os;
  StreamSink sink(os);
  sink.println("Hi {}", "there");
  EXPECT_EQ(os.str(), "Hi there\n");
}

// EOF
