/*
 * test-nio.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/io/io.h"
#include "rocket/log/log.h"
#include "rocket/nio/nio-utils.h"
#include "rocket/nio/nio.h"

#include <fstream>

#ifndef ROCKET_OS_WINDOWS
#include <unistd.h> // `pipe`, `write`, `close`
#endif

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

/// Closes a `FILE`.
struct FileCloser {
  void operator()(FILE* file) const { fclose(file); }
};

/**
 * Opens @p path for reading with @p buffer as its buffer, or returns null on error. @p buffer must outlive
 * the file. glibc ignores the size if the buffer is null, so the caller provides it.
 */
unique_ptr<FILE, FileCloser>
openWithBuffer(const std::filesystem::path& path, span<char> buffer) {
  unique_ptr<FILE, FileCloser> ret(fopen(path.string().c_str(), "rb"));
  if (ret != nullptr && setvbuf(ret.get(), buffer.data(), _IOFBF, buffer.size()) != 0) {
    ret.reset();
  }
  return ret;
}

#ifndef ROCKET_OS_WINDOWS
/**
 * Returns a `FILE` reading @p input from a pipe, which cannot seek, or null on error. If @p buffer is not
 * empty, it is used as the buffer of the file and must outlive it.
 *
 * The input is written in full before reading starts, so it must fit into the pipe.
 */
unique_ptr<FILE, FileCloser>
openPipe(string_view input, span<char> buffer = {}) {
  array<i32, 2> fds {};
  if (pipe(fds.data()) != 0) {
    return {};
  }
  const auto written = write(fds[1], input.data(), input.size());
  close(fds[1]);
  unique_ptr<FILE, FileCloser> ret(fdopen(fds[0], "rb"));
  if (ret == nullptr) {
    close(fds[0]);
    return {};
  }
  if (written != static_cast<ssize_t>(input.size()) ||
    (not buffer.empty() && setvbuf(ret.get(), buffer.data(), _IOFBF, buffer.size()) != 0)) {
    ret.reset();
  }
  return ret;
}
#endif // !ROCKET_OS_WINDOWS

/**
 * Calls @p fn with each kind of source over @p input: contiguous, seekable, and nonseekable, and files with
 * a tiny buffer, which scnlib refills during a scan.
 *
 * Nonseekable sources report no position from `tell`, so @p fn checks the position with #expectRest or
 * #expectTell.
 */
template<typename Fn>
void
forEachScanSource(string_view input, Fn fn) {
  SCOPED_TRACE(string(input));

  {
    SCOPED_TRACE("StringSource");
    StringSource in(input);
    fn(in);
  }

  {
    SCOPED_TRACE("SpanSource");
    SpanSource in(span<const u8>(reinterpret_cast<const u8*>(input.data()), input.size()));
    fn(in);
  }

  const auto temp = rocket::filesystem::tempFile();
  {
    FileSink out(temp);
    out.Sink::write(input);
  }

  {
    SCOPED_TRACE("FileSource");
    FileSource in(temp);
    fn(in);
  }

#ifndef ROCKET_OS_WINDOWS // The Windows C runtime only guarantees one character of putback
  {
    SCOPED_TRACE("FileSource (3-byte buffer)");
    array<char, 3> buf {};
    const auto file = openWithBuffer(temp, buf);
    ASSERT_NE(file, nullptr);
    FileSource in(file.get());
    fn(in);
  }

  {
    SCOPED_TRACE("FileSource (pipe)");
    const auto file = openPipe(input);
    ASSERT_NE(file, nullptr);
    FileSource in(file.get());
    fn(in);
  }

  {
    SCOPED_TRACE("FileSource (pipe, 3-byte buffer)");
    array<char, 3> buf {};
    const auto file = openPipe(input, buf);
    ASSERT_NE(file, nullptr);
    FileSource in(file.get());
    fn(in);
  }
#endif

  {
    SCOPED_TRACE("StreamSource (istringstream)");
    istringstream is { string(input) };
    StreamSource in(is);
    fn(in);
  }

  {
    SCOPED_TRACE("StreamSource (ifstream)");
    ifstream is(temp, ios::binary);
    StreamSource in(is);
    fn(in);
  }

  {
    SCOPED_TRACE("StreamSource (nonseekable)");
    NonseekableBuf buf { string(input) };
    istream is(&buf);
    StreamSource in(is);
    fn(in);
  }
}

/// Expects the remaining input of @p in to equal @p expected. Consumes the remaining input.
void
expectRest(Source& in, string_view expected) {
  EXPECT_EQ(in.readString(), expected);
}

