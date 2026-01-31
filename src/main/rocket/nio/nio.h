/**
 * @file nio.h
 *
 * New I/O: efficient sinks and sources.
 */

#pragma once

#include "rocket/rocket.h"
#include "rocket/format/format.h"

#include <boost/safe_numerics/safe_integer.hpp>

#include <iosfwd>
#include <memory>
#include <span>
#include <string>

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

// #Io ------------------------------------------------------------------------------------------------------

/**
 * The base class for #rocket::nio::Sink and #rocket::nio::Source.
 *
 * An I/O instance, either a sink or a source.
 */
struct Io {
  /// @ctor_copy
  Io(const Io& rhs) = delete;

  /// @dtor
  virtual ~Io() = default;

  /**
   * Closes the instance.
   *
   * @return 0 if successful, an error code otherwise
   */
  virtual i32 close() = 0;

  /**
   * Returns the error status.
   *
   * @return the error status
   */
  [[nodiscard]] virtual i32 error() const { return error_; }

  /**
   * Checks if the instance is open and the error status is 0.
   *
   * @return whether the instance is open and the error status is 0
   */
  [[nodiscard]] virtual bool good() const { return open_ && error_ == 0; }

  /**
   * Returns the handle of the instance.
   *
   * @return the handle of the instance, or -1 if the handle cannot be determined
   */
  [[nodiscard]] virtual i32 handle() const = 0;

  /**
   * Checks if the instance is open.
   *
   * @return whether the instance is open
   */
  [[nodiscard]] virtual bool open() const { return open_; }

protected:

  /// @ctor_default
  Io() = default;

  mutable i32 error_ = 0; ///< The error status.
  bool open_ = true; ///< Open flag.

  /**
   * Checks if the instance is open.
   *
   * If it is not and if the error status is 0, sets the error status to `EBADF`.
   *
   * @return whether the instance is open
   */
  bool checkOpen() const;
};

// #Sink ----------------------------------------------------------------------------------------------------

/**
 * Sink base class.
 */
struct Sink : Io {
  /**
   * Flushes the sink.
   *
   * @return 0 if successful, an error code otherwise
   */
  virtual i32 flush() = 0;

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
   * @param c the character
   * @return the number of bytes written
   */
  u64
  write(char c) {
    return write(std::string_view(&c, 1));
  }

  /**
   * Writes a string to the sink.
   *
   * @param in the string to write
   * @return the number of bytes written
   */
  virtual u64 write(std::string_view in) = 0;

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

  /// @ctor_default
  Sink() = default;
};

// #BufferedSink --------------------------------------------------------------------------------------------

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

  i32 close() override;

  i32 error() const override { return underlying_.error(); }

  i32 flush() override;

  bool good() const override { return underlying_.good(); }

  i32 handle() const override { return underlying_.handle(); }

  bool open() const override { return underlying_.open(); }

  u64 write(std::string_view in) override;

ROCKET_TEST_PRIVATE:

  Sink& underlying_; ///< The underlying sink.
  u64 size_; ///< The size of the buffer.
  /// The buffer.
  std::unique_ptr<char[]> buf_; // NOLINT
  u64 pos_ = 0; ///< The current position in the buffer.

  /// Flushes the buffer to the underlying sink.
  void flushBuffer();
};

// #FileSink ------------------------------------------------------------------------------------------------

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
   * @param file a `FILE` pointer to use
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

  i32 close() override;

  i32 flush() override;

  i32 handle() const override;

  u64 write(std::string_view in) override;

ROCKET_TEST_PRIVATE:

  FILE* file_; ///< The `FILE` pointer.
  Config config_; ///< The configuration.
};

// #NullSink ------------------------------------------------------------------------------------------------

/**
 * A null sink that never writes anything.
 */
struct NullSink : Sink {
  ~NullSink() override;

  i32 close() override { return EIO; }

  i32 flush() override { return EIO; }

  i32 handle() const override { return -1; }

  u64 write([[maybe_unused]] std::string_view in) override { return 0; }
};

// #SpanSink ------------------------------------------------------------------------------------------------

/**
 * A sink that writes to a span, i.e. into preallocated memory.
 */
struct SpanSink : Sink {
  /**
   * @ctor
   *
   * @param out the span to write to
   */
  explicit SpanSink(std::span<char> out) : out_(out) {}

  ~SpanSink() override = default;

  i32 close() override { return EIO; }

  i32 flush() override { return 0; }

  i32 handle() const override { return -1; }

  u64 write(std::string_view in) override;

private:

  std::span<char> out_;
  u64 pos_ = 0;
};

// #StreamSink ----------------------------------------------------------------------------------------------

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
  explicit StreamSink(std::ostream& os) : os_(os) {}

  ~StreamSink() override;

  i32 close() override;

  i32 flush() override;

  i32 handle() const override;

  u64 write(std::string_view in) override;

private:

  std::ostream& os_;
};

// #StringSink ----------------------------------------------------------------------------------------------

/**
 * A sink that appends to a string.
 *
 * If the sink is constructed without a string reference, it holds an owned string that can be accessed via
 * #StringSink::str.
 */
struct StringSink : Sink {
  /**
   * Makes a new #StringSink with an owned string.
   */
  explicit StringSink() = default;

  /**
   * Makes a new #StringSink with a string reference and no owned string.
   *
   * @param ref the string to write to. The reference must remain valid for the lifetime of the #StringSink
   */
  explicit StringSink(std::string& ref) : ptr_(&ref) {}

  ~StringSink() override = default;

  i32 close() override { return EIO; }

