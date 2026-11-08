/*
 * test-nio-utils.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/nio/nio.h"

using namespace rocket::nio;

namespace {

// Local functions ------------------------------------------------------------------------------------------

void
expectReadUntilUnescapedChar(Source& in, char c, const optional<string>& expected, u64 expectedTell) {
  EXPECT_EQ(readUntilUnescapedChar(in, c), expected);
  EXPECT_EQ(in.tell(), expectedTell);
}

/// Checks a contiguous source and noncontiguous sources, which are handled by separate code paths.
void
testReadUntilUnescapedChar(string_view input, char c, const optional<string>& expected, u64 expectedTell) {
  SCOPED_TRACE(string(input));

  {
    SCOPED_TRACE("StringSource");
    StringSource in(input);
    expectReadUntilUnescapedChar(in, c, expected, expectedTell);
  }

  {
    SCOPED_TRACE("BufferedSource");
    StringSource underlying(input);
    BufferedSource in(underlying, 64);
    expectReadUntilUnescapedChar(in, c, expected, expectedTell);
  }

  {
    SCOPED_TRACE("FileSource");
    const auto temp = rocket::filesystem::tempFile();
    {
      FileSink out(temp);
      out.Sink::write(input);
    }
    FileSource in(temp);
    expectReadUntilUnescapedChar(in, c, expected, expectedTell);
  }
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(nioUtils, readUntilUnescapedCharNoBackslash) {
  testReadUntilUnescapedChar(R"(abc"def)", '"', R"(abc)", 4);
  testReadUntilUnescapedChar(R"("def)", '"', "", 1);
}

TEST(nioUtils, readUntilUnescapedCharOddBackslashes) {
  // An odd number of backslashes escapes the character, so reading continues
  testReadUntilUnescapedChar(R"(a\"b"c)", '"', R"(a\"b)", 5);
  testReadUntilUnescapedChar(R"(a\\\"b"c)", '"', R"(a\\\"b)", 7);
  testReadUntilUnescapedChar(R"(\"b"c)", '"', R"(\"b)", 4);
}

TEST(nioUtils, readUntilUnescapedCharEvenBackslashes) {
  // An even number of backslashes consists of escaped backslashes only, so the character is unescaped
  testReadUntilUnescapedChar(R"(a\\"b)", '"', R"(a\\)", 4);
  testReadUntilUnescapedChar(R"(a\\\\"b)", '"', R"(a\\\\)", 6);
  testReadUntilUnescapedChar(R"(\\"b)", '"', R"(\\)", 3);
}

TEST(nioUtils, readUntilUnescapedCharBackslashNotAdjacent) {
  testReadUntilUnescapedChar(R"(a\nb"c)", '"', R"(a\nb)", 5);
}

TEST(nioUtils, readUntilUnescapedCharOtherChar) {
  testReadUntilUnescapedChar(R"(a\\'b)", '\'', R"(a\\)", 4);
  testReadUntilUnescapedChar(R"(a\'b'c)", '\'', R"(a\'b)", 5);
}

TEST(nioUtils, readUntilUnescapedCharNotFound) {
  // The source must not be advanced on failure
  testReadUntilUnescapedChar("", '"', nullopt, 0);
  testReadUntilUnescapedChar(R"(abc)", '"', nullopt, 0);
  testReadUntilUnescapedChar(R"(a\")", '"', nullopt, 0);
  testReadUntilUnescapedChar(R"(a\\\")", '"', nullopt, 0);
}

// EOF
