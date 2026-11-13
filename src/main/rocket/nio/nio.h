/**
 * @file nio.h
 *
 * New I/O: efficient and convenient sinks and sources.
 */

#pragma once

#include "rocket/format.h"
#include "rocket/unicode/unicode-fwd.h"

#include <fmt/color.h> // `fmt::text_style`

#include <expected>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace rocket::nio {

// Constants ------------------------------------------------------------------------------------------------

/**
 * The default buffer size in bytes.
 */
static constexpr u64 DEFAULT_BUFFER_SIZE = 64 * 1'024; // 64 KiB

// `Device` -------------------------------------------------------------------------------------------------

/**
 * The base class for #rocket::nio::Sink and #rocket::nio::Source.
 *
 * An I/O device, either a sink for output or a source for input.
 */
struct Device {
  /// @ctor_copy
  Device(const Device& rhs) = delete;

  /// @dtor
  virtual ~Device() = default;

  /// @member_op_asgmt_copy
  Device& operator=(const Device& rhs) = delete;

  /**
   * Checks if the device is in an unusable state, e.g., after an I/O error.
   *
   * This is taken from the underlying device, e.g., `ferror` for a `FILE` pointer or `bad()` for a stream.
   *
   * @return if the device is bad
   */
  [[nodiscard]] virtual bool bad() const = 0;

  /**
   * Checks if the device is exhausted: a source has reached the end of its input, a sink has no room for
   * more output. Repositioning may clear this state.
   *
   * This is taken from the underlying device, e.g., `feof` for a `FILE` pointer, `eof()` for a stream, or
   * the position for a contiguous source.
   *
   * @return if the device is exhausted
   */
  [[nodiscard]] virtual bool eof() const = 0;

  /**
   * Returns the handle of the instance.
   *
   * @return the handle of the instance, or -1 if the handle cannot be determined
   */
  [[nodiscard]] virtual i32 handle() const = 0;

protected:

  /// @ctor_default
  Device() = default;
};

// `Sink` ---------------------------------------------------------------------------------------------------

/**
 * Sink base class. An output device that can be written to.
 */
struct Sink : Device {
  /**
   * Flushes the sink.
   *
   * @return if the operation succeeded
   */
  virtual bool flush() = 0;

  /**
   * Prints a formatted message to the sink.
   *
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  template<typename... T>
  bool
  print(fmt::format_string<T...> fmt, T&&... args) {
    return vprint(fmt.get(), fmt::make_format_args(args...));
  }

  /**
   * Prints a formatted message to the sink.
   *
   * @param style the style to use
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  template<typename... T>
  bool
  print(fmt::text_style style, fmt::format_string<T...> fmt, T&&... args) {
    return vprint(style, fmt.get(), fmt::make_format_args(args...));
  }

  /**
   * Prints a formatted message to the sink.
   *
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  template<typename... T>
  bool
  print(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    return vprint(locale, fmt.get(), fmt::make_format_args(args...));
  }

  /**
   * Prints a formatted message and a line feed to the sink.
   *
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  template<typename... T>
  bool
  println(fmt::format_string<T...> fmt, T&&... args) {
    return vprintln(fmt.get(), fmt::make_format_args(args...));
  }

  /**
   * Prints a formatted message and a line feed to the sink.
   *
   * @param style the style to use
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  template<typename... T>
  bool
  println(fmt::text_style style, fmt::format_string<T...> fmt, T&&... args) {
    return vprintln(style, fmt.get(), fmt::make_format_args(args...));
  }

  /**
   * Prints a formatted message and a line feed to the sink.
   *
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  template<typename... T>
  bool
  println(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    return vprintln(locale, fmt.get(), fmt::make_format_args(args...));
  }

  /**
   * Prints a formatted message to the sink. This is the type-erased backend of #print.
   *
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  virtual bool vprint(fmt::string_view fmt, fmt::format_args args) = 0;

  /**
   * Prints a formatted message to the sink. This is the type-erased backend of #print.
   *
   * @param style the style to use
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  virtual bool vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) = 0;

  /**
   * Prints a formatted message to the sink. This is the type-erased backend of #print.
   *
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  virtual bool vprint(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) = 0;

  /**
   * Prints a formatted message and a line feed to the sink. This is the type-erased backend of #println.
   *
   * The default implementation calls #vprint and writes a line feed. Sinks that can print both at once
   * override this.
   *
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  virtual bool vprintln(fmt::string_view fmt, fmt::format_args args);

  /**
   * Prints a formatted message and a line feed to the sink. This is the type-erased backend of #println.
   *
   * The default implementation calls #vprint and writes a line feed. Sinks that can print both at once
   * override this.
   *
   * @param style the style to use
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  virtual bool vprintln(fmt::text_style style, fmt::string_view fmt, fmt::format_args args);

  /**
   * Prints a formatted message and a line feed to the sink. This is the type-erased backend of #println.
   *
   * The default implementation calls #vprint and writes a line feed. Sinks that can print both at once
   * override this.
   *
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments
   * @return if the operation succeeded
   */
  virtual bool vprintln(const std::locale& locale, fmt::string_view fmt, fmt::format_args args);

