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
#include <functional>

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
      out.write(input);
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

#if defined(__GLIBC__) || defined(__APPLE__)
#define ROCKET_TEST_HAS_FAILING_FILE

/// The state of a `FILE` made by #openFailing.
struct FailingFileState {
  string input;
  u64 pos = 0;
};

/// Reads from a #FailingFileState: first all of its input, then an I/O error.
i64
readFailing(void* cookie, char* buf, u64 size) {
  auto& state = *static_cast<FailingFileState*>(cookie);
  if (state.pos == state.input.size()) {
    errno = EIO;
    return -1;
  }
  const u64 n = min(size, state.input.size() - state.pos);
  memcpy(buf, state.input.data() + state.pos, n);
  state.pos += n;
  return static_cast<i64>(n);
}

/// Returns a `FILE` that yields @p input and then fails, like a device with an I/O error, or null on error.
unique_ptr<FILE, FileCloser>
openFailing(string_view input) {
  auto* state = new FailingFileState { string(input) }; // NOLINT: Deleted when the file is closed
#ifdef __GLIBC__
  cookie_io_functions_t functions {};
  functions.read = [](void* cookie, char* buf, size_t size) -> ssize_t {
    return readFailing(cookie, buf, size);
  };
  functions.close = [](void* cookie) {
    delete static_cast<FailingFileState*>(cookie); // NOLINT
    return 0;
  };
  unique_ptr<FILE, FileCloser> ret(fopencookie(state, "rb", functions));
#else
  unique_ptr<FILE, FileCloser> ret(funopen(
    state,
    [](void* cookie, char* buf, int size) { return static_cast<int>(readFailing(cookie, buf, size)); },
    nullptr,
    nullptr,
    [](void* cookie) {
      delete static_cast<FailingFileState*>(cookie); // NOLINT
      return 0;
    }));
#endif
  if (ret == nullptr) {
    delete state; // NOLINT
  }
  return ret;
}
#endif // __GLIBC__ || __APPLE__

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
    out.write(input);
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
    io::testing::NonseekableBuf buf { string(input) };
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
expectNoCodePoint(Source& in) {
  EXPECT_EQ(readCodePoint(in), nullopt);
}

void
expectCharacter(Source& in, string_view expected, u64 expectedTell) {
  const auto result = readCharacter(in);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(static_cast<string>(*result), expected);
  EXPECT_EQ(in.tell(), expectedTell);
}

/// Expects reading a code point from @p in, which is at position 0, to fail for invalid UTF-8.
void
expectInvalidUtf8(Source& in) {
  EXPECT_THAT([&] { (void) readCodePoint(in); }, throwsInputFailure(0, EndsWith("Invalid UTF-8")));
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
  // Like `/dev/null`, everything written is discarded, and every write succeeds
  NullSink out;
  EXPECT_EQ(out.write("abc"), 3);
  EXPECT_EQ(out.write('a'), 1);
  EXPECT_EQ(out.write(static_cast<u8>(0)), 1);
  EXPECT_EQ(out.write(""), 0);
  EXPECT_EQ(out.write("abcdef", 2, 3), 3);
  EXPECT_EQ(out.writeln("abc"), 4);
  EXPECT_TRUE(out.flush());
  EXPECT_FALSE(out.bad());
  EXPECT_FALSE(out.eof()); // Never full
}

TEST(nio, SpanSink) {
  string str = "---[abcd]---";
  const span<char> span(&str[4], 4);
  SpanSink out(span);
  EXPECT_EQ(out.write("ABCDEF"), 4); // Writing 6 chars, but only 4 fit
  EXPECT_EQ(str, "---[ABCD]---");
}

TEST(nio, writeOffset) {
  const string_view str = "abcdef";
  const span<const u8> bytes(reinterpret_cast<const u8*>(str.data()), str.size());
  StringSink out;

  EXPECT_EQ(out.write(bytes, 2, 2), 2); // "cd"
  EXPECT_EQ(out.write(bytes, 4), 2); // "ef"
  EXPECT_EQ(out.write(bytes, 5, 10), 1); // "f": fewer bytes are written if the input ends before
  EXPECT_EQ(out.write(bytes, 6), 0); // At the end: nothing to write
  EXPECT_EQ(out.write(str, 1, 2), 2); // "bc"
  EXPECT_EQ(out.writeln(str, 3, 1), 2); // "d\n"
  EXPECT_EQ(out.str(), "cdeffbcd\n");

  // An offset beyond the end is an error, and nothing is written
  EXPECT_THROW(out.write(bytes, 7), InvalidArgument);
  EXPECT_THROW(out.write(str, 7), InvalidArgument);
  EXPECT_THROW(out.writeln(str, 7), InvalidArgument);
  EXPECT_EQ(out.str(), "cdeffbcd\n");
}

