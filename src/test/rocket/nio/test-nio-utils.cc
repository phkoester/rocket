/*
 * test-nio-utils.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/nio/nio.h"
#include "rocket/unicode/unicode.h"

using namespace rocket::nio;

namespace {

// Local types ----------------------------------------------------------------------------------------------

/// A #rocket::nio::StreamSource that cannot seek or report its position, like one over a pipe.
struct NonseekableSource {
  NonseekableBuf buf;
  istream is;
  StreamSource in;

  explicit NonseekableSource(const string& str) : buf(str), is(&buf), in(is) {}
};

// Local functions ------------------------------------------------------------------------------------------

/// The message of the #rocket::InvalidState thrown when a source that cannot seek would have to go back.
constexpr string_view CANNOT_GO_BACK = "Cannot go back in a source that cannot report its position";

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
  ASSERT_EQ(in.tell(), nullopt);

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
  ASSERT_EQ(in.tell(), nullopt);

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

// Nonseekable sources ......................................................................................

// A source that cannot seek cannot go back either. Functions that must go back throw `InvalidState`, and the
// input read up to that point is lost. These tests pin down that behavior.

TEST(nioUtils, expectCharNonseekable) {
  NonseekableSource src("ab");
  ASSERT_EQ(src.in.tell(), nullopt);

  EXPECT_NO_THROW(expectChar(src.in, 'a'));
  // The position of the failure is unknown
  EXPECT_THAT([&] { expectChar(src.in, 'x'); }, throwsInputFailure(nullopt, EndsWith("Expected 'x'")));
  EXPECT_EQ(src.in.readString(), "b"); // `expectChar` only peeks, so nothing is lost
}

TEST(nioUtils, readChoiceNonseekableMatch) {
  // Even a match throws: the source has read past it and would have to go back
  NonseekableSource src("true!x");
  EXPECT_THAT(
    [&] { (void) readChoice(src.in, { "true" }, false, false); },
    ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), "x");
}

TEST(nioUtils, readChoiceNonseekableNoMatch) {
  NonseekableSource src("xyz");
  EXPECT_THAT(
    [&] { (void) readChoice(src.in, { "true" }, false, false); },
    ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), "yz");
}

TEST(nioUtils, readCodePointNonseekable) {
  NonseekableSource src("\xC3\xA4\x80" "ab");

  const auto first = readCodePoint(src.in);
  ASSERT_TRUE(first);
  EXPECT_EQ(static_cast<char32>(*first), U'ä');

  // Invalid UTF-8 does not throw, but the invalid byte is lost instead of being rewound
  const auto second = readCodePoint(src.in);
  ASSERT_FALSE(second);
  EXPECT_EQ(second.error(), InvalidUtf8);
  EXPECT_EQ(src.in.readString(), "ab");
}

TEST(nioUtils, readIdentifierNonseekable) {
  // The identifier is read, but going back over the code point after it throws
  NonseekableSource src("abc def");
  EXPECT_THAT(
    [&] { (void) readIdentifier(src.in); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), "def");
}

TEST(nioUtils, readIdentifierNonseekableNoIdentifier) {
  NonseekableSource src("1ab");
  EXPECT_THAT(
    [&] { (void) readIdentifier(src.in); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), "ab");
}

TEST(nioUtils, readStringNonseekable) {
  // A match without a word-boundary check does not need to go back
  NonseekableSource src("abc");
  EXPECT_TRUE(readString(src.in, "ab", false, false));
  EXPECT_EQ(src.in.readString(), "c");
}

TEST(nioUtils, readStringNonseekableMismatch) {
  NonseekableSource src("abxy");
  EXPECT_THAT(
    [&] { (void) readString(src.in, "abc", false, false); },
    ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), "y");
}

TEST(nioUtils, readStringNonseekableEndOfWord) {
  // Checking the word boundary reads the next code point and always goes back
  NonseekableSource src("ab cd");
  EXPECT_THAT(
    [&] { (void) readString(src.in, "ab", false, true); },
    ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), "cd");
}

TEST(nioUtils, readUntilCharNonseekable) {
  NonseekableSource src("ab,c");
  EXPECT_EQ(readUntilChar(src.in, ','), "ab");
  EXPECT_EQ(src.in.readString(), "c");
}

TEST(nioUtils, readUntilCharNonseekableNotFound) {
  NonseekableSource src("abc");
  EXPECT_THAT(
    [&] { (void) readUntilChar(src.in, ','); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), "");
}

TEST(nioUtils, readUntilUnescapedCharNonseekableNotFound) {
  NonseekableSource src(R"(a\,b)");
  EXPECT_THAT(
    [&] { (void) readUntilUnescapedChar(src.in, ','); },
    ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), "");
}

TEST(nioUtils, skipNonseekable) {
  // Skipping stops at the first nonwhitespace character, which it has to go back over
  NonseekableSource src("  xy");
  EXPECT_THAT([&] { skip(src.in, false, false); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), "y");
}

TEST(nioUtils, skipNonseekableCComments) {
  // A `/` that does not start a comment is read together with the character after it
  NonseekableSource src("/x y");
  EXPECT_THAT([&] { skip(src.in, true, false); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  EXPECT_EQ(src.in.readString(), " y");
}

TEST(nioUtils, skipNonseekableToEof) {
  // Whitespace and comments up to the end of the input need not go back
  NonseekableSource src("  // comment\n  ");
  EXPECT_NO_THROW(skip(src.in, true, false));
  EXPECT_EQ(src.in.readString(), "");
}

TEST(nioUtils, skipUntilStringNonseekable) {
  NonseekableSource src("a*/b");
  EXPECT_TRUE(skipUntilString(src.in, "*/"));
  EXPECT_EQ(src.in.readString(), "b");
}

// EOF