  /**
   * Writes a single character to the sink.
   *
   * @param val the character
   * @return the number of bytes written
   */
  u64
  write(char val) {
    return write(std::span<const u8>(reinterpret_cast<const u8*>(&val), 1));
  }

  /**
   * Writes a single byte to the sink.
   *
   * @param val the byte
   * @return the number of bytes written
   */
  u64
  write(u8 val) {
    return write(std::span<const u8>(reinterpret_cast<const u8*>(&val), 1));
  }

  /**
   * Writes bytes to the sink.
   *
   * @param in the bytes to write
   * @return the number of bytes written
   */
  virtual u64 write(std::span<const u8> in) = 0;

  /**
   * Writes bytes to the sink
   *
   * @param in the bytes to write
   * @param offset the offset at which to start writing
   * @param n the number of bytes to write
   * @return the number of bytes written
   */
  u64
  write(std::span<const u8> in, u64 offset, u64 n = NPOS) {
    return write(in.subspan(offset, n));
  }

  /**
   * Writes a string to the sink.
   *
   * @param in the string to write
   * @return the number of bytes written
   */
  u64
  write(std::string_view in) {
    const std::span<const u8> span(reinterpret_cast<const u8*>(in.data()), in.size());
    return write(span);
  }

  /**
   * Writes a string to the sink.
   *
   * @param in the string to write
   * @param offset the offset at which to start writing
   * @param n the number of bytes to write
   * @return the number of bytes written
   */
  u64
  write(std::string_view in, u64 offset, u64 n = std::string_view::npos) {
    return write(in.substr(offset, n));
  }

  /**
   * Writes a string and a line feed to the sink.
   *
   * @param in the string to write
   * @return the number of bytes written
   */
  u64 writeln(std::string_view in);

  /**
   * Writes a string and a line feed to the sink.
   *
   * @param in the string to write
   * @param offset the offset at which to start writing
   * @param n the number of bytes to write
   * @return the number of bytes written
   */
  u64
  writeln(std::string_view in, u64 offset, u64 n = std::string_view::npos) {
    return writeln(in.substr(offset, n));
  }

protected:

  /// @ctor
  Sink() = default;
};

// `FileSink` -----------------------------------------------------------------------------------------------

/**
 * A file sink, backed by a `FILE` pointer.
 */
struct FileSink : Sink {
  /**
   * @ctor
   *
   * Constructs a file sink with a `FILE` pointer. The file is *not* closed on destruction.
   *
   * @param file pointer to a `FILE`, may not be null
   */
  explicit FileSink(FILE* file);

  /**
   * @ctor
   *
   * Opens a file at path @p path for writing. The file is closed on destruction.
   *
   * @param path a path to a file
   * @param append whether to append to the file instead of overwriting it
   * @throw InvalidState if the file cannot be opened
   */
  explicit FileSink(const std::filesystem::path& path, bool append = false);

  ~FileSink() override;

  /// @member_op_cast{FILE*}
  explicit operator FILE*() { return file_; } // NOLINT

  /// @member_op_cast{const FILE*}
  explicit operator const FILE*() const { return file_; }

  [[nodiscard]] bool bad() const override;

  [[nodiscard]] bool eof() const override;

  bool flush() override;

  [[nodiscard]] i32 handle() const override;