/// Expects the position of @p in to equal @p expected, if @p in can tell its position.
void
expectTell(Source& in, u64 expected) {
  if (const auto pos = in.tell()) {
    EXPECT_EQ(*pos, expected);
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

TEST(nio, seekableSinks) {
  const auto temp = rocket::filesystem::tempFile();
  EXPECT_TRUE(FileSink(temp).seekable());

#ifndef ROCKET_OS_WINDOWS
  array<i32, 2> fds {};
  ASSERT_EQ(pipe(fds.data()), 0);
  close(fds[0]);
  const unique_ptr<FILE, FileCloser> pipeFile(fdopen(fds[1], "wb"));
  ASSERT_NE(pipeFile, nullptr);
  EXPECT_FALSE(FileSink(pipeFile.get()).seekable());
#endif

  EXPECT_FALSE(NullSink().seekable());

  string str = "....";
  EXPECT_FALSE(SpanSink(span<char>(str.data(), str.size())).seekable());

  EXPECT_FALSE(StringSink().seekable());

  ostringstream os;
  EXPECT_TRUE(StreamSink(os).seekable());

  ofstream ofs(temp, ios::binary);
  EXPECT_TRUE(StreamSink(ofs).seekable());

  streambuf* nullBuf = nullptr;
  ostream noBuf(nullBuf);
  EXPECT_FALSE(StreamSink(noBuf).seekable());

  struct NoSeekBuf : streambuf {} noSeekBuf; // `streambuf` cannot seek by default
  ostream noSeek(&noSeekBuf);
  EXPECT_FALSE(StreamSink(noSeek).seekable());

  // Like `stdout`, which depends on how the test is run
  EXPECT_EQ(StreamSink(cout).seekable(), io::seekable(stdout));
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

#ifndef ROCKET_OS_WINDOWS // The Windows C runtime only guarantees one character of putback
// A tiny `FILE` buffer makes scnlib refill it during a scan, so that backtracking puts back input from an
// earlier buffer. Without the scnlib patch, this returned input from the current buffer twice
TEST(nio, FileSourceScanBacktrackAcrossBufferRefill) {
  const auto temp = rocket::filesystem::tempFile();
  {
    FileSink out(temp);
    out.Sink::write("12e+x rest");
  }

  array<char, 3> buf {}; // Buffers "12e", "+x ", ...
  const auto file = openWithBuffer(temp, buf);
  ASSERT_NE(file, nullptr);
  FileSource in(file.get());

  // Reads "12e+", then backtracks to "12", which is in the first buffer
  const auto result = in.scan<double>("{}");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->value(), 12.0);
  EXPECT_EQ(in.readString(), "e+x rest");
}

TEST(nio, FileSourceScanFailureAcrossBufferRefill) {
  const auto temp = rocket::filesystem::tempFile();
  {
    FileSink out(temp);
    out.Sink::write("abcdefgh rest");
  }

  array<char, 4> buf {};
  const auto file = openWithBuffer(temp, buf);
  ASSERT_NE(file, nullptr);
  FileSource in(file.get());

  // Reads past the 'r' of "rest" before failing, then puts back everything
  EXPECT_FALSE((in.scan<string, i32>("{} {}")));
  EXPECT_EQ(in.readString(), "abcdefgh rest");
}
#endif

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
  EXPECT_EQ(in.position(), 0);
  char c = '\0';
  EXPECT_EQ(in.Source::read(c), 1);
  EXPECT_EQ(c, '0');
  EXPECT_EQ(in.position(), 1);

  in.Source::seek(127);
  EXPECT_EQ(in.position(), 127);
  EXPECT_EQ(in.Source::read(c), 1);
  EXPECT_EQ(c, '7');
  EXPECT_EQ(in.position(), 128);

  in.Source::seek(129);
  EXPECT_EQ(in.position(), 128);

  in.seek(-11, SeekMode::cur);
  EXPECT_EQ(in.position(), 117);

  EXPECT_FALSE(in.Source::seek(-1)); // Before the beginning: fails, position unchanged
  EXPECT_EQ(in.position(), 117);

  in.seek(0, SeekMode::end);
  EXPECT_EQ(in.position(), 128);

  in.seek(-10, SeekMode::end);
  EXPECT_EQ(in.position(), 118);

  in.seek(3, SeekMode::end);
  EXPECT_EQ(in.position(), 128);

  EXPECT_FALSE(in.seek(-200, SeekMode::end));
  EXPECT_EQ(in.position(), 128);
  EXPECT_FALSE(in.seek(numeric_limits<i64>::min(), SeekMode::cur));
  EXPECT_EQ(in.position(), 128);
}

TEST(nio, seekableSources) {
  const auto temp = rocket::filesystem::tempFile();
  {
    FileSink out(temp);
    out.Sink::write("abc");
  }

  EXPECT_TRUE(StringSource("abc").seekable());
  EXPECT_TRUE(SpanSource().seekable());
  EXPECT_TRUE(FileSource(temp).seekable());
  EXPECT_FALSE(NullSource().seekable());

#ifndef ROCKET_OS_WINDOWS
  const auto pipeFile = openPipe("abc");
  ASSERT_NE(pipeFile, nullptr);
  EXPECT_FALSE(FileSource(pipeFile.get()).seekable());
#endif

  istringstream iss("abc");
  EXPECT_TRUE(StreamSource(iss).seekable());

  ifstream ifs(temp, ios::binary);
  EXPECT_TRUE(StreamSource(ifs).seekable());

  NonseekableBuf buf("abc");
  istream is(&buf);
  EXPECT_FALSE(StreamSource(is).seekable());

  // Like `stdin`, which depends on how the test is run
  EXPECT_EQ(StreamSource(cin).seekable(), io::seekable(stdin));

  // A source is seekable exactly if it can tell its position, before and after reading
  forEachScanSource("abc", [](Source& in) {
    EXPECT_EQ(in.seekable(), in.tell().has_value());
    char c = '\0';
    ASSERT_EQ(in.Source::read(c), 1);
    EXPECT_EQ(in.seekable(), in.tell().has_value());
  });
}

// `scan` ...................................................................................................

TEST(nio, scanInteger) {
  forEachScanSource("42 rest", [](Source& in) {
    const auto result = in.scan<i32>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 42);
    expectTell(in, 2);
    expectRest(in, " rest");
  });

  forEachScanSource("-17", [](Source& in) { // At the end of the input
    const auto result = in.scan<i64>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), -17);
    expectTell(in, 3);
    expectRest(in, "");
  });

  forEachScanSource("0x1F;", [](Source& in) { // Base detected from the prefix
    const auto result = in.scan<u32>("{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 31);
    expectRest(in, ";");
  });
}