  i32 flush() override { return EIO; }

  i32 handle() const override { return -1; }

  /**
   * Returns a reference to the string (either referenced or owned).
   *
   * @return the referenced or the owned string
   */
  const std::string& str() const { return ptr_ != nullptr ? *ptr_ : owned_; }

  u64 write(std::string_view in) override;

private:

  std::string* ptr_ = nullptr;
  std::string owned_;
};

// #SeekMode ------------------------------------------------------------------------------------------------

/**
 * The seek mode for #rocket::nio::Source#seek.
 */
enum class SeekMode : u8 {
  beg, ///< Seek relative to the beginning of the source
  cur, ///< Seek relative to the current position of the source
  end ///< Seek relative to the end of the source
};

// #Source --------------------------------------------------------------------------------------------------

/**
 * Source base class.
 */
struct Source : Io {
  /**
   * Reads all characters from a source into string.
   *
   * @return the string read
   */
  std::string read();

  /**
   * Reads a single character from a source.
   *
   * @param out the character to read
   * @return the number of bytes read
   */
  u64 read(char& out) { return read({ &out, 1 }); }

  /**
   * Reads as many characters as available into a span.
   *
   * @param out the span to read into
   * @return the number of bytes read
   */
  virtual u64 read(std::span<char> out) = 0;

  /**
   * Reads a line from a source into a string.
   *
   * A trailing @c '\\r' is removed if it precedes a @c '\\n'.
   *
   * @return the line read, not containing the trailing @c '\\r' or @c '\\n'
   */
  std::string readln();

  /**
   * Reads a line from a source into a span.
   *
   * A trailing @c '\\r' is removed if it precedes a @c '\\n'.
   *
   * @param out the span to read into
   * @return the number of bytes read
   */
  u64 readln(std::span<char> out);

  /**
   * Seeks to a new position in the source.
   *
   * @param offset the offset to seek to
   * @param mode the seek mode
   * @return 0 if successful, an error code otherwise
   */
  virtual i32 seek(i64 offset, SeekMode mode = SeekMode::beg) = 0; // NOLINT

  /**
   * Returns the current input position
   *
   * @return the current input position, or #rocket::NPOS if that position cannot be determined
   */
  virtual u64 tell() = 0;

protected:

  /// @ctor_default
  Source() = default;
};

// #BufferedSource ------------------------------------------------------------------------------------------

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

  i32 close() override;

  i32 error() const override { return underlying_.error(); } // cppcheck-suppress uselessOverride

  bool good() const override { return underlying_.good(); } // cppcheck-suppress uselessOverride

  i32 handle() const override { return underlying_.handle(); }

  bool open() const override { return underlying_.open(); } // cppcheck-suppress uselessOverride

  u64 read(std::span<char> out) override;

  i32 seek(i64 offset, SeekMode mode = SeekMode::beg) override; // NOLINT

  u64 tell() override;

ROCKET_TEST_PRIVATE:

  Source& underlying_; ///< The underlying source.
  u64 size_; ///< The size of the buffer.
  /// The buffer.
  std::unique_ptr<char[]> buf_; // NOLINT
  u64 bufPos_ = NPOS; ///< Where buffer position 0 maps to in the underlying source.
  u64 pos_ = 0; ///< The current position in the buffer.
  /**
   * This is the actual input size of the buffer, which may be less than its allocated size.
   *
   * If this is 0, #pos_ must be 0, too, and the buffer is considered to be invalid.
   */
  u64 end_ = 0;
};

// #FileSource ----------------------------------------------------------------------------------------------

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

  i32 close() override;

  i32 handle() const override;

  u64 read(std::span<char> out) override;

  i32 seek(i64 offset, SeekMode mode = SeekMode::beg) override; // NOLINT

  u64 tell() override;

ROCKET_TEST_PRIVATE:

  FILE* file_; ///< The `FILE` pointer.
  Config config_; ///< The configuration.
};

// #NullSource ----------------------------------------------------------------------------------------------

/**
 * A null source that never reads anything.
 */
 struct NullSource : Source {
  ~NullSource() override;

  i32 close() override { return EIO; }

  i32 handle() const override { return -1; }

  u64 read([[maybe_unused]] std::span<char> out) override { return 0; }

  i32
  seek([[maybe_unused]] i64 offset, [[maybe_unused]] SeekMode mode = SeekMode::beg) override { // NOLINT
    return EINVAL;
  }

  u64 tell() override { return NPOS; }
};

// #StreamSource --------------------------------------------------------------------------------------------

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
  explicit StreamSource(std::istream& is) : is_(is) {}

  ~StreamSource() override;

  i32 close() override;

  i32 handle() const override;

  u64 read(std::span<char> out) override;

  i32 seek(i64 offset, SeekMode mode = SeekMode::beg) override; // NOLINT

  u64 tell() override;

private:

  std::istream& is_;
};

// #StringSource --------------------------------------------------------------------------------------------

/**
 * A sourcde that reads from a string.
 */
struct StringSource : Source {
  StringSource() = default;

  /**
   * @ctor
   *
   * @param in the string to read from
   */
  explicit StringSource(std::string_view in) : in_(in) {}

  ~StringSource() override = default;

  i32 close() override { return EIO; }

  i32 handle() const override { return -1; }

  u64 read(std::span<char> out) override;

  i32 seek(i64 offset, SeekMode mode = SeekMode::beg) override; // NOLINT

  u64 tell() override { return pos_; }

private:

  std::string_view in_;
  boost::safe_numerics::safe<u64> pos_ = 0;
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
