/**
 * @file nio.h
 *
 * New I/O: efficient and convenient sinks and sources.
 */

#pragma once

#include "rocket/rocket.h"
#include "rocket/unicode/unicode-fwd.h"

#include <boost/safe_numerics/safe_integer.hpp>

#include <fmt/color.h>
#include <fmt/format.h>

#include <iosfwd>
#include <memory>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

namespace rocket::nio {

// Constants ------------------------------------------------------------------------------------------------

/**
 * The default buffer size in bytes.
 */
static constexpr u64 DEFAULT_BUFFER_SIZE = 64 * 1'024; // 64 KiB
/**
 * The minimum buffer size in bytes.
 */
static constexpr u64 MIN_BUFFER_SIZE = 64;

// `Status` -------------------------------------------------------------------------------------------------

/// The status of an I/O instance.
struct Status {
  /**
   * If #bad is set, the I/O is in an unusable state.
   */
  unsigned bad : 1;
  /**
   * If #eof is set, the I/O is exhausted.
   *
   * Subsequent operations, such as repositioning, may clear this bit.
   */
  unsigned eof : 1;
};

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

  /**
   * Checks if the bad bit is set.
   *
   * @return whether the bad bit is set
   */
  [[nodiscard]] bool bad() const { return status_.bad; }

  /**
   * Closes the instance.
   *
   * @return whether the operation succeeded
   */
  virtual bool close() = 0;

  /**
   * Checks if the EOF bit is set.
   *
   * @return whether the EOF bit is set
   */
  [[nodiscard]] bool eof() const { return status_.eof; }

  /**
   * Returns the handle of the instance.
   *
   * @return the handle of the instance, or -1 if the handle cannot be determined
   */
  [[nodiscard]] virtual i32 handle() const = 0;

  /**
   * Returns the status.
   *
   * @return the status
   */
  [[nodiscard]] Status status() const { return status_; }

protected:

  /**
   * The status of the instance.
   *
   * When an instance is constructed, the #bad bit is set, meaning the stream hasn't been opened yet and is
   * not ready to use. Derived classes must unset the #bad bit to mark the instance as usable.
   */
  Status status_ { .bad=1, .eof=0 };

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
   * @return whether the operation succeeded
   */
  virtual bool flush() = 0;

  /**
   * Prints a formatted message to the sink.
   *
   * @param fmt the format string
   * @param args the arguments
   * @return the number of bytes written
   */
  template<typename... T>
  u64
  print(fmt::format_string<T...> fmt, T&&... args) {
    auto formatted = fmt::format(fmt, std::forward<T>(args)...);
    return write(formatted);
  }

  /**
   * Prints a formatted message to the sink.
   *
   * @param style the style to use
   * @param fmt the format string
   * @param args the arguments
   * @return the number of bytes written
   */
  template<typename... T>
  u64
  print(fmt::text_style style, fmt::format_string<T...> fmt, T&&... args) {
    auto formatted = fmt::format(style, fmt, std::forward<T>(args)...);
    return write(formatted);
  }

  /**
   * Prints a formatted message to the sink.
   *
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments
   * @return the number of bytes written
   */
  template<typename... T>
  u64
  print(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    auto formatted = fmt::format(locale, fmt, std::forward<T>(args)...);
    return write(formatted);
  }

  /**
   * Prints a formatted message and a line feed to the sink.
   *
   * @param fmt the format string
   * @param args the arguments
   * @return the number of bytes written
   */
  template<typename... T>
  u64
  println(fmt::format_string<T...> fmt, T&&... args) {
    auto ret = print(fmt, std::forward<T>(args)...);
    ret += write('\n');
    flush();
    return ret;
  }

  /**
   * Prints a formatted message and a line feed to the sink.
   *
   * @param locale the locale
   * @param fmt the format string
   * @param args the arguments
   * @return the number of bytes written
   */
  template<typename... T>
  u64
  println(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    auto ret = print(locale, fmt, std::forward<T>(args)...);
    ret += write('\n');
    flush();
    return ret;
  }

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

// `BufferedSink` -------------------------------------------------------------------------------------------

/**
 * A buffered sink that may be attached to another sink.
 *
 * @note Applying an additional buffer only makes sense if the underlying sink isn't already buffered.
 */
struct BufferedSink : Sink {
  /**
   * @ctor
   *
   * @param underlying the underlying sink
   * @param size the size of the buffer
   */
  explicit BufferedSink(Sink& underlying, u64 size = DEFAULT_BUFFER_SIZE);

