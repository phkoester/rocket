/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/format.h"
#include "rocket/cl/cl.h"
#include "rocket/io/io.h"
#include "rocket/nio/nio.h"
#include "rocket/str/Noun.h"

#ifdef ROCKET_OS_WINDOWS
  #include <fcntl.h> // `_O_BINARY`, `_O_TEXT`
#endif

#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace {

// `Mode` ---------------------------------------------------------------------------------------------------

/// The translation mode of a `FILE` pointer.
enum class Mode : u8 {
  Binary, ///< No translation. Always the case on POSIX systems
  Text, ///< CR/LF translation (Windows only)
  Unicode, ///< One of the Unicode translation modes, e.g., `_O_U8TEXT` (Windows only)
  Unknown ///< The mode cannot be determined
};

/**
 * Determines the translation mode of @p file.
 *
 * On Windows, `_setmode` returns the previous mode, so the mode is set and then restored. Call this only
 * before any I/O on @p file.
 */
Mode
queryMode([[maybe_unused]] FILE* file) {
#ifdef ROCKET_OS_WINDOWS
  const i32 fd = _fileno(file);
  if (fd < 0) {
    return Mode::Unknown;
  }
  const i32 old = _setmode(fd, _O_BINARY);
  if (old == -1) {
    return Mode::Unknown;
  }
  _setmode(fd, old);
  switch (old) {
    case _O_BINARY: return Mode::Binary;
    case _O_TEXT: return Mode::Text;
    default: return Mode::Unicode;
  }
#else
  return Mode::Binary;
#endif
}

string_view
modeName(Mode mode) {
  switch (mode) {
#ifdef ROCKET_OS_WINDOWS
    case Mode::Binary: return "binary mode";
#else
    case Mode::Binary: return "binary mode (POSIX has no text mode)";
#endif
    case Mode::Text: return "text mode (CR/LF conversion)";
    case Mode::Unicode: return "Unicode text mode";
    case Mode::Unknown: return "unknown mode";
  }
  ROCKET_TERMINATE_UNREACHABLE_CODE();
}

/// Switches `stdin` and `stdout` to binary mode. Has no effect on POSIX systems.
void
setBinaryMode() {
#ifdef ROCKET_OS_WINDOWS
  if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
    process().error(nio::err, EXIT_FAILURE, "Cannot switch `stdin` to binary mode");
  }
  if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
    process().error(nio::err, EXIT_FAILURE, "Cannot switch `stdout` to binary mode");
  }
#endif
}

// `Modes`, `Counts` ----------------------------------------------------------------------------------------

/// The translation modes of `stdin` and `stdout`.
struct Modes {
  Mode in;
  Mode out;
};

/// Byte counts of a copy operation.
struct Counts {
  /// The number of bytes read, after CR/LF conversion.
  u64 read = 0;
  /**
   * The number of bytes consumed from `stdin`, before CR/LF conversion.
   *
   * Only determined if `stdin` is in text mode. This is the difference of the stream positions, so it is
   * only available if `stdin` is seekable.
   */
  optional<u64> readRaw;
  /// The number of bytes written, before LF/CRLF conversion.
  u64 written = 0;
  /**
   * The number of bytes produced on `stdout`, after LF/CRLF conversion.
   *
   * Only determined if `stdout` is in text mode. Every LF becomes CRLF, so this is always known.
   */
  optional<u64> writtenRaw;

  /// Adds @p rhs. A raw count becomes unknown as soon as one of the added raw counts is unknown.
  void
  add(const Counts& rhs) {
    read += rhs.read;
    readRaw = readRaw && rhs.readRaw ? optional(*readRaw + *rhs.readRaw) : nullopt;
    written += rhs.written;
    writtenRaw = writtenRaw && rhs.writtenRaw ? optional(*writtenRaw + *rhs.writtenRaw) : nullopt;
  }
};

string
formatCounts(const Counts& counts, const Modes& modes) {
  string ret = fmt::format("read {}", str::noun::byte.amount(static_cast<i64>(counts.read)));
  if (modes.in == Mode::Text) {
    if (counts.readRaw) {
      ret += fmt::format(" ({} before CR/LF conversion)", *counts.readRaw);
    } else {
      ret += " (count before CR/LF conversion unknown, `stdin` is not seekable)";
    }
  }
  ret += fmt::format(", wrote {}", str::noun::byte.amount(static_cast<i64>(counts.written)));
  if (modes.out == Mode::Text && counts.writtenRaw) {
    ret += fmt::format(" ({} after LF/CRLF conversion)", *counts.writtenRaw);
  }
  return ret;
}