  bool vprint(fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

  bool vprintln(fmt::string_view fmt, fmt::format_args args) override;

  bool vprintln(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprintln(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

  u64 write(std::span<const u8> in) override;

ROCKET_TEST_PRIVATE:

  FILE* file_; ///< The `FILE` pointer, never null.
  bool closeOnDestroy_; ///< Whether to close the file on destruction.
};

// `NullSink` -----------------------------------------------------------------------------------------------

/**
 * A null sink that never writes anything.
 */
struct NullSink : Sink {
  NullSink() = default;

  ~NullSink() override = default;

  [[nodiscard]] bool bad() const override { return false; }

  [[nodiscard]] bool eof() const override { return true; }

  bool flush() override { return true; }

  [[nodiscard]] i32 handle() const override { return -1; }

  bool vprint(fmt::string_view, fmt::format_args) override { return true; }

  bool vprint(fmt::text_style, fmt::string_view, fmt::format_args) override { return true; }

  bool vprint(const std::locale&, fmt::string_view, fmt::format_args) override { return true; }

  bool vprintln(fmt::string_view, fmt::format_args) override { return true; }

  bool vprintln(fmt::text_style, fmt::string_view, fmt::format_args) override { return true; }

  bool vprintln(const std::locale&, fmt::string_view, fmt::format_args) override { return true; }

  u64 write(std::span<const u8>) override { return 0; }
};

// `SpanSink` -----------------------------------------------------------------------------------------------

/**
 * A sink that writes to a span, i.e., into preallocated memory.
 */
struct SpanSink : Sink {
  /**
   * @ctor
   *
   * @param out the span to write to
   */
  explicit SpanSink(std::span<char> out);

  ~SpanSink() override = default;

  [[nodiscard]] bool bad() const override { return false; }

  [[nodiscard]] bool eof() const override { return pos_ == out_.size(); }

  bool flush() override { return true; }

  [[nodiscard]] i32 handle() const override { return -1; }

  bool vprint(fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

  u64 write(std::span<const u8> in) override;

private:

  std::span<char> out_;
  u64 pos_ = 0;
};

// `StreamSink` ---------------------------------------------------------------------------------------------

/**
 * The class #StreamSink provides support for #std::ostream.
 */
struct StreamSink : Sink {
  /**
   * @ctor
   *
   * @param os the output stream to write to
   */
  explicit StreamSink(std::ostream& os);

  ~StreamSink() override;

  [[nodiscard]] bool bad() const override { return os_.bad(); }

  [[nodiscard]] bool eof() const override { return os_.eof(); }

  bool flush() override;

  [[nodiscard]] i32 handle() const override;

  bool vprint(fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

  bool vprintln(fmt::string_view fmt, fmt::format_args args) override;

  bool vprintln(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprintln(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

  u64 write(std::span<const u8> in) override;

private:

  std::ostream& os_;
};

// `StringSink` ---------------------------------------------------------------------------------------------

/**
 * A sink that appends to a string.
 *
 * If the sink is constructed without a string reference, it holds an owned string that can be accessed via
 * #rocket::nio::StringSink::str.
 */
struct StringSink : Sink {
  /**
   * Constructs a new #StringSink with an owned string.
   */
  explicit StringSink() = default;

  /**
   * Constructs a new #StringSink with a string reference and no owned string.
   *
   * @param ref the string to write to. The reference must remain valid for the lifetime of the #StringSink
   */
  explicit StringSink(std::string& ref);

  ~StringSink() override = default;

  [[nodiscard]] bool bad() const override { return false; }

  [[nodiscard]] bool eof() const override { return false; }

  bool flush() override { return true; }

  [[nodiscard]] i32 handle() const override { return -1; }

  /**
   * Returns a reference to the string (either referenced or owned).
   *
   * @return the referenced or the owned string
   */
  [[nodiscard]] const std::string& str() const { return ptr_ != nullptr ? *ptr_ : owned_; }

  bool vprint(fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

  u64 write(std::span<const u8> in) override;

private:

  /// Returns the string to write to (either referenced or owned).
  std::string& target() { return ptr_ != nullptr ? *ptr_ : owned_; }

  std::string* ptr_ = nullptr;
  std::string owned_;
};

// `SeekMode` -----------------------------------------------------------------------------------------------

/**
 * The seek mode for #rocket::nio::Source#seek.
 */
enum class SeekMode : u8 {
  beg, ///< Seek relative to the beginning of the source
  cur, ///< Seek relative to the current position of the source
  end ///< Seek relative to the end of the source
};

// `Source` -------------------------------------------------------------------------------------------------

/**
 * Source base class. An input device that can be read from.
 */
struct Source : Device {
  /**
   * Provides #std::istream interoperability.
   *
   * @return a reference to an input stream
   */
  [[nodiscard]] virtual std::istream& istream() = 0;

  /**
   * Returns the next character without consuming it.
   *
   * Unlike reading and seeking back, this also works with sources that cannot seek, such as pipes.
   *
   * @return the next character, or `nullopt` if no character is available
   */
  virtual std::optional<char> peek() = 0;

  /**
   * Reads a single character from a source.
   *
   * @param out the character to read
   * @return the number of bytes read
   */
  u64 read(char& out) { return read(std::span<u8>(reinterpret_cast<u8*>(&out), 1)); }

  /**
   * Reads as many bytes as available into a span.
   *
   * @param out the span to read into
   * @return the number of bytes read
   */
  virtual u64 read(std::span<u8> out) = 0;

  /**
   * Reads as many characters as available into a string.
   *
   * @param out the string to read into
   * @return the number of bytes read
   */
  u64 read(std::string& out) { return read(std::span<u8>(reinterpret_cast<u8*>(out.data()), out.size())); }

  /**
   * Reads a line from a source into a string.
   *
   * A trailing @c '\\r' is removed if it precedes a @c '\\n'.
   *
   * @return the line read, not containing the trailing @c '\\r' or @c '\\n'
   */
  virtual std::string readln() = 0;

  /**
   * Reads all available bytes from a source into a vector.
   *
   * @return the bytes read
   */
  std::vector<u8> readAll();

  /**
   * Reads all available characters from a source into a string.
   *
   * @return the string read
   */
  std::string readString();

  /**
   * Seeks a new position in the source, using #rocket::nio::SeekMode::beg.
   *
   * @param offset the offset to seek to
   * @return if the operation succeeded
   */
  bool seek(i64 offset) { return seek(offset, SeekMode::beg); }

  /**
   * Seeks a new position in the source.
   *
   * @param offset the offset to seek to
   * @param mode the seek mode
   * @return if the operation succeeded
   */
  virtual bool seek(i64 offset, SeekMode mode) = 0;

  /**
   * Returns the current input position
   *
   * @return the current input position, or #rocket::NPOS if the position cannot be determined
   */
  [[nodiscard]] virtual u64 tell() = 0;

protected:

  /// @ctor_default
  Source() = default;
};

// `ContiguousSource` ---------------------------------------------------------------------------------------

/**
 * A contiguous source.
 */
struct ContiguousSource : Source {
  ~ContiguousSource() override = default;

  [[nodiscard]] bool bad() const final { return false; }

  /**
   * Returns the bytes available in the contiguous source.
   *
   * @return the bytes available in the contiguous source
   */
  [[nodiscard]] virtual std::span<const u8> bytes() const = 0;

  [[nodiscard]] bool eof() const override { return bytes().empty(); }

  std::optional<char> peek() override;

  std::string readln() override;

  /**
   * Returns the string available in the contiguous source.
   *
   * @return the string available in the contiguous source
   */
  [[nodiscard]] virtual std::string_view str() const = 0;

protected:

  /// @ctor_default
  ContiguousSource() = default;
};

// `FileSource` ---------------------------------------------------------------------------------------------

/**
 * A file source, backed by a `FILE` pointer.
 */
struct FileSource : Source {
  /**
   * @ctor
   *
   * Constructs a file source with a `FILE` pointer. The file is *not* closed on destruction.
   *
   * @param file pointer to a `FILE`, may not be null
   */
  explicit FileSource(FILE* file);

  /**
   * @ctor
   *
   * Opens a file at path @p path for reading. The file is closed on destruction.
   *
   * @param path a path to a file
   * @throw InvalidState if the file cannot be opened
   */
  explicit FileSource(const std::filesystem::path& path);

  ~FileSource() override;

  [[nodiscard]] bool bad() const override;

  [[nodiscard]] bool eof() const override;

  [[nodiscard]] i32 handle() const override;

  std::istream& istream() override;

  std::optional<char> peek() override;

  u64 read(std::span<u8> out) override;

  std::string readln() override;

  bool seek(i64 offset, SeekMode mode) override;

  [[nodiscard]] u64 tell() override;

ROCKET_TEST_PRIVATE:

  FILE* file_; ///< The `FILE` pointer, never null.
  bool closeOnDestroy_; ///< Whether to close the file on destruction.
  std::unique_ptr<std::istream> istream_; ///< An optional input stream, reading through #file_.
};

// `NullSource` ---------------------------------------------------------------------------------------------

/**
 * A null source that never reads anything.
 */
struct NullSource : Source {
  NullSource() = default;

  ~NullSource() override = default;

  [[nodiscard]] bool bad() const override { return false; }

  [[nodiscard]] bool eof() const override { return true; }

  [[nodiscard]] i32 handle() const override { return -1; }

  std::istream& istream() override;

  std::optional<char> peek() override { return std::nullopt; }

  u64 read(std::span<u8>) override { return 0; }

  std::string readln() override { return {}; }

  bool seek(i64, SeekMode) override { return false; }

  [[nodiscard]] u64 tell() override { return NPOS; }

private:

  std::unique_ptr<std::istream> istream_;
};

// `SpanSource` ---------------------------------------------------------------------------------------------

/**
 * A source that reads from a span.
 */
struct SpanSource : ContiguousSource {
  /// @ctor_default
  SpanSource() : SpanSource(std::span<const u8>()) {}

  /**
   * @ctor
   *
   * @param in the span to read from
   */
  explicit SpanSource(std::span<const u8> in);

  ~SpanSource() override = default;

  [[nodiscard]] std::span<const u8> bytes() const override { return in_.subspan(pos_); }

  [[nodiscard]] i32 handle() const override { return -1; }

  [[nodiscard]] std::istream& istream() override;

  /**
   * Returns the input span the source was constructed with.
   *
   * @return the input span that the source was constructed with
   */
  [[nodiscard]] std::span<const u8> in() const { return in_; }

  u64 read(std::span<u8> out) override;

  bool seek(i64 offset, SeekMode mode) override;

  [[nodiscard]] std::string_view
  str() const override {
    const auto bytes = this->bytes();
    return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
  }

  [[nodiscard]] u64 tell() override { return pos_; }

private:

  std::span<const u8> in_;
  u64 pos_ = 0;
  std::unique_ptr<std::istream> istream_;
};

// `StreamSource` -------------------------------------------------------------------------------------------

/**
 * The class #StreamSource provides support for #std::istream.
 */
struct StreamSource : Source {
  /**
   * @ctor
   *
   * @param is the input stream to read from
   */
  explicit StreamSource(std::istream& is);

  [[nodiscard]] bool bad() const override { return is_.bad(); }

  [[nodiscard]] bool eof() const override { return is_.eof(); }

  [[nodiscard]] i32 handle() const override;

  std::istream& istream() override { return is_; }

  std::optional<char> peek() override;

  u64 read(std::span<u8> out) override;

  std::string readln() override;

  bool seek(i64 offset, SeekMode mode) override;

  [[nodiscard]] u64 tell() override;

private:

  std::istream& is_;
};

// `StringSource` -------------------------------------------------------------------------------------------

/**
 * A source that reads from a string.
 */
struct StringSource : ContiguousSource {
  /// @ctor_default
  StringSource() : StringSource(std::string_view()) {}

  /**
   * @ctor
   *
   * @param in the string to read from
   */
  explicit StringSource(std::string_view in);

  ~StringSource() override = default;

  [[nodiscard]] std::span<const u8>
  bytes() const override {
    const auto str = this->str();
    return { reinterpret_cast<const u8*>(str.data()), str.size() };
  }

  [[nodiscard]] i32 handle() const override { return -1; }

  /**
   * Returns the input string the source was constructed with.
   *
   * @return the input string that the source was constructed with
   */
  [[nodiscard]] std::string_view in() const { return in_; }

  [[nodiscard]] std::istream& istream() override;

  u64 read(std::span<u8> out) override;

  bool seek(i64 offset, SeekMode mode) override;

  [[nodiscard]] std::string_view str() const override { return in_.substr(pos_); }

  [[nodiscard]] u64 tell() override { return pos_; }

private:

  std::string_view in_;
  u64 pos_ = 0;
  std::unique_ptr<std::istream> istream_;
};

// Variables ------------------------------------------------------------------------------------------------

/// The standard input source.
ROCKET_PUBLIC extern Source& in; // NOLINT
/// The standard output sink.
ROCKET_PUBLIC extern Sink& out; // NOLINT
/// The standard error sink.
ROCKET_PUBLIC extern Sink& err; // NOLINT

} // namespace rocket::nio

// EOF