TEST(nio, scanFloat) {
  forEachScanSource("3.25,", [](Source& in) {
    const auto result = in.scan<double>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 3.25);
    expectTell(in, 4);
    expectRest(in, ",");
  });

  // Reads "12e+", then backtracks to "12", as the exponent has no digits
  forEachScanSource("12e+x rest", [](Source& in) {
    const auto result = in.scan<double>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 12.0);
    expectTell(in, 2);
    expectRest(in, "e+x rest");
  });
}

TEST(nio, scanStringAndChar) {
  forEachScanSource("hello world", [](Source& in) { // A string ends at whitespace
    const auto result = in.scan<string>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "hello");
    expectRest(in, " world");
  });

  forEachScanSource("xyz", [](Source& in) {
    const auto result = in.scan<char>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 'x');
    expectTell(in, 1);
    expectRest(in, "yz");
  });
}

TEST(nio, scanBool) {
  forEachScanSource("true 0", [](Source& in) {
    const auto result = in.scan<bool, bool>("{} {}");
    ASSERT_TRUE(result);
    const auto [val1, val2] = result->values();
    EXPECT_TRUE(val1);
    EXPECT_FALSE(val2);
    expectRest(in, "");
  });
}

TEST(nio, scanMultipleValues) {
  forEachScanSource("2026-11-14T12", [](Source& in) {
    const auto result = in.scan<i32, u32, u32>("{}-{}-{}");
    ASSERT_TRUE(result);
    const auto [year, month, day] = result->values();
    EXPECT_EQ(year, 2026);
    EXPECT_EQ(month, 11);
    EXPECT_EQ(day, 14);
    expectTell(in, 10);
    expectRest(in, "T12");
  });

  forEachScanSource("debug, info.", [](Source& in) { // Custom scanner
    const auto result = in.scan<log::LogLevel, log::LogLevel>("{}, {}");
    ASSERT_TRUE(result);
    const auto [level1, level2] = result->values();
    EXPECT_EQ(level1, log::LogLevel::debug);
    EXPECT_EQ(level2, log::LogLevel::info);
    expectRest(in, ".");
  });
}

#ifndef ROCKET_OS_WINDOWS
TEST(nio, scanTimePointFromPipe) {
  using namespace std::chrono;
  constexpr string_view INPUT = "2026-11-14T12:34:56 rest";
  const sys_seconds expected = sys_days(2026y / November / 14) + 12h + 34min + 56s;

  // With the default buffer, and with a tiny buffer, which scnlib refills during the scan
  array<char, 3> tiny {};
  for (const span<char> buffer : { span<char>(), span<char>(tiny) }) {
    SCOPED_TRACE(fmt::format("buffer size {}", buffer.size()));
    const auto pipeFile = openPipe(INPUT, buffer);
    ASSERT_NE(pipeFile, nullptr);
    FileSource in(pipeFile.get());
    ASSERT_FALSE(in.seekable());

    const auto result = in.scan<sys_seconds>("{:%FT%T}");
    ASSERT_TRUE(result) << result.error().msg();
    EXPECT_EQ(result->value(), expected);
    EXPECT_EQ(in.tell(), nullopt);
    expectRest(in, " rest"); // Only the time point was consumed
  }
}
#endif // !ROCKET_OS_WINDOWS