  ~BufferedSink() override;

  bool close() override;

  bool flush() override;

  [[nodiscard]] i32 handle() const override { return underlying_.handle(); }

  u64 write(std::span<const u8> in) override;

ROCKET_TEST_PRIVATE:

  Sink& underlying_; ///< The underlying sink.
  u64 size_; ///< The size of the buffer.
  /// The buffer.
  std::unique_ptr<u8[]> buf_; // NOLINT
  u64 pos_ = 0; ///< The current position in the buffer.

  /// Flushes the buffer to the underlying sink.
  void flushBuffer();
};

// `FileSink` -----------------------------------------------------------------------------------------------

/**
 * A file sink, backed by a `FILE` pointer.
 */
struct FileSink : Sink {
  /**
   * Configuration for the #FileSink constructor.
   */
  struct Config {
    /**
     * Whether to append to the file instead of overwriting it.
     */
    bool append = false;
    /**
     * Whether to close the file on destruction.
     *
     * The default is `true`. For `stdout` and `stderr`, this is automatically configured to be `false`.
     */
    bool closeOnDestroy = true;
  };

  /**
   * Returns a default configuration.
   *
   * @return a default configuration
   */
  static consteval Config defaultConfig() { return {}; }

  /**
   * @ctor
   *
   * @param file a `FILE` pointer to use, nonnull
   * @param config the configuration
   */
  explicit FileSink(FILE* file, const Config& config = defaultConfig());

  /**
   * @ctor
   *
   * @param path a path to a file
   * @param config the configuration
   */
  explicit FileSink(const std::string& path, const Config& config = defaultConfig());

  ~FileSink() override;

  bool close() override;

  bool flush() override;

  [[nodiscard]] i32 handle() const override;

  u64 write(std::span<const u8> in) override;

ROCKET_TEST_PRIVATE:

  FILE* file_ = nullptr; ///< The `FILE` pointer.
  Config config_; ///< The configuration.
};

// `NullSink` -----------------------------------------------------------------------------------------------

/**
 * A null sink that never writes anything.
 */
struct NullSink : Sink {
  NullSink();

  ~NullSink() override = default;

  bool close() override { return false; }

  bool flush() override { return false; }

  [[nodiscard]] i32 handle() const override { return -1; }

  u64 write([[maybe_unused]] std::span<const u8> in) override { return 0; }
};

// `SpanSink` -----------------------------------------------------------------------------------------------

/**
 * A sink that writes to a span, i.e. into preallocated memory.
 */
struct SpanSink : Sink {
  /**
   * @ctor
   *
   * @param out the span to write to
   */
  explicit SpanSink(std::span<char> out);

  ~SpanSink() override = default;

  bool close() override;

  bool flush() override { return false; }

  [[nodiscard]] i32 handle() const override { return -1; }

  u64 write(std::span<const u8> in) override;

private:

  std::span<char> out_;
  u64 pos_ = 0;
};

// `StreamSink` ---------------------------------------------------------------------------------------------

/**
 * The class #StreamSink provides support for #std::ostream.
 *
 * Using I/O streams is generally discouraged, because it’s not partable and not efficient. Wherever
 * possible, use #rocket::nio::FileSink instead.
 */
struct StreamSink : Sink {
  /**
   * @ctor
   *
   * @param os the output stream to write to
   */
  explicit StreamSink(std::ostream& os);

  ~StreamSink() override;

  bool close() override;

  bool flush() override;

  [[nodiscard]] i32 handle() const override;

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
  explicit StringSink();

  /**
   * Constructs a new #StringSink with a string reference and no owned string.
   *
   * @param ref the string to write to. The reference must remain valid for the lifetime of the #StringSink
   */
  explicit StringSink(std::string& ref);

  ~StringSink() override = default;

  bool close() override;

  bool flush() override { return false; }

  [[nodiscard]] i32 handle() const override { return -1; }