TEST(nio, StreamSink) {
  ostringstream os;
  StreamSink out(os);
  out.println("Hi {}", "there");
  EXPECT_EQ(os.str(), "Hi there\n");
}

TEST(nio, StreamSinkExceptionMask) {
  // A stream buffer that accepts nothing and cannot sync, like a full device
  struct FailingOutBuf : streambuf {
  protected:

    int_type overflow(int_type) override { return traits_type::eof(); }
    int sync() override { return -1; }
  };

  // With `badbit` in the exception mask, a failure throws `ios_base::failure`. Every operation reports it as a
  // failure instead
  const vector<pair<string, function<bool(Sink&)>>> cases {
    { "print", [](Sink& out) { return out.print("Hi {}", "there"); } },
    { "print style", [](Sink& out) { return out.print(fmt::fg(fmt::color::red), "Hi {}", "there"); } },
    { "print locale", [](Sink& out) { return out.print(testLocale(), "{:L}", 1'000); } },
    { "println", [](Sink& out) { return out.println("Hi {}", "there"); } },
    { "println style", [](Sink& out) { return out.println(fmt::fg(fmt::color::red), "Hi {}", "there"); } },
    { "println locale", [](Sink& out) { return out.println(testLocale(), "{:L}", 1'000); } },
    { "write", [](Sink& out) { return out.write("abc") == 3; } },
    { "writeln", [](Sink& out) { return out.writeln("abc") == 4; } },
    { "flush", [](Sink& out) { return out.flush(); } },
  };
  for (const auto& [name, fn] : cases) {
    SCOPED_TRACE(name);
    FailingOutBuf buf;
    ostream os(&buf);
    os.exceptions(ios::badbit);
    StreamSink out(os);
    bool result = true;
    EXPECT_NO_THROW(result = fn(out));
    EXPECT_FALSE(result);
    EXPECT_TRUE(out.bad());
  }
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
      && out.write("d") == 1;
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
    out.write("12e+x rest");
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
    out.write("abcdefgh rest");
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
  // Like `/dev/null`, the source is always at the end of its input
  NullSource in;
  EXPECT_FALSE(in.bad());
  EXPECT_TRUE(in.eof());
  char c; // NOLINT
  EXPECT_EQ(in.read(c), 0);
  string str(4, '#');
  EXPECT_EQ(in.read(str), 0);
  EXPECT_EQ(str, "####");
  EXPECT_EQ(in.peek(), nullopt);
  EXPECT_TRUE(in.readAll().empty());
  EXPECT_EQ(in.readString(), "");

  // Seeking always succeeds, and the position stays 0
  EXPECT_TRUE(in.seekable());
  EXPECT_EQ(in.tell(), 0);
  EXPECT_TRUE(in.seek(0));
  EXPECT_TRUE(in.seek(42));
  EXPECT_TRUE(in.seek(-1, SeekMode::cur));
  EXPECT_TRUE(in.seek(numeric_limits<i64>::max(), SeekMode::end));
  EXPECT_EQ(in.tell(), 0);
  EXPECT_TRUE(in.eof());
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

TEST(nio, SpanSourceSeek) {
  const string str = "abc";
  SpanSource in(span<const u8>(reinterpret_cast<const u8*>(str.data()), str.size()));

  EXPECT_TRUE(in.seek(2, SeekMode::beg));
  EXPECT_EQ(in.position(), 2);
  EXPECT_FALSE(in.seek(4, SeekMode::beg)); // Beyond the end: fails, position unchanged
  EXPECT_EQ(in.position(), 2);
  EXPECT_FALSE(in.seek(-3, SeekMode::cur)); // Before the beginning
  EXPECT_EQ(in.position(), 2);
  EXPECT_TRUE(in.seek(0, SeekMode::end));
  EXPECT_EQ(in.position(), 3);
}

TEST(nio, StringSource) {
  StringSource in("Hello, world!");
  const auto str = in.readString();
  EXPECT_EQ(str, "Hello, world!");
}

TEST(nio, StringSourceSeek) {
  const string str128 = testString(128);
  StringSource in(str128);

  in.seek(0);
  EXPECT_EQ(in.position(), 0);
  char c = '\0';
  EXPECT_EQ(in.read(c), 1);
  EXPECT_EQ(c, '0');
  EXPECT_EQ(in.position(), 1);

  in.seek(127);
  EXPECT_EQ(in.position(), 127);
  EXPECT_EQ(in.read(c), 1);
  EXPECT_EQ(c, '7');
  EXPECT_EQ(in.position(), 128);

  EXPECT_FALSE(in.seek(129)); // Beyond the end: fails, position unchanged
  EXPECT_EQ(in.position(), 128);
  EXPECT_TRUE(in.seek(128)); // The end itself is in range
  EXPECT_EQ(in.position(), 128);

  in.seek(-11, SeekMode::cur);
  EXPECT_EQ(in.position(), 117);

  EXPECT_FALSE(in.seek(-1)); // Before the beginning: fails, position unchanged
  EXPECT_EQ(in.position(), 117);

  in.seek(0, SeekMode::end);
  EXPECT_EQ(in.position(), 128);

  in.seek(-10, SeekMode::end);
  EXPECT_EQ(in.position(), 118);

  EXPECT_FALSE(in.seek(3, SeekMode::end));
  EXPECT_EQ(in.position(), 118);
  EXPECT_FALSE(in.seek(11, SeekMode::cur));
  EXPECT_EQ(in.position(), 118);
  EXPECT_FALSE(in.seek(numeric_limits<i64>::max(), SeekMode::cur));
  EXPECT_EQ(in.position(), 118);
  EXPECT_TRUE(in.seek(10, SeekMode::cur));
  EXPECT_EQ(in.position(), 128);

  EXPECT_FALSE(in.seek(-200, SeekMode::end));
  EXPECT_EQ(in.position(), 128);
  EXPECT_FALSE(in.seek(numeric_limits<i64>::min(), SeekMode::cur));
  EXPECT_EQ(in.position(), 128);
}

#ifndef ROCKET_OS_WINDOWS
namespace {

/// Redirects a file descriptor to one end of a new pipe, and restores it on destruction.
struct PipeRedirect {
  explicit PipeRedirect(i32 fd) : fd_(fd) {
    array<i32, 2> fds {};
    if (pipe(fds.data()) != 0) {
      return;
    }
    saved_ = dup(fd);
    dup2(fd == STDIN_FILENO ? fds[0] : fds[1], fd);
    close(fds[0]);
    close(fds[1]);
  }

  PipeRedirect(const PipeRedirect&) = delete;
  PipeRedirect& operator=(const PipeRedirect&) = delete;

  ~PipeRedirect() {
    if (saved_ >= 0) {
      dup2(saved_, fd_);
      close(saved_);
    }
  }

  [[nodiscard]] bool ok() const { return saved_ >= 0; }

private:

  i32 fd_;
  i32 saved_ = -1;
};

} // namespace

TEST(nio, standardStreamsOverPipes) {
  // With `stdin` and `stdout` over pipes, the standard streams are not seekable. That is decided from the
  // `FILE` alone, without seeking the stream buffer, which would seek the `FILE` and set `errno`
  fflush(stdout);
  cout.flush();
  {
    const PipeRedirect in(STDIN_FILENO);
    const PipeRedirect out(STDOUT_FILENO);
    ASSERT_TRUE(in.ok());
    ASSERT_TRUE(out.ok());

    errno = 0;
    EXPECT_EQ(io::tell(cin), NPOS);
    EXPECT_FALSE(io::seekable(cin));
    EXPECT_FALSE(io::seekable(cout));
    EXPECT_EQ(errno, 0);

    StreamSource src(cin);
    EXPECT_FALSE(src.seekable());
    EXPECT_EQ(src.tell(), nullopt);
    EXPECT_FALSE(src.seek(0));
    EXPECT_FALSE(src.seek(0, SeekMode::cur));
    EXPECT_EQ(errno, 0);
    EXPECT_FALSE(cin.fail());
  }
  clearerr(stdin);
}
#endif // !ROCKET_OS_WINDOWS

TEST(nio, seekableSources) {
  const auto temp = rocket::filesystem::tempFile();
  {
    FileSink out(temp);
    out.write("abc");
  }

  EXPECT_TRUE(StringSource("abc").seekable());
  EXPECT_TRUE(SpanSource().seekable());
  EXPECT_TRUE(FileSource(temp).seekable());
  EXPECT_TRUE(NullSource().seekable()); // Like `/dev/null`, unlike `FileSource` over it

#ifndef ROCKET_OS_WINDOWS
  const auto pipeFile = openPipe("abc");
  ASSERT_NE(pipeFile, nullptr);
  EXPECT_FALSE(FileSource(pipeFile.get()).seekable());
#endif

  istringstream iss("abc");
  EXPECT_TRUE(StreamSource(iss).seekable());

  ifstream ifs(temp, ios::binary);
  EXPECT_TRUE(StreamSource(ifs).seekable());

  io::testing::NonseekableBuf buf("abc");
  istream is(&buf);
  EXPECT_FALSE(StreamSource(is).seekable());

  // Like `stdin`, which depends on how the test is run
  EXPECT_EQ(StreamSource(cin).seekable(), io::seekable(stdin));

  // A source is seekable exactly if it can tell its position, before and after reading
  forEachScanSource("abc", [](Source& in) {
    EXPECT_EQ(in.seekable(), in.tell().has_value());
    char c = '\0';
    ASSERT_EQ(in.read(c), 1);
    EXPECT_EQ(in.seekable(), in.tell().has_value());
  });
}

// `readln` .................................................................................................

TEST(nio, readln) {
  const auto test = [](string_view input, const vector<string>& expected) {
    forEachScanSource(input, [&](Source& in) {
      for (const auto& line : expected) {
        EXPECT_EQ(in.readln(), line);
      }
      EXPECT_EQ(in.readln(), nullopt); // No more input
      EXPECT_EQ(in.readln(), nullopt); // Still none
    });
  };

  test("First line\r\nSecond line\n", { "First line", "Second line" });
  test("a\nb", { "a", "b" }); // The last line need not end with a line feed
  test("\n\n", { "", "" }); // Empty lines are not the end of the input
  test("", {});
  test("a\r", { "a\r" }); // A carriage return is only removed before a line feed
  test("a\r\r\n", { "a\r" }); // Only one carriage return is removed
}

TEST(nio, readlnIoError) {
  // A complete line before the error is returned. The incomplete line at the error is dropped, and the source
  // is bad

#ifdef ROCKET_TEST_HAS_FAILING_FILE
  {
    SCOPED_TRACE("FileSource");
    const auto file = openFailing("a\nbc");
    ASSERT_NE(file, nullptr);
    FileSource in(file.get());
    EXPECT_EQ(in.readln(), "a");
    EXPECT_FALSE(in.bad());
    EXPECT_EQ(in.readln(), nullopt);
    EXPECT_TRUE(in.bad());
  }
#endif

  {
    SCOPED_TRACE("StreamSource");
    io::testing::FailingBuf buf("a\nbc");
    istream is(&buf);
    StreamSource in(is);
    EXPECT_EQ(in.readln(), "a");
    EXPECT_FALSE(in.bad());
    EXPECT_EQ(in.readln(), nullopt);
    EXPECT_TRUE(in.bad());
  }
}

TEST(nio, readlnNullSource) {
  NullSource in;
  EXPECT_EQ(in.readln(), nullopt);
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
    ASSERT_EQ(in.read(c), 1);
    ASSERT_EQ(in.peek(), 'b');
    const auto result = in.scan<string, i32>("{} {}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->values(), make_tuple("b"s, 12));
    expectTell(in, 5);
    EXPECT_EQ(in.readln(), " cd");
  });
}

TEST(nio, scanAfterPeek) {
  // Peeking leaves the character in the source, so the next scan sees it
  forEachScanSource("12 34 x", [](Source& in) {
    auto result = in.scan<i32>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 12);

    EXPECT_EQ(in.peek(), ' ');
    result = in.scan<i32>("{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 34);

    EXPECT_EQ(in.peek(), ' ');
    EXPECT_EQ(in.peek(), ' ');
    const auto chr = in.scan<char>(" {}");
    ASSERT_TRUE(chr);
    EXPECT_EQ(chr->value(), 'x');

    EXPECT_EQ(in.peek(), nullopt);
    EXPECT_FALSE(in.scan<i32>("{}"));
    expectRest(in, "");
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
    EXPECT_EQ(in.read(c), 1);
    EXPECT_EQ(c, 'a');
    EXPECT_EQ(in.peek(), 'b');
    EXPECT_EQ(in.tell(), 1);

    EXPECT_EQ(in.read(c), 1);
    EXPECT_EQ(c, 'b');
    EXPECT_EQ(in.peek(), nullopt);
    EXPECT_TRUE(in.eof());
  });
}

// `readCharacter` ..........................................................................................

TEST(nio, readCharacter) {
  forEachSource("ab", [](Source& in) {
    expectCharacter(in, "a", 1);
    expectCharacter(in, "b", 2);
    EXPECT_EQ(readCharacter(in), nullopt); // End of input
  });

  // A combining mark continues the character
  forEachSource("a\u0301b", [](Source& in) {
    expectCharacter(in, "a\u0301", 3);
    expectCharacter(in, "b", 4);
  });

  // CR LF is a single character, although both code points are ASCII
  forEachSource("\r\nx", [](Source& in) {
    expectCharacter(in, "\r\n", 2);
    expectCharacter(in, "x", 3);
  });

  // Man, ZWJ, woman, ZWJ, boy
  forEachSource("\U0001F468\u200D\U0001F469\u200D\U0001F466x", [](Source& in) {
    expectCharacter(in, "\U0001F468\u200D\U0001F469\u200D\U0001F466", 18);
    expectCharacter(in, "x", 19);
  });

  // Two flags, each made of two regional indicators
  forEachSource("\U0001F1E9\U0001F1EA\U0001F1EB\U0001F1F7", [](Source& in) {
    expectCharacter(in, "\U0001F1E9\U0001F1EA", 8);
    expectCharacter(in, "\U0001F1EB\U0001F1F7", 16);
  });

  // A non-ASCII code point that does not continue the character is gone back over
  forEachSource("äö", [](Source& in) {
    expectCharacter(in, "ä", 2);
    expectCharacter(in, "ö", 4);
  });
}

TEST(nio, readCharacterInvalidUtf8) {
  // At the start of the character, invalid UTF-8 throws, and the source is not advanced
  forEachSource("\xFF" "a", [](Source& in) {
    EXPECT_THAT([&] { (void) readCharacter(in); }, throwsInputFailure(0, EndsWith("Invalid UTF-8")));
    expectRest(in, "\xFF" "a");
  });

  // After the first code point, invalid UTF-8 ends the character, and stays in the source
  forEachSource("a\xFF", [](Source& in) {
    expectCharacter(in, "a", 1);
    expectRest(in, "\xFF");
  });
  forEachSource("a\xC3", [](Source& in) { // Truncated
    expectCharacter(in, "a", 1);
    expectRest(in, "\xC3");
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
    expectNoCodePoint(in);
    EXPECT_TRUE(in.eof());
  });

  forEachSource("a", [](Source& in) {
    expectCodePoint(in, U'a', 1);
    expectNoCodePoint(in);
    EXPECT_TRUE(in.eof());
    expectNoCodePoint(in); // Still at the end
  });
}

// On invalid UTF-8, the source is not advanced. All of these sources can seek

TEST(nio, readCodePointInvalidLeadByte) {
  for (const string_view input : { "\xFFx"sv, "\x80x"sv, "\xC0\x80x"sv }) {
    forEachSource(input, [input](Source& in) {
      expectInvalidUtf8(in);
      expectRest(in, input);
    });
  }
}

TEST(nio, readCodePointTruncatedSequence) {
  for (const string_view input : { "\xC3"sv, "\xE2\x82"sv, "\xF0\x9F\x98"sv }) {
    forEachSource(input, [input](Source& in) {
      expectInvalidUtf8(in);
      expectRest(in, input);
    });
  }
}

TEST(nio, readCodePointInvalidContinuation) {
  for (const string_view input : { "\xC3" "Ax"sv, "\xE2\x82" "Ax"sv, "\xED\xA0\x80x"sv }) { // Last: surrogate
    forEachSource(input, [input](Source& in) {
      expectInvalidUtf8(in);
      expectRest(in, input);
    });
  }
}

// EOF
