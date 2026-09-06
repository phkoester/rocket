/*
 * test-Subformat.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/macro.h"
#include "rocket/format/Subformat.h"
#include "rocket/nio/nio.h"

#include <fmt/xchar.h>

using namespace rocket::format;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Subformat, char) {
  auto lambda = [](i32 n) -> string {
    nio::StringSink buf;
    buf.print("n is {}.{}", n, Subformat<char>([&] {
      if (n == 11) {
        return Subformat<char>::params();
      }
      if (n < 6) {
        return Subformat<char>::params(" This {} it is less than {:d}.", "means", 6);
      }
      return Subformat<char>::params(" This means it is greater {} {:d}.", "than", 5);
    }));
    return buf.str();
  };

  EXPECT_EQ(lambda(5), "n is 5. This means it is less than 6.");
  EXPECT_EQ(lambda(6), "n is 6. This means it is greater than 5.");
  EXPECT_EQ(lambda(11), "n is 11.");
}

TEST(Subformat, charWithTagged) {
  auto lambda = [](i32 n) -> string {
    nio::StringSink buf;
    buf.print("{}:{}{}", ROCKET_SRC_FILE, __LINE__, Subformat<char>([&] {
      if (n == 1) {
        auto params = Subformat<char>::params(": First case: The {0} is `@@`{1} Again, the {0} is `@@`{1} But here comes another one: `⊕`{1}", "command line", ".");
        params.tag("@@", "grep {} {} {}", "-i", "foo", "bar");
        params.tag("⊕", "{} -l", "ls");
        return params;
      }
      auto params = Subformat<char>::params(": Second {}: \\1.", "case");
      params.tag("\\1", "This {} doesn't mean much", "sentence");
      return params;
    }));
    return buf.str();
  };

  EXPECT_THAT(lambda(1), matchesRegex(".*\\.cc:\\d+: First case: The command line is `grep -i foo bar`\\. Again, the command line is `grep -i foo bar`\\. But here comes another one: `ls -l`\\."));
  EXPECT_THAT(lambda(2), matchesRegex(".*\\.cc:\\d+: Second case: This sentence doesn't mean much\\."));
}

TEST(Subformat, char32) {
  auto str = fmt::format(U"a {} d", Subformat<char32>([&] {
    return Subformat<char32>::params(fmt::format(U"{} {}", U'b', U'c'));
  }));
  EXPECT_EQ(str, U"a b c d");
}

// EOF