  /**
   * Returns a reference to the string (either referenced or owned).
   *
   * @return the referenced or the owned string
   */
  [[nodiscard]] const std::string& str() const { return ptr_ != nullptr ? *ptr_ : owned_; }

  u64 write(std::span<const u8> in) override;

private:

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
  virtual std::istream& istream() = 0;

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
  virtual std::string readln();

  /**
   * Reads all available bytes from a source into a vector.
   *
   * @return the bytes read
   */
  std::vector<u8> readAll();

  /**
   * Tries to read a valid code point from a source.
   *
   * @return a code point if one was read, null otherwise. If the return value is null, the number of bytes
   *   read is undefined
   */
  virtual std::optional<unicode::CodePoint> readCodePoint();

  /**
   * Reads all available characters from a source into a string.
   *
   * @return the string read
   */
  std::string readString();

  /**
   * Seeks a new position in the source.
   *
   * @param offset the offset to seek to
   * @param mode the seek mode
   * @return whether the operation succeeded
   */
  virtual bool seek(i64 offset, SeekMode mode = SeekMode::beg) = 0; // NOLINT

  /**
   * Stores a value in the source.
   *
   * @param val the value to store
   * @return a reference to the stored value, which remains valid for the lifetime of the source
   */
  const std::string&
  store(std::string&& val) {
    return *strings_.insert(std::move(val)).first;
  }

  /**
   * Stores a value in the source.
   *
   * @param val the value to store
   * @return a reference to the stored value, which remains valid for the lifetime of the source
   */
  const std::u32string&
  store(std::u32string&& val) {
    return *u32strings_.insert(std::move(val)).first;
  }

  /**
   * Returns the current input position
   *
   * @return the current input position, or #rocket::NPOS if the position cannot be determined
   */
  virtual u64 tell() = 0;

protected:

  /// @ctor_default
  Source() = default;

private:

  /// We need pointer stability, which #std::unordered_set guarantees.
  std::unordered_set<std::string> strings_;
  /// We need pointer stability, which #std::unordered_set guarantees.
  std::unordered_set<std::u32string> u32strings_;
};

// `ContiguousSource` ---------------------------------------------------------------------------------------

/**
 * A contiguous source.
 */
struct ContiguousSource : Source {
  ~ContiguousSource() override = default;

  /**
   * Returns the bytes available in the contiguous source.
   *
   * @return the bytes available in the contiguous source
   */
  virtual std::span<const u8> bytes() const = 0;

  std::string readln() override;

  std::optional<unicode::CodePoint> readCodePoint() override;

  /**
   * Returns the string available in the contiguous source.
   *
   * @return the string available in the contiguous source
   */
  virtual std::string_view str() const = 0;

protected:

  /// @ctor_default
  ContiguousSource() = default;
};

// `BufferedSource` -----------------------------------------------------------------------------------------

/**
 * A buffered source that may be attached to another source.
 *
 * @note Applying an additional buffer only makes sense if the underlying source isn't already buffered.
 */
struct BufferedSource : Source {
  /**
   * @ctor
   *
   * @param underlying the underlying source
   * @param size the size of the buffer
   */
  explicit BufferedSource(Source& underlying, u64 size = DEFAULT_BUFFER_SIZE);

  ~BufferedSource() override;

  bool close() override;

  [[nodiscard]] i32 handle() const override { return underlying_.handle(); }

  std::istream& istream() override;

  u64 read(std::span<u8> out) override;

  bool seek(i64 offset, SeekMode mode = SeekMode::beg) override; // NOLINT

  u64 tell() override;

ROCKET_TEST_PRIVATE:

  Source& underlying_; ///< The underlying source.
  u64 size_; ///< The size of the buffer.
  /// The buffer.
  std::unique_ptr<u8[]> buf_; // NOLINT
  u64 bufPos_ = NPOS; ///< Where buffer position 0 maps to in the underlying source.
  u64 pos_ = 0; ///< The current position in the buffer.
  /**
   * This is the actual input size of the buffer, which may be less than its allocated size.
   *
   * If this is 0, #pos_ must be 0, too, and the buffer is considered to be invalid.
   */
  u64 end_ = 0;
};

// `FileSource` ---------------------------------------------------------------------------------------------

/**
 * A file source, backed by a `FILE` pointer.
 */
struct FileSource : Source {
  /**
   * Configuration for the #FileSource constructor.
   */
   struct Config {
    /**
     * Whether to close the file on destruction.
     *
     * The default is `true`. For `stdin`, this is automatically configured to be `false`.
     */
    bool closeOnDestroy = true;
  };

