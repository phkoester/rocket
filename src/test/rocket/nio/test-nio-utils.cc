/*
 * test-nio-utils.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/nio/nio.h"

#include <sstream>

using namespace rocket::nio;

namespace {

// `NonseekableBuf` -----------------------------------------------------------------------------------------

/// A stream buffer that cannot seek, like one over a pipe.
struct NonseekableBuf : stringbuf {
  explicit NonseekableBuf(const string& str) : stringbuf(str, ios::in) {}

protected:

  pos_type seekoff(off_type, ios::seekdir, ios::openmode) override { return pos_type(off_type(-1)); }

  pos_type seekpos(pos_type, ios::openmode) override { return pos_type(off_type(-1)); }
};

// Local functions ------------------------------------------------------------------------------------------

/// Calls @p fn with a contiguous source and noncontiguous sources, which are handled by separate code paths.
template<typename Fn>
void
forEachSource(string_view input, Fn fn) {
  SCOPED_TRACE(string(input));

  {
    SCOPED_TRACE("StringSource");
    StringSource in(input);
    fn(in);
  }

  {
    SCOPED_TRACE("FileSource");
    const auto temp = rocket::filesystem::tempFile();
    {
      FileSink out(temp);
      out.Sink::write(input);
    }
    FileSource in(temp);
    fn(in);
  }
}

void
testReadChoice(
  string_view input,
  const set<string_view>& values,
  bool ignoreCase,
  bool endOfWord,
  optional<string_view> expected,
  u64 expectedTell) {
  forEachSource(input, [&](Source& in) {
    EXPECT_EQ(readChoice(in, values, ignoreCase, endOfWord), expected);
    EXPECT_EQ(in.tell(), expectedTell);
  });
}

void
testReadUntilUnescapedChar(string_view input, char c, const optional<string>& expected, u64 expectedTell) {
  forEachSource(input, [&](Source& in) {
    EXPECT_EQ(readUntilUnescapedChar(in, c), expected);
    EXPECT_EQ(in.tell(), expectedTell);
  });
}

void
testSkipUntilString(string_view input, string_view str, bool expected, u64 expectedTell) {
  forEachSource(input, [&](Source& in) {
    EXPECT_EQ(skipUntilString(in, str), expected);
    EXPECT_EQ(in.tell(), expectedTell);
  });
}

void
testSkipCComments(string_view input, u64 expectedTell) {
  forEachSource(input, [&](Source& in) {
    skip(in, true, false);
    EXPECT_EQ(in.tell(), expectedTell);
  });
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(nioUtils, readChar) {
  forEachSource("ab", [](Source& in) {
    EXPECT_FALSE(readChar(in, 'x'));
    EXPECT_EQ(in.tell(), 0);
    EXPECT_TRUE(readChar(in, 'a'));
    EXPECT_EQ(in.tell(), 1);
    EXPECT_FALSE(readChar(in, 'a'));
    EXPECT_EQ(in.tell(), 1);
    EXPECT_TRUE(readChar(in, 'b'));
    EXPECT_EQ(in.tell(), 2);
    EXPECT_FALSE(readChar(in, 'b')); // EOF
  });
}

TEST(nioUtils, readCharNonseekable) {
  NonseekableBuf buf("ab");
  istream is(&buf);
  StreamSource in(is);
  ASSERT_EQ(in.tell(), NPOS);

  EXPECT_FALSE(readChar(in, 'x'));
  EXPECT_TRUE(readChar(in, 'a'));
  EXPECT_FALSE(readChar(in, 'a'));
  EXPECT_TRUE(readChar(in, 'b'));
  EXPECT_FALSE(readChar(in, 'b')); // EOF
}

TEST(nioUtils, readChoiceWordBoundary) {
  // A word boundary follows
  testReadChoice("true", { "true" }, false, true, "true", 4);
  testReadChoice("true a", { "true" }, false, true, "true", 4);
  testReadChoice("true,a", { "true" }, false, true, "true", 4);

  // No word boundary follows
  testReadChoice("trueX", { "true" }, false, true, nullopt, 0);
  testReadChoice("trueXy", { "true" }, false, true, nullopt, 0);
  testReadChoice("true1", { "true" }, false, true, nullopt, 0);
  testReadChoice("true_", { "true" }, false, true, nullopt, 0);
  testReadChoice("true$", { "true" }, false, true, nullopt, 0);
  testReadChoice("trueä", { "true" }, false, true, nullopt, 0);
}

TEST(nioUtils, readChoiceWordBoundaryInvalidUtf8) {
  // An undecodable code point counts as a word boundary
  testReadChoice("true\xFF", { "true" }, false, true, "true", 4);
  testReadChoice("true\xC3", { "true" }, false, true, "true", 4);
}

TEST(nioUtils, readChoiceNoWordBoundaryRequired) {
  testReadChoice("trueX", { "true" }, false, false, "true", 4);
}

TEST(nioUtils, readChoiceLongestMatch) {
  testReadChoice("abcd e", { "ab", "abcd" }, false, true, "abcd", 4);
  testReadChoice("ab!", { "ab", "abcd" }, false, true, "ab", 2);

  // The longest match, "ab", is followed by "c", so there is no word boundary
  testReadChoice("abc!", { "ab", "abcd" }, false, true, nullopt, 0);
  testReadChoice("abc!", { "ab", "abcd" }, false, false, "ab", 2);
}

TEST(nioUtils, readChoiceIgnoreCase) {
  testReadChoice("TRUE a", { "true" }, true, true, "true", 4);
  testReadChoice("TRUE a", { "true" }, false, true, nullopt, 0);
  testReadChoice("TRUEX", { "true" }, true, true, nullopt, 0);
}

TEST(nioUtils, readChoiceNoMatch) {
  testReadChoice("", { "true" }, false, true, nullopt, 0);
  testReadChoice("tru", { "true" }, false, true, nullopt, 0);
  testReadChoice("false", { "true" }, false, true, nullopt, 0);
}

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

TEST(nioUtils, readWhilePredicate) {
  const auto isDigit = [](char c) { return c >= '0' && c <= '9'; };
  forEachSource("123ab", [&](Source& in) {
    EXPECT_EQ(readWhilePredicate(in, isDigit), "123");
    EXPECT_EQ(in.tell(), 3);
    EXPECT_EQ(readWhilePredicate(in, isDigit), "");
    EXPECT_EQ(in.tell(), 3);
  });
  forEachSource("123", [&](Source& in) {
    EXPECT_EQ(readWhilePredicate(in, isDigit), "123"); // Up to EOF
    EXPECT_EQ(in.tell(), 3);
  });
}

TEST(nioUtils, readWhilePredicateNonseekable) {
  const auto isDigit = [](char c) { return c >= '0' && c <= '9'; };
  NonseekableBuf buf("123ab");
  istream is(&buf);
  StreamSource in(is);
  ASSERT_EQ(in.tell(), NPOS);

  EXPECT_EQ(readWhilePredicate(in, isDigit), "123");
  char c = '\0';
  EXPECT_EQ(in.Source::read(c), 1);
  EXPECT_EQ(c, 'a'); // The first nondigit was not consumed
}

TEST(nioUtils, skipUntilStringFound) {
  testSkipUntilString("*/x", "*/", true, 2);
  testSkipUntilString("a*/x", "*/", true, 3);
  testSkipUntilString("*/*/", "*/", true, 2); // Stops after the first occurrence
  testSkipUntilString("a\nb", "\n", true, 2);
}

