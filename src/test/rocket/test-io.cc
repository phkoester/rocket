/*
 * test-io.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/io.h"

#include "rocket-gtest/matcher.h"

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace rocket::io;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

// `Buffer` .................................................................................................

TEST(io, Buffer_getCodePoint) {
  string s = "ä€";
  auto is = io::is(s);
  Buffer buf(is);
  unicode::CodePoint cp;
  // ä
  auto bytes = buf.getCodePoint(&cp);
  EXPECT_EQ(*bytes, (vector<byte> { byte(0xc3), byte(0xa4) }));
  EXPECT_EQ(cp, 0xe4U);
  // €
  bytes = buf.getCodePoint(&cp);
  EXPECT_EQ(*bytes, (vector<byte> { byte(0xe2), byte(0x82), byte(0xac) }));
  EXPECT_EQ(cp, 0x20acU);
  // EOF
  bytes = buf.getCodePoint(&cp);
  EXPECT_FALSE(bytes);
}

TEST(io, Buffer_getGrapheme) {
  //  ä:  2 bytes, 1 code point
  //  €:  3 bytes, 1 code point
  // ☢️:  6 bytes, 2 code points
  // 🧑‍🌾: 11 bytes, 3 code points

  string s = "ä€☢️🧑‍🌾";
  auto is = io::is(s);
  Buffer buf(is);
  unicode::Grapheme gr;
  // ä
  auto bytes = buf.getGrapheme(&gr);
  EXPECT_EQ(bytes->size(), 2);
  EXPECT_EQ(gr.codePoints.size(), 1);
  EXPECT_EQ(static_cast<string>(gr), "ä");
  // €
  bytes = buf.getGrapheme(&gr);
  EXPECT_EQ(bytes->size(), 3);
  EXPECT_EQ(gr.codePoints.size(), 1);
  EXPECT_EQ(static_cast<string>(gr), "€");
  // ☢️
  bytes = buf.getGrapheme(&gr);
  EXPECT_EQ(bytes->size(), 6);
  EXPECT_EQ(gr.codePoints.size(), 2);
  EXPECT_EQ(static_cast<string>(gr), "☢️");
  // 🧑‍🌾
  bytes = buf.getGrapheme(&gr);
  EXPECT_EQ(bytes->size(), 11);
  EXPECT_EQ(gr.codePoints.size(), 3);
  EXPECT_EQ(static_cast<string>(gr), "🧑‍🌾");
  // EOF
  bytes = buf.getGrapheme(&gr);
  EXPECT_FALSE(bytes);
}

TEST(io, Buffer_put) {
  auto is = io::is("abc");
  Buffer buf(is);
  vector<byte> bytes;
  optional<byte> got;
  EXPECT_EQ(got = buf.get(), byte('a')); bytes.push_back(*got);
  EXPECT_EQ(got = buf.get(), byte('b')); bytes.push_back(*got);
  EXPECT_EQ(got = buf.get(), byte('c')); bytes.push_back(*got);
  EXPECT_EQ(buf.position(), 3);
  buf.put(bytes);
  EXPECT_EQ(buf.position(), 0);
  EXPECT_EQ(got = buf.get(), byte('a'));
  EXPECT_EQ(got = buf.get(), byte('b'));
  EXPECT_EQ(got = buf.get(), byte('c'));
  EXPECT_EQ(buf.position(), 3);
}

// Functions ................................................................................................

TEST(io, fd) {
  EXPECT_EQ(fd(cin), STDIN_FILENO);
  EXPECT_EQ(fd(cout), STDOUT_FILENO);
  EXPECT_EQ(fd(cerr), STDERR_FILENO);

  EXPECT_EQ(fd(istringstream()), -1);
  EXPECT_EQ(fd(ostringstream()), -1);
}

TEST(io, getChar) {
  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { getChar(io::resetg(is), 'x'); },
        throwsParseFailure(0, HasSubstr("EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("y");
    EXPECT_THAT(
        [&] { getChar(io::resetg(is), { 'x', 'z' }); },
        throwsParseFailure(0, HasSubstr("Expected any of {'x', 'z'}, got 'y'")));
    EXPECT_ISTREAM(is, true, false, 1);
  }

  {
    auto is = io::is("x");
    EXPECT_EQ(getChar(is, { 'x', 'y' }), 'x');
    EXPECT_ISTREAM(is, false, false, 1);
  }
}

TEST(io, getOptionalChar) {
  {
    auto is = io::is();
    EXPECT_EQ(getOptionalChar(is, set<char> { 'x' }), nullopt);
    EXPECT_ISTREAM(is, false, false, 0);
  }

  {
    auto is = io::is("a");
    EXPECT_EQ(getOptionalChar(is, set<char> { 'b' }), nullopt);
    EXPECT_ISTREAM(is, false, false, 0);
  }

  {
    auto is = io::is("a");
    EXPECT_EQ(getOptionalChar(is, { 'a', 'b' }), 'a');
    EXPECT_ISTREAM(is, false, false, 1);
  }
}

TEST(io, getString) {
  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { getString(io::resetg(is), set<string_view> { "a", "b", "c" }); },
        throwsParseFailure(0, { 0, 0 }, HasSubstr("\"\" does not match any of {\"a\", \"b\", \"c\"}, got EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("ab");
    EXPECT_THAT(
        [&] { getString(io::resetg(is), set<string_view> { "abc", "def" }); },
        throwsParseFailure(2, { 0, 2 }, HasSubstr("\"ab\" does not match any of {\"abc\", \"def\"}, got EOF")));
    EXPECT_ISTREAM(is, true, true, 2);
  }

  {
    auto is = io::is("def");
    EXPECT_EQ(getString(is, set<string_view> { "abc", "def" }), "def");
    EXPECT_ISTREAM(is, false, false, 3);
  }

  {
    auto is = io::is("defx");
    EXPECT_EQ(getString(is, set<string_view> { "abc", "def" }), "def");
    EXPECT_ISTREAM(is, false, false, 3);
  }
}

TEST(io, getUntil) {
  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { getUntil(io::resetg(is), ';', false, 1); },
        throwsParseFailure(0, HasSubstr("Seeking ';', got EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("abcde");
    EXPECT_THAT(
        [&] { getUntil(io::resetg(is), ';', false, 1); },
        throwsParseFailure(5, HasSubstr("Seeking ';', got EOF")));
    EXPECT_ISTREAM(is, true, true, 5);
  }

  {
    auto is = io::is("abcde;");
    auto s = getUntil(is, ';', false, 1);
    EXPECT_ISTREAM(is, false, false, 5);
    EXPECT_EQ(s, "abcde");
    char c = getChar(is);
    EXPECT_EQ(c, ';');
    EXPECT_ISTREAM(is, false, false, 6);
  }

  {
    auto is = io::is("abcde;");
    auto s = getUntil(is, ';', false, 1);
    EXPECT_ISTREAM(is, false, false, 5);
    EXPECT_EQ(s, "abcde");
    char c = getChar(is);
    EXPECT_EQ(c, ';');
    EXPECT_ISTREAM(is, false, false, 6);
  }

  {
    auto is = io::is("abc|");
    EXPECT_THAT(
        [&] { getUntil(io::resetg(io::resetg(is)), '|', true, 4); },
        throwsParseFailure(3, { 0, 4 }, HasSubstr("Expected at least 4 characters before '|', got 3")));
    EXPECT_ISTREAM(is, true, false, 4);
  }

  {
    auto is = io::is("abc|");
    EXPECT_THAT(
        [&] { getUntil(io::resetg(io::resetg(is)), [](char c) { return c == '|'; }, "pipe symbol", true, 4); },
        throwsParseFailure(3, { 0, 4 }, HasSubstr("Expected at least 4 characters before pipe symbol, got 3")));
    EXPECT_ISTREAM(is, true, false, 4);
  }
}

TEST(io, getWhile) {
  {
    auto is = io::is();
    EXPECT_EQ(getWhile(is, { 'x', 'y' }, 0), "");
    EXPECT_ISTREAM(is, false, false, 0);
  }

  {
    auto is = io::is();
    EXPECT_THAT(
        [&] { getWhile(io::resetg(io::resetg(is)), { 'x', 'y' }, 1); },
        throwsParseFailure(0, { 0, 1 }, HasSubstr("Expected at least 1 character contained in {'x', 'y'}, got 0 and EOF")));
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    auto is = io::is("y");
    EXPECT_EQ(getWhile(is, { 'x', 'y' }, 1), "y");
    EXPECT_ISTREAM(is, false, false, 1);
  }

  {
    auto is = io::is("x");
    EXPECT_THAT(
        [&] { getWhile(io::resetg(is), { 'x', 'y' }, 2); },
        throwsParseFailure(1, { 0, 2 }, HasSubstr("Expected at least 2 characters contained in {'x', 'y'}, got 1 and EOF")));
    EXPECT_ISTREAM(is, true, true, 1);
  }

  {
    auto is = io::is("yx");
    EXPECT_EQ(getWhile(is, { 'x', 'y' }, 2), "yx");
    EXPECT_ISTREAM(is, false, false, 2);
  }
}

TEST(io, isatty) {
  EXPECT_EQ(isatty(istringstream()), false);
  EXPECT_EQ(isatty(ostringstream()), false);
}

/**
 * This test requires `ROCKET_TEST_TERMINAL=1`.
 */
TEST(io, isattyTerminal) {
  EXPECT_ENV("ROCKET_TEST_TERMINAL");

  EXPECT_EQ(isatty(cout), true);
  EXPECT_EQ(isatty(cerr), true);
  EXPECT_EQ(isatty(cin), true);
}

// EOF
