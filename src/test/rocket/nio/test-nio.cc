/*
 * test-nio.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/scan.h"
#include "rocket/filesystem/filesystem.h"
#include "rocket/log/log.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/nio/nio.h"

#include <fstream>

using namespace rocket::nio;

namespace {

// Local functions ------------------------------------------------------------------------------------------

/// Returns "01234567890123...".
string
testString(u64 n, i32 add = 0) {
  string ret;
  for (u64 i = 0; i < n; ++i) {
    ret += static_cast<char>('0' + add);
    add = (add + 1) % 10;
  }
  return ret;
}

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
expectCodePoint(Source& in, char32 expected, u64 expectedTell) {
  const auto result = readCodePoint(in);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(static_cast<char32>(*result), expected);
  EXPECT_EQ(in.tell(), expectedTell);
}

void
expectCodePointError(Source& in, ReadCodePointError expected) {
  const auto result = readCodePoint(in);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), expected);
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

// `Sink` ...................................................................................................

TEST(nio, FileSinkDoesNotExist) {
  EXPECT_THAT(
    [&] { const FileSink out("/file/does/not/exist"); },
    ThrowsMessage<InvalidState>(EndsWith("Cannot open file `/file/does/not/exist` for writing")));
}

TEST(nio, NullSink) {
  NullSink out;
  EXPECT_FALSE(out.bad());
  EXPECT_TRUE(out.eof());
  EXPECT_EQ(out.Sink::write("a"), 0);
}

TEST(nio, SpanSink) {
  string str = "---[abcd]---";
  const span<char> span(&str[4], 4);
  SpanSink out(span);
  EXPECT_EQ(out.Sink::write("ABCDEF"), 4); // Writing 6 chars, but only 4 fit
  EXPECT_EQ(str, "---[ABCD]---");
}

TEST(nio, StreamSink) {
  ostringstream os;
  StreamSink out(os);
  out.println("Hi {}", "there");
  EXPECT_EQ(os.str(), "Hi there\n");
}

TEST(nio, StringSink) {
  StringSink out1; // With managed string
  out1.println("Hi {}", "there");
  EXPECT_EQ(out1.str(), "Hi there\n");

  string buf;
  StringSink out2(buf); // With external string
  out2.println("Hi {}", "there");
  EXPECT_EQ(buf, "Hi there\n");
}

// `Source` .................................................................................................

TEST(nio, FileSourceDoesNotExist) {
  EXPECT_THAT(
    [&] { const FileSource in("/file/does/not/exist"); },
    ThrowsMessage<InvalidState>(EndsWith("Cannot open file `/file/does/not/exist` for reading")));
}

TEST(nio, FileSourceReadAll) {
  const auto temp = rocket::filesystem::tempFile();

  const vector<u8> data { 0, 0, 0, 0 };

  {
    FileSink out(temp.string()); // Auto-close
    out.write(data);
  }

  FileSource in(temp.string());
  auto bytes = in.readAll();
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  EXPECT_EQ(in.tell(), 4);
  EXPECT_FALSE(in.bad());
  EXPECT_EQ(bytes, data);

  bytes = in.readAll();
  EXPECT_TRUE(bytes.empty());
}

TEST(nio, FileSourceReadString) {
  const auto temp = rocket::filesystem::tempFile();

  {
    FileSink out(temp.string()); // Auto-close
    out.writeln("Hey there");
  }

  FileSource in(temp.string());
  auto str = in.readString();
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  EXPECT_EQ(str, "Hey there\n");
  str = in.readString();
  EXPECT_EQ(str, "");
  EXPECT_FALSE(in.bad());

  in.seek(-6, SeekMode::end);
  EXPECT_EQ(in.tell(), 4);
  str = in.readString();
  EXPECT_EQ(str, "there\n");
  EXPECT_EQ(in.tell(), 10);
}

TEST(nio, FileSourceScanIstream) {
  const auto path = testSource("test-nio-FileSourceScanIstream.txt");
  FileSource in(path);
  auto& is = in.istream();
  auto result = scn::scan<log::LogLevel, log::LogLevel>(is, "{}, {}");
  ASSERT_TRUE(result);
  const auto [level1, level2] = result->values();
  EXPECT_EQ(level1, log::LogLevel::debug);
  EXPECT_EQ(level2, log::LogLevel::info);
  EXPECT_EQ(io::tell(is), 11);
}

TEST(nio, NullSource) {
  NullSource in;
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  char c; // NOLINT
  EXPECT_EQ(in.Source::read(c), 0);
  EXPECT_EQ(in.peek(), nullopt);
}

TEST(nio, SpanSource) {
  vector<u8> vec { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!' };
  SpanSource in(vec);
  const auto str = in.readString();
  EXPECT_EQ(str, "Hello, world!");
}

TEST(nio, StreamSourceReadAll) {
  auto temp = rocket::filesystem::tempFile();

  const vector<u8> data { 0, 0, 0, 0 };

  {
    ofstream os(temp.c_str(), ios::binary);
    StreamSink out(os);
    out.write(data);
  }

  ifstream is(temp.c_str());
  StreamSource in(is);
  auto bytes = in.readAll();
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  EXPECT_EQ(in.tell(), 4);
  EXPECT_EQ(bytes, data);

  bytes = in.readAll();
  EXPECT_TRUE(bytes.empty());
}

TEST(nio, StreamSourceReadString) {
  auto temp = rocket::filesystem::tempFile();

  {
    ofstream os(temp.c_str(), ios::binary);
    StreamSink out(os);
    out.writeln("Hey there");
  }

  ifstream is(temp.c_str());
  StreamSource in(is);
  auto str = in.readString();
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  EXPECT_EQ(in.tell(), 10);
  EXPECT_EQ(str, "Hey there\n");
  str = in.readString();
  EXPECT_EQ(str, "");

  in.seek(-6, SeekMode::end);
  EXPECT_EQ(in.tell(), 4);
  str = in.readString();
  EXPECT_EQ(str, "there\n");
  EXPECT_EQ(in.tell(), 10);
}

TEST(nio, StringSource) {
  StringSource in("Hello, world!");
  const auto str = in.readString();
  EXPECT_EQ(str, "Hello, world!");
}

TEST(nio, StringSourceReadln) {
  StringSource in("First line\r\nSecond line\n");
  string line = in.readln();
  EXPECT_EQ(line, "First line");
  line = in.readln();
  EXPECT_EQ(line, "Second line");
  line = in.readln();
  EXPECT_EQ(line, "");
}

TEST(nio, StringSourceSeek) {
  const string str128 = testString(128);
  StringSource in(str128);

  in.Source::seek(0);
  EXPECT_EQ(in.tell(), 0);
  char c = '\0';
  EXPECT_EQ(in.Source::read(c), 1);
  EXPECT_EQ(c, '0');
  EXPECT_EQ(in.tell(), 1);

  in.Source::seek(127);
  EXPECT_EQ(in.tell(), 127);
  EXPECT_EQ(in.Source::read(c), 1);
  EXPECT_EQ(c, '7');
  EXPECT_EQ(in.tell(), 128);

  in.Source::seek(129);
  EXPECT_EQ(in.tell(), 128);

  in.seek(-11, SeekMode::cur);
  EXPECT_EQ(in.tell(), 117);

  EXPECT_FALSE(in.Source::seek(-1)); // Before the beginning: fails, position unchanged
  EXPECT_EQ(in.tell(), 117);

  in.seek(0, SeekMode::end);
  EXPECT_EQ(in.tell(), 128);

  in.seek(-10, SeekMode::end);
  EXPECT_EQ(in.tell(), 118);

  in.seek(3, SeekMode::end);
  EXPECT_EQ(in.tell(), 128);

  EXPECT_FALSE(in.seek(-200, SeekMode::end));
  EXPECT_EQ(in.tell(), 128);
  EXPECT_FALSE(in.seek(numeric_limits<i64>::min(), SeekMode::cur));
  EXPECT_EQ(in.tell(), 128);
}

// `peek` ...................................................................................................

TEST(nio, peek) {
  forEachSource("ab", [](Source& in) {
    EXPECT_EQ(in.peek(), 'a');
    EXPECT_EQ(in.peek(), 'a'); // Peeking does not advance
    EXPECT_EQ(in.tell(), 0);

    char c = '\0';
    EXPECT_EQ(in.Source::read(c), 1);
    EXPECT_EQ(c, 'a');
    EXPECT_EQ(in.peek(), 'b');
    EXPECT_EQ(in.tell(), 1);

    EXPECT_EQ(in.Source::read(c), 1);
    EXPECT_EQ(c, 'b');
    EXPECT_EQ(in.peek(), nullopt);
    EXPECT_TRUE(in.eof());
  });
}

// `readCodePoint` ..........................................................................................

TEST(nio, readCodePointValid) {
  // One, two, three, and four UTF-8 bytes
  forEachSource("aä€\U0001F971", [](Source& in) {
    expectCodePoint(in, U'a', 1);
    expectCodePoint(in, U'ä', 3);
    expectCodePoint(in, U'€', 6);
    expectCodePoint(in, U'\U0001F971', 10);
  });
}

TEST(nio, readCodePointEof) {
  forEachSource("", [](Source& in) {
    expectCodePointError(in, ReadCodePointError::Eof);
    EXPECT_TRUE(in.eof());
  });

  forEachSource("a", [](Source& in) {
    expectCodePoint(in, U'a', 1);
    expectCodePointError(in, ReadCodePointError::Eof);
    EXPECT_TRUE(in.eof());
    expectCodePointError(in, ReadCodePointError::Eof); // Still at the end
  });
}

TEST(nio, readCodePointInvalidLeadByte) {
  for (const string_view input : { "\xFF"sv, "\x80"sv, "\xC0\x80"sv }) {
    forEachSource(input, [](Source& in) { expectCodePointError(in, ReadCodePointError::InvalidUtf8); });
  }
}

TEST(nio, readCodePointTruncatedSequence) {
  for (const string_view input : { "\xC3"sv, "\xE2\x82"sv, "\xF0\x9F\x98"sv }) {
    forEachSource(input, [](Source& in) { expectCodePointError(in, ReadCodePointError::InvalidUtf8); });
  }
}

TEST(nio, readCodePointInvalidContinuation) {
  for (const string_view input : { "\xC3" "A"sv, "\xE2\x82" "A"sv, "\xED\xA0\x80"sv }) { // Last: surrogate
    forEachSource(input, [](Source& in) { expectCodePointError(in, ReadCodePointError::InvalidUtf8); });
  }
}

// EOF