// `Io` -----------------------------------------------------------------------------------------------------

/**
 * Reads lines from standard input and writes to standard output, using one specific I/O method.
 */
struct Io {
  virtual ~Io() = default;

  /// Returns a description of the I/O method.
  [[nodiscard]] virtual string_view description() const = 0;

  /// Flushes standard output.
  virtual void flush() = 0;

  /**
   * Reads a line from standard input.
   *
   * @return the line read, including the trailing line feed if there was one, or `nullopt` if no more input
   *   is available
   */
  virtual optional<string> readLine() = 0;

  /**
   * Returns the current position in standard input.
   *
   * @return the current position, or #rocket::NPOS if standard input is not seekable
   */
  [[nodiscard]] virtual u64 tell() = 0;

  /**
   * Writes a string to standard output.
   *
   * @return the number of bytes written
   */
  virtual u64 write(string_view str) = 0;
};

// `FileIo` -------------------------------------------------------------------------------------------------

/// Reads from the `stdin` `FILE` pointer, writes to `stdout` via `fwrite`.
struct FileIo : Io {
  [[nodiscard]] string_view
  description() const override {
    return "`file`: `fgets(stdin)` -> `fwrite(stdout)`";
  }

  void
  flush() override {
    fflush(stdout);
  }

  optional<string>
  readLine() override {
    string ret;
    array<char, 256> buf {};
    while (fgets(buf.data(), static_cast<i32>(buf.size()), stdin) != nullptr) {
      ret.append(buf.data());
      if (ret.ends_with('\n')) {
        break;
      }
    }
    if (ferror(stdin) != 0) {
      process().error(nio::err, EXIT_FAILURE, "Cannot read from `stdin`");
    }
    if (ret.empty()) {
      return nullopt;
    }
    return ret;
  }

  [[nodiscard]] u64
  tell() override {
    return io::tell(stdin);
  }

  u64
  write(string_view str) override {
    return fwrite(str.data(), 1, str.size(), stdout);
  }
};

// `StreamIo` -----------------------------------------------------------------------------------------------

/// Reads from `std::cin`, writes to `std::cout`.
struct StreamIo : Io {
  [[nodiscard]] string_view
  description() const override {
    return "`stream`: `getline(std::cin)` -> `std::cout.write()`";
  }

  void
  flush() override {
    cout.flush();
  }

  optional<string>
  readLine() override {
    string ret;
    if (not getline(cin, ret)) {
      if (cin.bad()) {
        process().error(nio::err, EXIT_FAILURE, "Cannot read from `std::cin`");
      }
      return nullopt;
    }
    if (not cin.eof()) {
      ret.push_back('\n');
    }
    return ret;
  }

  [[nodiscard]] u64
  tell() override {
    return io::tell(cin);
  }

  u64
  write(string_view str) override {
    cout.write(str.data(), static_cast<streamsize>(str.size()));
    return cout ? str.size() : 0;
  }
};

// `NioIo` --------------------------------------------------------------------------------------------------

/// Reads from a #rocket::nio::Source, writes to a #rocket::nio::Sink.
struct NioIo : Io {
  NioIo(string description, unique_ptr<nio::Source> source, unique_ptr<nio::Sink> sink) :
    description_(std::move(description)),
    source_(std::move(source)),
    sink_(std::move(sink)) {}

  [[nodiscard]] string_view description() const override { return description_; }

  void
  flush() override {
    sink_->flush();
  }

  optional<string>
  readLine() override {
    auto ret = source_->readln();
    if (source_->bad()) {
      process().error(nio::err, EXIT_FAILURE, "Cannot read from source");
    }
    // `readln` stops at a line feed or at EOF. If it hit EOF, there was no trailing line feed.
    const bool eof = source_->eof();
    if (eof && ret.empty()) {
      return nullopt;
    }
    if (not eof) {
      ret.push_back('\n');
    }
    return ret;
  }

  [[nodiscard]] u64
  tell() override {
    return source_->tell();
  }

  u64
  write(string_view str) override {
    return sink_->write(str);
  }

private:

  string description_;
  unique_ptr<nio::Source> source_;
  unique_ptr<nio::Sink> sink_;
};

// Local functions ------------------------------------------------------------------------------------------

