/**
 * @file nio.h
 *
 * New I/O: efficient and convenient sinks and sources.
 */

#pragma once

#include "rocket/rocket.h"
#include "rocket/io/io.h"

#include <fmt/color.h> // `fmt::text_style`

#include <scn/ranges.h>

#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rocket::nio {

// Constants ------------------------------------------------------------------------------------------------

/// The default buffer size in bytes.
constexpr u64 DEFAULT_BUFFER_SIZE = 16 * 1'024; // 16 KiB

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
   * Returns the error code of the device: the I/O error that made the device #bad.
   *
   * The code is set when an operation makes the device bad, or at construction if the device is bad already.
   * It stays set while the device is bad. #rocket::nio::Source#seek restores a bad source, which clears the
   * code. A device that is not bad, and was not made bad by some other user of the underlying device, has an
   * empty code.
   *
   * @return the error code of the device
   */
  [[nodiscard]] const std::error_code& code() const { return ec_; }

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

  /**
   * Checks if the position in the device can be changed.
   *
   * For a source, this tells if #rocket::nio::Source#seek and #rocket::nio::Source#tell work.
   * Pipes, sockets, and terminals are not seekable.
   *
   * This is determined once, at construction, as this may take a system call.
   *
   * @return if the device is seekable
   */
  [[nodiscard]] bool seekable() const { return seekable_; }

protected:

  std::error_code ec_; ///< The error code of the device, see #code.
  bool seekable_ = false; ///< Whether the device is seekable, see #seekable.

  /// @ctor_default
  Device() = default;
};

// `Sink` ---------------------------------------------------------------------------------------------------

/**
 * Sink base class. An output device that can be written to.
 *
 * A subclass that overrides #write must also declare `using Sink::write;`, or the other overloads of #write
 * are hidden in the subclass.
 */
struct Sink : Device {
  /**
   * Flushes the sink.
   *
   * @return if the operation succeeded
   */
  virtual bool flush() = 0;

  // `fmt::make_format_args` binds its arguments to lvalue references, so `args` cannot be forwarded
  // NOLINTBEGIN(cppcoreguidelines-missing-std-forward)

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
    return vprint(fmt.get(), fmt::make_format_args(args...)) && write('\n') == 1;
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
    return vprint(style, fmt.get(), fmt::make_format_args(args...)) && write('\n') == 1;
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
    return vprint(locale, fmt.get(), fmt::make_format_args(args...)) && write('\n') == 1;
  }

  // NOLINTEND(cppcoreguidelines-missing-std-forward)

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
   * Writes bytes to the sink.
   *
   * @param in the bytes to write
   * @param offset the offset at which to start writing
   * @param n the number of bytes to write. Fewer are written if @p in ends before
   * @return the number of bytes written
   * @throw #rocket::InvalidArgument if @p offset is greater than the size of @p in
   */
  u64 write(std::span<const u8> in, u64 offset, u64 n = NPOS);

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
   * @param n the number of bytes to write. Fewer are written if @p in ends before
   * @return the number of bytes written
   * @throw #rocket::InvalidArgument if @p offset is greater than the size of @p in
   */
  u64 write(std::string_view in, u64 offset, u64 n = NPOS);

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
   * @param n the number of bytes to write. Fewer are written if @p in ends before
   * @return the number of bytes written
   * @throw #rocket::InvalidArgument if @p offset is greater than the size of @p in
   */
  u64 writeln(std::string_view in, u64 offset, u64 n = NPOS);

protected:

  /// @ctor_default
  Sink() = default;

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
};

// `FileSink` -----------------------------------------------------------------------------------------------

/**
 * A file sink, backed by a `FILE` pointer.
 *
 * Whether the file is seekable is determined once, at construction, as this takes a system call. If the
 * file is reopened afterward, e.g., with `freopen`, it is still considered seekable or not as before.
 */
struct FileSink final : Sink {
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
   * @param binary whether to open the file in binary or text mode
   * @throw #rocket::io::IoError if the file cannot be opened
   */
  explicit FileSink(const std::filesystem::path& path, bool append = false, bool binary = true);

  ~FileSink() override;

  /// @member_op_cast{FILE*}
  explicit operator FILE*() { return file_; } // NOLINT

  /// @member_op_cast{const FILE*}
  explicit operator const FILE*() const { return file_; }