TEST(nio, scanSkipsWhitespace) {
  // Leading whitespace is skipped, trailing whitespace is not consumed
  forEachScanSource(" \t\n 42  ", [](Source& in) {
    const auto result = in.scan<i32>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 42);
    expectTell(in, 6);
    expectRest(in, "  ");
  });

  // Whitespace in the format string matches any amount of whitespace, including none
  forEachScanSource("1   2,3 4", [](Source& in) {
    const auto result = in.scan<i32, i32, i32, i32>("{} {},{} {}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->values(), make_tuple(1, 2, 3, 4));
    expectRest(in, "");
  });
}

TEST(nio, scanSuccessive) {
  forEachScanSource("1 2 3", [](Source& in) {
    for (i32 i = 1; i <= 3; ++i) {
      const auto result = in.scan<i32>("{}");
      ASSERT_TRUE(result);
      EXPECT_EQ(result->value(), i);
    }
    const auto result = in.scan<i32>("{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::end_of_input);
    expectRest(in, "");
  });
}

TEST(nio, scanFailureResets) {
  // A failed scan leaves the source where it was before the scan
  forEachScanSource("abc 1", [](Source& in) { // Invalid value
    const auto result = in.scan<i32>("{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
    expectTell(in, 0);
    expectRest(in, "abc 1");
  });

  forEachScanSource("  12 abc", [](Source& in) { // The first value succeeds, the second fails
    const auto result = in.scan<i32, i32>("{} {}");
    ASSERT_FALSE(result);
    expectTell(in, 0);
    expectRest(in, "  12 abc");
  });

  forEachScanSource("12;34", [](Source& in) { // Literal mismatch
    const auto result = in.scan<i32, i32>("{},{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_literal);
    expectTell(in, 0);
    expectRest(in, "12;34");
  });

  forEachScanSource("abcdefgh rest", [](Source& in) { // Reads past several 3-byte buffers before failing
    const auto result = in.scan<string, i32>("{} {}");
    ASSERT_FALSE(result);
    expectTell(in, 0);
    expectRest(in, "abcdefgh rest");
  });

  forEachScanSource("x 5", [](Source& in) { // After a failure, a different scan succeeds
    ASSERT_FALSE(in.scan<i32>("{}"));
    const auto result = in.scan<char, i32>("{} {}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->values(), make_tuple('x', 5));
    expectRest(in, "");
  });
}

TEST(nio, scanEndOfInput) {
  forEachScanSource("", [](Source& in) {
    const auto result = in.scan<i32>("{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::end_of_input);
    expectRest(in, "");
  });

  forEachScanSource("   ", [](Source& in) { // Only whitespace
    ASSERT_FALSE(in.scan<i32>("{}"));
    expectRest(in, "   ");
  });

  forEachScanSource("7,", [](Source& in) { // Input ends before the second value
    ASSERT_FALSE((in.scan<i32, i32>("{},{}")));
    expectRest(in, "7,");
  });
}

TEST(nio, scanLocale) {
  // The locale only applies to `L` specifiers
  forEachScanSource("1,5 2,5", [](Source& in) {
    const auto result = in.scan<double, double>(testLocale(), "{:L} {}");
    ASSERT_TRUE(result);
    const auto [val1, val2] = result->values();
    EXPECT_EQ(val1, 1.5);
    EXPECT_EQ(val2, 2.0);
    expectRest(in, ",5");
  });

  forEachScanSource("x", [](Source& in) { // A failed scan with a locale resets, too
    ASSERT_FALSE(in.scan<double>(testLocale(), "{:L}"));
    expectRest(in, "x");
  });
}

TEST(nio, scanAfterRead) {
  // Scanning starts at the current position, after input read otherwise
  forEachScanSource("ab 12 cd", [](Source& in) {
    char c = '\0';
    ASSERT_EQ(in.Source::read(c), 1);
    ASSERT_EQ(in.peek(), 'b');
    const auto result = in.scan<string, i32>("{} {}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->values(), make_tuple("b"s, 12));
    expectTell(in, 5);
    EXPECT_EQ(in.readln(), " cd");
  });
}

TEST(nio, scanNullSource) {
  NullSource in;
  const auto result = in.scan<i32>("{}");
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().code(), scn::scan_error::end_of_input);
  EXPECT_FALSE(in.scan<i32>(testLocale(), "{:L}"));
  EXPECT_TRUE(in.scan<>("")); // Nothing expected, so nothing missing
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
