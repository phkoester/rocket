/*
 * test-nio-utils.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/io/IoError.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/nio/nio.h"

using namespace rocket::nio;

namespace {

// Local types ----------------------------------------------------------------------------------------------

/// A #rocket::nio::StreamSource that cannot seek or report its position, like one over a pipe.
struct NonseekableSource {
  io::testing::NonseekableBuf buf;
  istream is;
  StreamSource in;

  explicit NonseekableSource(const string& str) : buf(str), is(&buf), in(is) {}
};

/// A stream buffer that reports its position, but cannot seek anywhere else.
struct TellOnlyBuf : stringbuf {
  explicit TellOnlyBuf(const string& str) : stringbuf(str, ios::in) {}

protected:

  pos_type
  seekoff(off_type off, ios::seekdir dir, ios::openmode which) override {
    if (off == 0 && dir == ios::cur) {
      return stringbuf::seekoff(off, dir, which);
    }
    return pos_type(off_type(-1));
  }

  pos_type seekpos(pos_type, ios::openmode) override { return pos_type(off_type(-1)); }
};

/// A #rocket::nio::StreamSource over a #rocket::io::testing::FailingBuf. The stream sets `badbit` when the
/// buffer fails.
struct FailingSource {
  io::testing::FailingBuf buf;
  istream is;
  StreamSource in;

  explicit FailingSource(const string& str) : buf(str), is(&buf), in(is) {}
};

// Local functions ------------------------------------------------------------------------------------------

/// The message of the #rocket::io::IoError thrown on an I/O error.
constexpr string_view CANNOT_READ = "Cannot read from the source";

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

/// Tests `readChoice`. On failure, the source must not be advanced, so @p expectedTell defaults to 0.
void
testReadChoice(
  string_view input,
  const set<string_view>& values,
  bool ignoreCase,
  bool endOfWord,
  optional<string_view> expected,
  u64 expectedTell = 0) {
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
  io::testing::NonseekableBuf buf("ab");
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
  testReadChoice("trueX", { "true" }, false, true, nullopt);
  testReadChoice("trueXy", { "true" }, false, true, nullopt);
  testReadChoice("true1", { "true" }, false, true, nullopt);
  testReadChoice("true_", { "true" }, false, true, nullopt);
  testReadChoice("true$", { "true" }, false, true, nullopt);
  testReadChoice("trueä", { "true" }, false, true, nullopt);
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
  testReadChoice("abc!", { "ab", "abcd" }, false, true, nullopt);
  testReadChoice("abc!", { "ab", "abcd" }, false, false, "ab", 2);
}

TEST(nioUtils, readChoiceIgnoreCase) {
  testReadChoice("TRUE a", { "true" }, true, true, "true", 4);
  testReadChoice("TRUE a", { "true" }, false, true, nullopt);
  testReadChoice("TRUEX", { "true" }, true, true, nullopt);
}

TEST(nioUtils, readChoiceNoMatch) {
  testReadChoice("", { "true" }, false, true, nullopt);
  testReadChoice("tru", { "true" }, false, true, nullopt);
  testReadChoice("false", { "true" }, false, true, nullopt);
}

TEST(nioUtils, readIdentifier) {
  const auto test = [](string_view input, optional<string> expected, u64 expectedTell) {
    forEachSource(input, [&](Source& in) {
      EXPECT_EQ(readIdentifier(in), expected);
      EXPECT_EQ(in.tell(), expectedTell);
    });
  };

  test("abc", "abc", 3);
  test("abc def", "abc", 3);
  test("_a1$ x", "_a1$", 4);
  test("$", "$", 1);
  test("ärger!", "ärger", 6);
  test("abc€x", "abc", 3); // `€` is not part of an identifier
  test("abc\xFF", "abc", 3); // Invalid UTF-8 ends the identifier
}

TEST(nioUtils, readIdentifierNoIdentifier) {
  // On failure, the source is not advanced
  for (const string_view input : { ""sv, "1ab"sv, " ab"sv, "€"sv, "\xFF"sv }) {
    forEachSource(input, [](Source& in) {
      EXPECT_EQ(readIdentifier(in), nullopt);
      EXPECT_EQ(in.tell(), 0);
    });
  }
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
  io::testing::NonseekableBuf buf("123ab");
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

// I/O errors ...............................................................................................

TEST(nioUtils, ioErrors) {
  // Each function reads the given input, and then the device fails
  const auto isDigit = [](char c) { return c >= '0' && c <= '9'; };
  const vector<pair<string, function<void(Source&)>>> cases {
    { "", [](Source& in) { expectChar(in, 'a'); } },
    { "", [](Source& in) { expectColon(in); } },
    { "", [](Source& in) { expectComma(in); } },
    { "", [](Source& in) { (void) readChar(in, 'a'); } },
    { "tr", [](Source& in) { (void) readChoice(in, { "true" }); } },
    { "ab", [](Source& in) { (void) readIdentifier(in); } },
    { "ab", [](Source& in) { (void) readString(in, "abc"); } },
    { ".12", [](Source& in) { (void) readSubseconds(in); } },
    { "ab", [](Source& in) { (void) readUntilChar(in, ','); } },
    { "ab", [](Source& in) { (void) readUntilUnescapedChar(in, ','); } },
    { "12", [&](Source& in) { (void) readWhilePredicate(in, isDigit); } },
    { "  ", [](Source& in) { skip(in, true, true); } },
    { "ab", [](Source& in) { (void) skipUntilString(in, "*/"); } },
  };
  for (u64 i = 0; i < cases.size(); ++i) {
    SCOPED_TRACE(fmt::format("case {}", i));
    FailingSource src(cases[i].first);
    EXPECT_THAT([&] { cases[i].second(src.in); }, ThrowsMessage<io::IoError>(EndsWith(CANNOT_READ)));
    EXPECT_TRUE(src.in.bad());
  }
}