  /**
   * Returns a default configuration.
   *
   * @return a default configuration
   */
  static consteval Config defaultConfig() { return {}; }

  /**
   * @ctor
   *
   * @param file a `FILE` pointer to use
   * @param config the configuration
   */
  explicit FileSource(FILE* file, const Config& config = defaultConfig());

  /**
   * @ctor
   *
   * @param path a path to a file
   * @param config the configuration
   */
  explicit FileSource(const std::string& path, const Config& config = defaultConfig());

  ~FileSource() override;

  bool close() override;

  [[nodiscard]] i32 handle() const override;

  std::istream& istream() override;

  u64 read(std::span<u8> out) override;

  bool seek(i64 offset, SeekMode mode = SeekMode::beg) override; // NOLINT

  u64 tell() override;

ROCKET_TEST_PRIVATE:

  FILE* file_ = nullptr; ///< The `FILE` pointer.
  Config config_; ///< The configuration.
  std::unique_ptr<std::istream> istream_; ///< An optional input stream.
};

// `NullSource` ---------------------------------------------------------------------------------------------

/**
 * A null source that never reads anything.
 */
 struct NullSource : Source {
  NullSource();

  ~NullSource() override = default;

  bool close() override { return false; }

  [[nodiscard]] i32 handle() const override { return -1; }

  std::istream& istream() override;

  u64 read([[maybe_unused]] std::span<u8> out) override { return 0; }

  bool
  seek([[maybe_unused]] i64 offset, [[maybe_unused]] SeekMode mode = SeekMode::beg) override { // NOLINT
    return false;
  }

  u64 tell() override { return NPOS; }

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

  std::span<const u8> bytes() const override { return in_.subspan(pos_); }

  bool close() override;

  i32 handle() const override { return -1; }

  std::istream& istream() override;

  /**
   * Returns the input span the source was constructed with.
   *
   * @return the input span that the source was constructed with
   */
  std::span<const u8> in() const { return in_; }

  u64 read(std::span<u8> out) override;

  bool seek(i64 offset, SeekMode mode = SeekMode::beg) override; // NOLINT

  std::string_view
  str() const override {
    const auto bytes = this->bytes();
    return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
  }

  u64 tell() override { return bad() ? NPOS : static_cast<u64>(pos_); }

private:

  std::span<const u8> in_;
  boost::safe_numerics::safe<u64> pos_ = 0;
  std::unique_ptr<std::istream> istream_;
};

// `StreamSource` -------------------------------------------------------------------------------------------

/**
 * The class #StreamSource provides support for #std::istream.
 *
 * Using I/O streams is generally discouraged, because it’s not partable and not efficient. Wherever
 * possible, use #rocket::nio::FileSource instead.
 */
struct StreamSource : Source {
  /**
   * @ctor
   *
   * @param is the input stream to read from
   */
  explicit StreamSource(std::istream& is);

  ~StreamSource() override;

  bool close() override;

  [[nodiscard]] i32 handle() const override;

  std::istream& istream() override { return is_; }

  u64 read(std::span<u8> out) override;

  bool seek(i64 offset, SeekMode mode = SeekMode::beg) override; // NOLINT

  u64 tell() override;

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

  std::span<const u8>
  bytes() const override {
    const auto str = this->str();
    return { reinterpret_cast<const u8*>(str.data()), str.size() };
  }

  bool close() override;

  i32 handle() const override { return -1; }

  /**
   * Returns the input string the source was constructed with.
   *
   * @return the input string that the source was constructed with
   */
  std::string_view in() const { return in_; }

  std::istream& istream() override;

  u64 read(std::span<u8> out) override;

  bool seek(i64 offset, SeekMode mode = SeekMode::beg) override; // NOLINT

  std::string_view str() const override { return in_.substr(pos_); }

  u64 tell() override { return bad() ? NPOS : static_cast<u64>(pos_); }

private:

  std::string_view in_;
  boost::safe_numerics::safe<u64> pos_ = 0;
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