TEST(nioUtils, skipUntilStringPartialMatches) {
  // The character that ends a partial match may start the actual match
  testSkipUntilString("**/x", "*/", true, 3);
  testSkipUntilString("***/x", "*/", true, 4);
  testSkipUntilString("*x*/y", "*/", true, 4);

  // The tail of a partial match may be the start of the actual match
  testSkipUntilString("aaab", "aab", true, 4);
  testSkipUntilString("abababc", "ababc", true, 7);
}

TEST(nioUtils, skipUntilStringNotFound) {
  // The source is advanced to its end
  testSkipUntilString("", "*/", false, 0);
  testSkipUntilString("**", "*/", false, 2);
  testSkipUntilString("abc", "*/", false, 3);
}

TEST(nioUtils, skipCCommentsEndingInAsterisks) {
  testSkipCComments("/***/x", 5);
  testSkipCComments("/****/x", 6);
  testSkipCComments("/** a **/x", 9);
  testSkipCComments("  /** a **/  x", 13);
  testSkipCComments("/** a **/", 9);
}

TEST(nioUtils, skipCCommentsUnterminated) {
  forEachSource("/** a *", [&](Source& in) {
    EXPECT_THAT(
      [&] { skip(in, true, false); },
      throwsInputFailure(0, EndsWith("Unterminated C-style comment")));
    EXPECT_EQ(in.tell(), 0); // The source must not be advanced on failure
  });
}

// EOF