TEST(nioUtils, readCharacterIoError) {
  // The first code point is read, then the device fails while checking whether the character goes on
  FailingSource src("a");
  EXPECT_THAT([&] { (void) readCharacter(src.in); }, ThrowsMessage<io::IoError>(EndsWith(CANNOT_READ)));
  EXPECT_TRUE(src.in.bad());
}

TEST(nioUtils, readCodePointIoError) {
  {
    // The device fails before the first byte
    FailingSource src("");
    EXPECT_THAT([&] { (void) readCodePoint(src.in); }, ThrowsMessage<io::IoError>(EndsWith(CANNOT_READ)));
    EXPECT_TRUE(src.in.bad());
  }
  {
    // The device fails in the middle of a sequence of several bytes
    FailingSource src("\xC3");
    EXPECT_THAT([&] { (void) readCodePoint(src.in); }, ThrowsMessage<io::IoError>(EndsWith(CANNOT_READ)));
    EXPECT_TRUE(src.in.bad());
  }
}

// Failing seeks ............................................................................................

TEST(nioUtils, goingBackFailsIfSeekFails) {
  // The source reports its position, so going back is attempted, but the seek fails. This must not pass as a
  // failure that left the source where it was
  TellOnlyBuf buf("abx");
  istream is(&buf);
  StreamSource in(is);
  ASSERT_EQ(in.tell(), 0);

  EXPECT_THAT(
    [&] { (void) readString(in, "abc", false, false); },
    ThrowsMessage<InvalidState>(EndsWith("Cannot go back to position 0 in the source")));
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

TEST(nioUtils, readCharacterNonseekable) {
  {
    // An ASCII character after the character is checked by peeking
    NonseekableSource src("a\u0301 x");
    const auto result = readCharacter(src.in);
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<string>(*result), "a\u0301");
    EXPECT_EQ(src.in.readString(), " x");
  }
  {
    // A non-ASCII code point after the character must be gone back over
    NonseekableSource src("aé");
    EXPECT_THAT(
      [&] { (void) readCharacter(src.in); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  }
}

TEST(nioUtils, readChoiceNonseekableMatch) {
  // A byte is only consumed if it continues the match, so the source stops right after it
  NonseekableSource src("true!x");
  EXPECT_EQ(readChoice(src.in, { "true" }, false, false), "true");
  EXPECT_EQ(src.in.readString(), "!x");
}

TEST(nioUtils, readChoiceNonseekableNoMatch) {
  {
    // The first byte does not match, so nothing is consumed, and nothing must be gone back over
    NonseekableSource src("xyz");
    EXPECT_EQ(readChoice(src.in, { "true" }, false, false), nullopt);
    EXPECT_EQ(src.in.readString(), "xyz");
  }
  {
    // A partial match must be gone back over
    NonseekableSource src("trux");
    EXPECT_THAT(
      [&] { (void) readChoice(src.in, { "true" }, false, false); },
      ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
    EXPECT_EQ(src.in.readString(), "x");
  }
}

TEST(nioUtils, readChoiceNonseekableEndOfWord) {
  {
    // An ASCII character after the match decides the word boundary by peeking
    NonseekableSource src("true x");
    EXPECT_EQ(readChoice(src.in, { "true" }), "true");
    EXPECT_EQ(src.in.readString(), " x");
  }
  {
    // Without a word boundary, the match must be gone back over
    NonseekableSource src("truex");
    EXPECT_THAT(
      [&] { (void) readChoice(src.in, { "true" }); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
    EXPECT_EQ(src.in.readString(), "x");
  }
  {
    // A non-ASCII character must be read as a whole code point, and then gone back over
    NonseekableSource src("true€");
    EXPECT_THAT(
      [&] { (void) readChoice(src.in, { "true" }); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  }
}

TEST(nioUtils, readChoiceNonseekableLongerCandidateFails) {
  // After "a" matched, "abc" failed at "x": the source would have to go back over "b"
  for (const bool endOfWord : { false, true }) {
    NonseekableSource src("abx");
    EXPECT_THAT(
      [&] { (void) readChoice(src.in, { "a", "abc" }, false, endOfWord); },
      ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
    EXPECT_EQ(src.in.readString(), "x");
  }
}

TEST(nioUtils, readCodePointNonseekable) {
  {
    NonseekableSource src("\xC3\xA4\x80" "ab");

    const auto first = readCodePoint(src.in);
    ASSERT_TRUE(first);
    EXPECT_EQ(static_cast<char32>(*first), U'ä');

    // A byte that is neither a single byte nor a lead byte is detected by peeking, so nothing is consumed.
    // The position of the failure is unknown
    EXPECT_THAT(
      [&] { (void) readCodePoint(src.in); }, throwsInputFailure(nullopt, EndsWith("Invalid UTF-8")));
    EXPECT_EQ(src.in.readString(), "\x80" "ab");
  }
  {
    // A malformed sequence of several bytes must be gone back over
    NonseekableSource src("\xC3" "Ab");
    EXPECT_THAT([&] { (void) readCodePoint(src.in); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
    EXPECT_EQ(src.in.readString(), "b");
  }
}

TEST(nioUtils, readIdentifierNonseekable) {
  {
    // An ASCII character after the identifier is detected by peeking
    NonseekableSource src("abc def");
    EXPECT_EQ(readIdentifier(src.in), "abc");
    EXPECT_EQ(src.in.readString(), " def");
  }
  {
    // A non-ASCII code point can continue the identifier
    NonseekableSource src("abä x");
    EXPECT_EQ(readIdentifier(src.in), "abä");
    EXPECT_EQ(src.in.readString(), " x");
  }
  {
    // A non-ASCII code point after the identifier must be gone back over
    NonseekableSource src("abc€x");
    EXPECT_THAT(
      [&] { (void) readIdentifier(src.in); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  }
  {
    // An invalid byte after the identifier is detected by peeking, so nothing must be gone back over
    NonseekableSource src("abc\x80x");
    EXPECT_EQ(readIdentifier(src.in), "abc");
    EXPECT_EQ(src.in.readString(), "\x80x");
  }
}

TEST(nioUtils, readIdentifierNonseekableNoIdentifier) {
  {
    // An ASCII character is checked by peeking, so nothing is consumed
    NonseekableSource src("1ab");
    EXPECT_EQ(readIdentifier(src.in), nullopt);
    EXPECT_EQ(src.in.readString(), "1ab");
  }
  {
    // A non-ASCII code point must be read as a whole, and then gone back over
    NonseekableSource src("€ab");
    EXPECT_THAT(
      [&] { (void) readIdentifier(src.in); }, ThrowsMessage<InvalidState>(EndsWith(CANNOT_GO_BACK)));
  }
  {
    // An invalid byte is detected by peeking, so nothing is consumed
    NonseekableSource src("\x80" "ab");
    EXPECT_EQ(readIdentifier(src.in), nullopt);
    EXPECT_EQ(src.in.readString(), "\x80" "ab");
  }
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
  // An ASCII character after the match decides the word boundary by peeking
  NonseekableSource src("ab cd");
  EXPECT_TRUE(readString(src.in, "ab", false, true));
  EXPECT_EQ(src.in.readString(), " cd");
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