  [[nodiscard]] bool bad() const override;

  [[nodiscard]] bool eof() const override;

  bool flush() override;

  [[nodiscard]] i32 handle() const override;

  using Sink::write;

  u64 write(std::span<const u8> in) override;

protected:

  bool vprint(fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

ROCKET_TEST_PRIVATE:

  FILE* file_; ///< The `FILE` pointer, never null.
  bool closeOnDestroy_; ///< Whether to close the file on destruction.
};

// `NullSink` -----------------------------------------------------------------------------------------------

/**
 * A null sink that discards everything written to it, like `/dev/null`.
 *
 * Every operation succeeds: a write reports all bytes as written, and the sink is never bad or exhausted.
 */
struct NullSink : Sink {
  NullSink() = default;

  ~NullSink() override = default;

  [[nodiscard]] bool bad() const override { return false; }

  [[nodiscard]] bool eof() const override { return false; }

  bool flush() override { return true; }

  [[nodiscard]] i32 handle() const override { return -1; }

  using Sink::write;

  u64 write(std::span<const u8> in) override { return in.size(); }

protected:

  bool vprint(fmt::string_view /*fmt*/, fmt::format_args /*args*/) override { return true; }

  bool vprint(fmt::text_style /*style*/, fmt::string_view /*fmt*/, fmt::format_args /*args*/) override { return true; }

  bool vprint(const std::locale& /*locale*/, fmt::string_view /*fmt*/, fmt::format_args /*args*/) override { return true; }
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

  using Sink::write;

  u64 write(std::span<const u8> in) override;

protected:

  bool vprint(fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

private:

  std::span<char> out_;
  u64 pos_ = 0;
};

// `StreamSink` ---------------------------------------------------------------------------------------------

/**
 * The class #StreamSink provides support for #std::ostream.
 *
 * Whether the stream is seekable is determined once, at construction, as this may take a system call. If the
 * stream buffer is replaced afterward, e.g., with `rdbuf`, the stream is still considered seekable or not as
 * before.
 *
 * The exception mask of the stream is left alone. Whatever it is, the sink does not throw on I/O errors, nor
 * for any state bit in the mask: an exception from the stream buffer is an I/O error, which makes the sink
 * #bad and sets its #code.
 */
struct StreamSink final : Sink {
  /**
   * @ctor
   *
   * @param os the output stream to write to
   */
  explicit StreamSink(std::ostream& os);

  /**
   * @ctor
   *
   * Opens a file at path @p path for writing. The file is closed on destruction.
   *
   * @param path a path to a file
   * @param append whether to append to the file instead of overwriting it
   * @param binary whether to open the file in binary or text mode
   * @throw #rocket::io::IoError if the file cannot be opened
   */
  explicit StreamSink(const std::filesystem::path& path, bool append = false, bool binary = true);

  ~StreamSink() override;

  [[nodiscard]] bool bad() const override { return out().bad(); }

  [[nodiscard]] bool eof() const override { return out().eof(); }

  bool flush() override;

  [[nodiscard]] i32 handle() const override;

  /**
   * Returns a reference to the stream to write to (either referenced or owned).
   *
   * @return the output stream
   */
  [[nodiscard]] std::ostream&
  out() {
    return ptr_ != nullptr ? *ptr_ : static_cast<std::ostream&>(owned_);
  }

  /**
   * Returns a reference to the stream to write to (either referenced or owned).
   *
   * @return the output stream
   */
  [[nodiscard]] const std::ostream&
  out() const {
    return ptr_ != nullptr ? *ptr_ : static_cast<const std::ostream&>(owned_);
  }

  using Sink::write;

  u64 write(std::span<const u8> in) override;

protected:

  bool vprint(fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

private:

  std::ostream* ptr_ = nullptr;
  std::ofstream owned_;
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
  explicit StringSink();

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
  [[nodiscard]] const std::string& str() const { return out(); }

  using Sink::write;

  u64 write(std::span<const u8> in) override;

protected:

  bool vprint(fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(fmt::text_style style, fmt::string_view fmt, fmt::format_args args) override;

  bool vprint(const std::locale& locale, fmt::string_view fmt, fmt::format_args args) override;

private:

  [[nodiscard]] std::string& out() { return ptr_ != nullptr ? *ptr_ : owned_; }

  [[nodiscard]] const std::string& out() const { return ptr_ != nullptr ? *ptr_ : owned_; }

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

// `ScanFormatString` ---------------------------------------------------------------------------------------

/**
 * The format string type for #rocket::nio::Source#scan.
 *
 * A source is not contiguous in general, so the format string is checked as for a noncontiguous source. This
 * rejects scanning a #std::string_view at compile time.
 *
 * @tparam T the types to scan
 */
template<typename... T>
using ScanFormatString = scn::scan_format_string<scn::scan_file&, T...>;

// `Source` -------------------------------------------------------------------------------------------------

/**
 * Source base class. An input device that can be read from.
 *
 * A subclass that overrides #read or #seek must also declare `using Source::read;` or `using Source::seek;`,
 * or the other overloads are hidden in the subclass.
 */
struct Source : Device {
  /**
   * Returns the next character without consuming it.
   *
   * Unlike reading and seeking back, this also works with sources that cannot seek, such as pipes.
   *
   * @return the next character, or null if no character is available
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
   * Reads characters into a string, as many as fit into its current size.
   *
   * The string is not resized: it is filled from the start with at most `out.size()` characters, and
   * characters beyond the number read are left unchanged. To read all available characters, use #readString.
   *
   * @param out the string to read into
   * @return the number of bytes read
   */
  u64 read(std::string& out) { return read(std::span<u8>(reinterpret_cast<u8*>(out.data()), out.size())); }

  /**
   * Reads a line from a source into a string.
   *
   * A trailing @c '\\r' is removed if it precedes a @c '\\n'. The last line need not end with a @c '\\n'.
   *
   * On an I/O error, the result is null, even if part of a line was read, and the source is bad.
   *
   * @return the line read, not containing the trailing @c '\\r' or @c '\\n', or null if no more input is
   *   available or an I/O error occurred
   */
  virtual std::optional<std::string> readln() = 0;

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
   * Scans values from the source, using `scnlib`.
   *
   * The source is advanced past the scanned input. The position of the remaining input is the position of
   * the source, so the result holds no range, unlike the result of `scn::scan`.
   *
   * @tparam T the types to scan
   * @param fmt the format string
   * @return the scanned values, or the error
   */
  template<typename... T>
  [[nodiscard]] scn::scan_expected<scn::scan_result<scn::ranges::dangling, T...>>
  scan(ScanFormatString<T...> fmt) {
    scn::scan_result<scn::ranges::dangling, T...> ret;
    if (const auto result = vscan(fmt.get(), scn::make_scan_args(ret.values())); not result) {
      return scn::unexpected(result.error());
    }
    return ret;
  }

  /**
   * Scans values from the source, using `scnlib` and a locale.
   *
   * The locale only applies to arguments with the `L` format specifier. See #scan.
   *
   * @tparam T the types to scan
   * @param locale the locale
   * @param fmt the format string
   * @return the scanned values, or the error
   */
  template<typename... T>
  [[nodiscard]] scn::scan_expected<scn::scan_result<scn::ranges::dangling, T...>>
  scan(const std::locale& locale, ScanFormatString<T...> fmt) {
    scn::scan_result<scn::ranges::dangling, T...> ret;
    if (const auto result = vscan(locale, fmt.get(), scn::make_scan_args(ret.values())); not result) {
      return scn::unexpected(result.error());
    }
    return ret;
  }

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
   * Returns the current input position.
   *
   * @return the current input position, or an empty optional if the position cannot be determined
   */
  [[nodiscard]] virtual std::optional<u64> tell() = 0;

protected:

  /**
   * Scans values from the source. This is the type-erased backend of #scan.
   *
   * @param fmt the format string
   * @param args the arguments to scan into
   * @return nothing, or the error
   */
  virtual scn::scan_expected<void> vscan(
    std::string_view fmt, scn::scan_args args) = 0;

  /**
   * Scans values from the source, using a locale. This is the type-erased backend of #scan.
   *
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments to scan into
   * @return nothing, or the error
   */
  virtual scn::scan_expected<void> vscan(
    const std::locale& locale, std::string_view fmt, scn::scan_args args) = 0;

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

  /**
   * Returns the current input position. Unlike #rocket::nio::Source#tell, this cannot fail, because a
   * contiguous source always has a position.
   *
   * @return the current input position
   */
  [[nodiscard]] virtual u64 position() const = 0;

  std::optional<std::string> readln() override;

  [[nodiscard]] std::optional<u64> tell() final { return position(); }

  /**
   * Returns the string available in the contiguous source.
   *
   * @return the string available in the contiguous source
   */
  [[nodiscard]] virtual std::string_view str() const = 0;

protected:

  scn::scan_expected<void> vscan(
    std::string_view fmt, scn::scan_args args) override;

  scn::scan_expected<void> vscan(
    const std::locale& locale, std::string_view fmt, scn::scan_args args) override;

  /// @ctor_default
  ContiguousSource() { seekable_ = true; }
};

// `FileSource` ---------------------------------------------------------------------------------------------

/**
 * A file source, backed by a `FILE` pointer.
 *
 * Whether the file is seekable is determined once, at construction, as this takes a system call. If the
 * file is reopened afterward, e.g., with `freopen`, it is still considered seekable or not as before.
 */
struct FileSource final : Source {
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
   * @param binary whether to open the file in binary or text mode
   * @throw #rocket::io::IoError if the file cannot be opened
   */
  explicit FileSource(const std::filesystem::path& path, bool binary = true);

  ~FileSource() override;

  [[nodiscard]] bool bad() const override;

  [[nodiscard]] bool eof() const override;

  [[nodiscard]] i32 handle() const override;

  std::optional<char> peek() override;

  using Source::read;

  u64 read(std::span<u8> out) override;

  std::optional<std::string> readln() override;

  using Source::seek;

  bool seek(i64 offset, SeekMode mode) override;

  [[nodiscard]] std::optional<u64> tell() override;

protected:

  scn::scan_expected<void> vscan(
    std::string_view fmt, scn::scan_args args) override;

  scn::scan_expected<void> vscan(
    const std::locale& locale, std::string_view fmt, scn::scan_args args) override;

ROCKET_TEST_PRIVATE:

  FILE* file_; ///< The `FILE` pointer, never null.
  bool closeOnDestroy_; ///< Whether to close the file on destruction.
};

// `NullSource` ---------------------------------------------------------------------------------------------

/**
 * A null source that is always at the end of its input, like `/dev/null`.
 *
 * Reading yields nothing, and scanning fails unless the format string expects nothing. As with `/dev/null`,
 * seeking always succeeds, to any offset in any mode, and the position always stays 0.
 */
struct NullSource : Source {
  NullSource() { seekable_ = true; }

  ~NullSource() override = default;

  [[nodiscard]] bool bad() const override { return false; }

  [[nodiscard]] bool eof() const override { return true; }

  [[nodiscard]] i32 handle() const override { return -1; }

  std::optional<char> peek() override { return std::nullopt; }

  using Source::read;

  u64 read(std::span<u8> /*out*/) override { return 0; }

  std::optional<std::string> readln() override { return std::nullopt; }

  using Source::seek;

  bool seek(i64 /*offset*/, SeekMode /*mode*/) override { return true; }

  [[nodiscard]] std::optional<u64> tell() override { return 0; }

protected:

  scn::scan_expected<void> vscan(
    std::string_view fmt, scn::scan_args args) override;

  scn::scan_expected<void> vscan(const std::locale& locale,
    std::string_view fmt, scn::scan_args args) override;
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

  /**
   * Returns the input span the source was constructed with.
   *
   * @return the input span that the source was constructed with
   */
  [[nodiscard]] std::span<const u8> in() const { return in_; }

  using Source::read;

  u64 read(std::span<u8> out) override;

  using Source::seek;

  /**
   * Seeks a new position in the source. A position before the beginning or beyond the end is out of range;
   * then, the position is left unchanged.
   *
   * @param offset the offset to seek to
   * @param mode the seek mode
   * @return if the position was in range
   */
  bool seek(i64 offset, SeekMode mode) override;

  [[nodiscard]] std::string_view
  str() const override {
    const auto bytes = this->bytes();
    return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
  }

  [[nodiscard]] u64 position() const override { return pos_; }

private:

  std::span<const u8> in_;
  u64 pos_ = 0;
};

// `StreamSource` -------------------------------------------------------------------------------------------

/**
 * The class #StreamSource provides support for #std::istream.
 *
 * Whether the stream is seekable is determined once, at construction, as this may take a system call. If the
 * stream buffer is replaced afterward, e.g., with `rdbuf`, the stream is still considered seekable or not as
 * before.
 *
 * The exception mask of the stream is left alone. Whatever it is, the source does not throw on I/O errors,
 * nor for any state bit in the mask: an exception from the stream buffer is an I/O error, which makes the
 * source #bad and sets its #code.
 */
struct StreamSource final : Source {
  /**
   * @ctor
   *
   * @param is the input stream to read from
   */
  explicit StreamSource(std::istream& is);

  /**
   * @ctor
   *
   * @param path a path to a file
   * @param binary whether to open the file in binary or text mode
   * @throw #rocket::io::IoError if the file cannot be opened
   */
  explicit StreamSource(const std::filesystem::path& path, bool binary = true);

  [[nodiscard]] bool bad() const override { return in().bad(); }

  [[nodiscard]] bool eof() const override { return in().eof(); }

  [[nodiscard]] i32 handle() const override;

  /**
   * Returns a reference to the input stream (either referenced or owned).
   *
   * @return the input stream
   */
  [[nodiscard]] std::istream&
  in() {
    return ptr_ != nullptr ? *ptr_ : static_cast<std::istream&>(owned_);
  }

  /**
   * Returns a reference to the input stream (either referenced or owned).
   *
   * @return the input stream
   */
  [[nodiscard]] const std::istream&
  in() const {
    return ptr_ != nullptr ? *ptr_ : static_cast<const std::istream&>(owned_);
  }

  std::optional<char> peek() override;

  using Source::read;

  u64 read(std::span<u8> out) override;

  std::optional<std::string> readln() override;

  using Source::seek;

  bool seek(i64 offset, SeekMode mode) override;

  [[nodiscard]] std::optional<u64> tell() override;

protected:

  scn::scan_expected<void> vscan(
    std::string_view fmt, scn::scan_args args) override;

  scn::scan_expected<void> vscan(
    const std::locale& locale, std::string_view fmt, scn::scan_args args) override;

private:

  std::istream* ptr_ = nullptr;
  std::ifstream owned_;
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

  using Source::read;

  u64 read(std::span<u8> out) override;

  using Source::seek;

  /**
   * Seeks a new position in the source. A position before the beginning or beyond the end is out of range;
   * then, the position is left unchanged.
   *
   * @param offset the offset to seek to
   * @param mode the seek mode
   * @return if the position was in range
   */
  bool seek(i64 offset, SeekMode mode) override;

  [[nodiscard]] std::string_view str() const override { return in_.substr(pos_); }

  [[nodiscard]] u64 position() const override { return pos_; }

private:

  std::string_view in_;
  u64 pos_ = 0;
};

// Variables ------------------------------------------------------------------------------------------------

/// The source connected to stdin.
ROCKET_PUBLIC extern Source& in; // NOLINT
/// The sink connected to stdout.
ROCKET_PUBLIC extern Sink& out; // NOLINT
/// The sink connected to stderr.
ROCKET_PUBLIC extern Sink& err; // NOLINT

#ifdef ROCKET_TEST

namespace testing {

// `FailingSource` ------------------------------------------------------------------------------------------

/// A #rocket::nio::StreamSource over a #rocket::io::testing::FailingBuf. The stream sets `badbit` when the
/// buffer fails.
struct FailingSource {
  io::testing::FailingBuf buf; ///< The stream buffer.
  std::istream is; ///< The stream over #buf.
  StreamSource in; ///< The source over #is.

  /**
   * @ctor
   *
   * @param str the string to read from
   */
  explicit FailingSource(const std::string& str) : buf(str), is(&buf), in(is) {}
};

// `NonseekableSource` -------------------------------------------------------------------------------------

/// A #rocket::nio::StreamSource that cannot seek or report its position, like one over a pipe.
struct NonseekableSource {
  io::testing::NonseekableBuf buf; ///< The stream buffer.
  std::istream is; ///< The stream over #buf.
  StreamSource in; ///< The source over #is.

  /**
   * @ctor
   *
   * @param str the string to read from
   */
  explicit NonseekableSource(const std::string& str) : buf(str), is(&buf), in(is) {}
};

} // namespace testing

#endif // ROCKET_TEST

} // namespace rocket::nio

// EOF
