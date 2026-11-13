/*
 * test-nio.cc
 */

#include "rocket-test/rocket-test.h"

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

/**
 * Calls @p fn with each sink that keeps its output, and expects the output to equal @p expected.
 *
 * @p fn returns `false` if any print operation failed.
 */
template<typename Fn>
void
forEachSink(string_view expected, Fn fn) {
  {
    SCOPED_TRACE("FileSink");
    const auto temp = rocket::filesystem::tempFile();
    {
      FileSink out(temp);
      EXPECT_TRUE(fn(out));
      EXPECT_FALSE(out.bad());
    }
    FileSource in(temp);
    EXPECT_EQ(in.readString(), expected);
  }

  {
    SCOPED_TRACE("SpanSink");
    string buf(expected.size() + 8, '#'); // With room to spare
    SpanSink out(span<char>(buf.data(), buf.size()));
    EXPECT_TRUE(fn(out));
    EXPECT_EQ(buf, string(expected) + string(8, '#'));
  }

  {
    SCOPED_TRACE("StreamSink");
    ostringstream os;
    StreamSink out(os);
    EXPECT_TRUE(fn(out));
    EXPECT_FALSE(out.bad());
    EXPECT_EQ(os.str(), expected);
  }

  {
    SCOPED_TRACE("StringSink (owned)");
    StringSink out;
    EXPECT_TRUE(fn(out));
    EXPECT_EQ(out.str(), expected);
  }

  {
    SCOPED_TRACE("StringSink (referenced)");
    string buf = "prefix:";
    StringSink out(buf);
    EXPECT_TRUE(fn(out));
    EXPECT_EQ(buf, "prefix:" + string(expected));
    EXPECT_EQ(&out.str(), &buf);
  }
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

/// A numeric punctuation facet with `'` as the thousands separator and `,` as the decimal point.
struct TestNumpunct : numpunct<char> {
  char do_decimal_point() const override { return ','; }
  char do_thousands_sep() const override { return '\''; }
  string do_grouping() const override { return "\3"; }
};

/// Returns a locale using #TestNumpunct, independent of the locales installed on the system.
const locale&
testLocale() {
  static const locale ret(locale::classic(), new TestNumpunct); // NOLINT
  return ret;
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

// `print`, `println` .......................................................................................

TEST(nio, print) {
  forEachSink("Hi there, 42", [](Sink& out) { return out.print("Hi {}, {}", "there", 42); });
  forEachSink("No args", [](Sink& out) { return out.print("No args"); });
  forEachSink("", [](Sink& out) { return out.print(""); });
  forEachSink("ab-c", [](Sink& out) { return out.print("a") && out.print("{}", 'b') && out.print("-c"); });
}

TEST(nio, println) {
  forEachSink("Hi there, 42\n", [](Sink& out) { return out.println("Hi {}, {}", "there", 42); });
  forEachSink("\n", [](Sink& out) { return out.println(""); });
  forEachSink("a\nb\n", [](Sink& out) { return out.println("a") && out.println("{}", 'b'); });
  forEachSink("a b\n", [](Sink& out) { return out.print("a ") && out.println("b"); });
}

TEST(nio, printStyle) {
  const auto style = fmt::fg(fmt::color::red) | fmt::emphasis::bold;
  const auto expected = fmt::format(style, "Hi {}", "there");
  ASSERT_NE(expected, "Hi there"); // The style must actually emit escape sequences
  forEachSink(expected, [&](Sink& out) { return out.print(style, "Hi {}", "there"); });

  // An empty style emits no escape sequences
  forEachSink("Hi there", [](Sink& out) { return out.print(fmt::text_style(), "Hi {}", "there"); });
}

TEST(nio, printlnStyle) {
  const auto style = fmt::fg(fmt::color::red) | fmt::emphasis::bold;
  const auto expected = fmt::format(style, "Hi {}", "there") + '\n'; // Line feed after the reset sequence
  forEachSink(expected, [&](Sink& out) { return out.println(style, "Hi {}", "there"); });

  forEachSink("Hi there\n", [](Sink& out) { return out.println(fmt::text_style(), "Hi {}", "there"); });
}

TEST(nio, printLocale) {
  // The locale only applies to `L` specifiers
  forEachSink("1'234'567 1234567 1'234,5", [](Sink& out) {
    return out.print(testLocale(), "{:L} {} {:L}", 1'234'567, 1'234'567, 1234.5);
  });

  // The classic locale has no grouping
  forEachSink("1234567", [](Sink& out) { return out.print(locale::classic(), "{:L}", 1'234'567); });
}

TEST(nio, printlnLocale) {
  forEachSink("1'234'567 1234567\n", [](Sink& out) {
    return out.println(testLocale(), "{:L} {}", 1'234'567, 1'234'567);
  });
}

TEST(nio, printMixed) {
  const auto style = fmt::fg(fmt::color::green);
  const auto expected = "a 1'000\n" + fmt::format(style, "b") + "\nc\nd";
  forEachSink(expected, [&](Sink& out) {
    return out.println(testLocale(), "a {:L}", 1'000) && out.println(style, "b") && out.println("c")
      && out.Sink::write("d") == 1;
  });
}

TEST(nio, printNullSink) {
  NullSink out;
  EXPECT_TRUE(out.print("Hi {}", "there"));
  EXPECT_TRUE(out.print(fmt::fg(fmt::color::red), "Hi {}", "there"));
  EXPECT_TRUE(out.print(testLocale(), "{:L}", 1'000));
  EXPECT_TRUE(out.println("Hi {}", "there"));
  EXPECT_TRUE(out.println(fmt::fg(fmt::color::red), "Hi {}", "there"));
  EXPECT_TRUE(out.println(testLocale(), "{:L}", 1'000));
}

TEST(nio, printSpanSinkTruncated) {
  {
    string str = "[....]";
    SpanSink out(span<char>(&str[1], 4));
    EXPECT_FALSE(out.print("{}", "ABCDEF")); // Only 4 of 6 chars fit
    EXPECT_EQ(str, "[ABCD]");
    EXPECT_TRUE(out.eof());
    EXPECT_FALSE(out.print("X")); // Full
    EXPECT_TRUE(out.print("")); // Nothing to write always fits
    EXPECT_EQ(str, "[ABCD]");
  }

  {
    string str = "[....]";
    SpanSink out(span<char>(&str[1], 4));
    EXPECT_TRUE(out.print("AB"));
    EXPECT_FALSE(out.println("CD")); // The line feed does not fit
    EXPECT_EQ(str, "[ABCD]");
  }

  {
    string str = "[....]";
    SpanSink out(span<char>(&str[1], 4));
    EXPECT_FALSE(out.print(testLocale(), "{:L}", 1'234'567)); // "1'234'567" is too long
    EXPECT_EQ(str, "[1'23]");
  }

  {
    string str = "[....]";
    SpanSink out(span<char>(&str[1], 4));
    EXPECT_FALSE(out.print(fmt::fg(fmt::color::red), "A")); // The escape sequences do not fit
    EXPECT_EQ(str.substr(1, 4), fmt::format(fmt::fg(fmt::color::red), "A").substr(0, 4));
    EXPECT_TRUE(out.eof());
  }
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

TEST(nio, FileSourceScan) {
  const auto path = testSource("test-nio-FileSourceScan.txt");
  FileSource in(path);
  auto result = in.scan<log::LogLevel, log::LogLevel>("{}, {}");
  ASSERT_TRUE(result);
  const auto [level1, level2] = result->values();
  EXPECT_EQ(level1, log::LogLevel::debug);
  EXPECT_EQ(level2, log::LogLevel::info);
  EXPECT_EQ(in.tell(), 11);
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