unique_ptr<Io>
makeIo(string_view source) {
  if (source == "file") {
    return make_unique<FileIo>();
  }
  if (source == "stream") {
    return make_unique<StreamIo>();
  }
  if (source == "file-source") {
    return make_unique<NioIo>(
      "`file-source`: `nio::FileSource(stdin).readln()` -> `nio::FileSink(stdout)`",
      make_unique<nio::FileSource>(stdin),
      make_unique<nio::FileSink>(stdout));
  }
  if (source == "stream-source") {
    return make_unique<NioIo>(
      "`stream-source`: `nio::StreamSource(std::cin).readln()` -> `nio::StreamSink(std::cout)`",
      make_unique<nio::StreamSource>(cin),
      make_unique<nio::StreamSink>(cout));
  }
  ROCKET_TERMINATE_UNREACHABLE_CODE();
}

/**
 * Copies a single line from standard input to standard output and flushes standard output.
 *
 * @return the byte counts, or `nullopt` if no more input is available
 */
optional<Counts>
copyLine(Io& io, const Modes& modes) {
  // On Windows, in text mode, `ftell` accounts for the CRs removed from the buffered input. Note that it is
  // known to be unreliable for files with LF-only line endings
  const u64 before = modes.in == Mode::Text ? io.tell() : NPOS;
  auto line = io.readLine();
  if (not line) {
    return nullopt;
  }
  const u64 after = before != NPOS ? io.tell() : NPOS;

  Counts ret;
  ret.read = line->size();
  if (before != NPOS && after != NPOS && after >= before) {
    ret.readRaw = after - before;
  }

  ret.written = io.write(*line);
  if (modes.out == Mode::Text) {
    ret.writtenRaw = ret.written + static_cast<u64>(ranges::count(string_view(*line).substr(0, ret.written), '\n'));
  }
  io.flush(); // Before reporting to `stderr`, so the output appears in order on a terminal

  return ret;
}

/// Reads and writes lines until no more input is available.
void
echo(Io& io, const Modes& modes, bool tty) {
  if (tty) {
#ifdef ROCKET_OS_WINDOWS
    io.write("Enter text, end with Ctrl-Z and Enter:\n");
#else
    io.write("Enter text, end with Ctrl-D:\n");
#endif
    io.flush();
  }

  u64 lines = 0;
  Counts total;
  if (modes.in == Mode::Text) {
    total.readRaw = 0;
  }
  if (modes.out == Mode::Text) {
    total.writtenRaw = 0;
  }
  while (auto counts = copyLine(io, modes)) {
    ++lines;
    nio::err.println("# Line {}: {}", lines, formatCounts(*counts, modes));
    total.add(*counts);
  }
  nio::err.println("# Total, {}: {}", str::noun::line.amount(static_cast<i64>(lines)), formatCounts(total, modes));
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process().init(argc, argv, "read-stdin");

  // Parse command line

  optional<bool> help;
  optional<bool> binary;
  optional<string> source = "file";

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config {
    .usages={ "[OPTION]..." },
    .prolog="Copy standard input to standard output line by line, using a specific I/O method. Information "
      "about the I/O method and the byte counts is written to standard error."
  };
  cl::CommandLine cl({
    cl::Option::help(&general, help),
    cl::Option::custom({
      .description="switch `stdin` and `stdout` to binary mode: no CR/LF conversion, and Ctrl-Z is not "
        "treated as EOF (Windows only, POSIX has no text mode)",
      .group=&general,
      .name="binary",
      .shortName="b"_c
    }, binary),
    cl::Option::custom({
      .choices=set<string>{ "file", "file-source", "stream", "stream-source" },
      .description="read from `stdin` (default), `std::cin`, `nio::FileSource(stdin)`, or "
        "`nio::StreamSource(std::cin)`, and write to the matching output",
      .group=&general,
      .name="source",
      .shortName="s"_c
    }, source)
  }, {}, config);
  cl.parse(process().args());

  // Set up, before any I/O on `stdin` and `stdout`

  if (binary.value_or(false)) {
    setBinaryMode();
  }
  const Modes modes { .in=queryMode(stdin), .out=queryMode(stdout) };
  const bool tty = ROCKET_ISATTY(ROCKET_FILENO(stdin)) != 0;
  auto io = makeIo(source.value());

  // Report

  string_view kind;
  if (tty) {
    kind = "terminal";
  } else if (io::seekable(stdin)) {
    kind = "seekable";
  } else {
    kind = "not seekable (pipe, socket, or device)";
  }
  nio::err.println("# Method: {}", io->description());
  nio::err.println("# stdin:  {}, {}", modeName(modes.in), kind);
  nio::err.println("# stdout: {}", modeName(modes.out));

  // Run

  echo(*io, modes, tty);

  process().exit(EXIT_SUCCESS);
}

// EOF
