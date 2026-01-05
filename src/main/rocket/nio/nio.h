/**
 * @file nio.h
 *
 * New I/O: efficient sinks and sources.
 */

#pragma once

#include "rocket/format/format.h"

#include <iosfwd>
#include <memory>
#include <span>
#include <string>

namespace rocket::nio {

// Constants ------------------------------------------------------------------------------------------------

/**
 * The default buffer size in bytes.
 */
static constexpr size_t DEFAULT_BUFFER_SIZE = 64 * 1'024; // 64 KiB
/**
  * The minimum buffer size in bytes.
  */
static constexpr size_t MIN_BUFFER_SIZE = 64;

// `Io` -----------------------------------------------------------------------------------------------------

/**
 * The base class for #rocket::nio::Sink and #rocket::nio::Source.
 */
struct Io {
  /// The offset type.
  using Offset = long;
  /// The position type.
  using Position = unsigned long;

  /// @dtor
  virtual ~Io() {}

  /**
   * Closes the object.
   *
   * @return 0 if successful, an error code otherwise
   */
  virtual int close() = 0;

  /**
   * Returns the file descriptor.
   *
   * @return the file descriptor, or -1 if the file descriptor cannot be determined
   */
  virtual int fd() = 0;

  /**
   * Returns the error status.
   *
   * @return the error status
   */
   virtual int error() const { return error_; }

  /**
   * Returns if the object is open and the error status is 0.
   *
   * @return `true` if the object is open and the error status is 0
   */
   virtual bool good() const { return open_ && error_ == 0; }

   /**
    * Returns if the object is open.
    *
    * @return `true` if the object is open
    */
   virtual bool open() const { return open_; }

protected:

  /// @ctor_default
  Io() {}

  Io(const Io& rhs) = delete;

  int error_ = 0; ///< The error status.
  bool open_ = true; ///< Open flag.

  /**
   * Checks if the object is open. If not and if the error status is 0, sets the error status to `EBADF`.
   *
   * @return `true` if the object is open
   */
  bool checkOpen();
};

// `Sink` ---------------------------------------------------------------------------------------------------

/**
 * Sink base class.
 */
struct Sink : Io {
  virtual ~Sink() override {}

  /**
   * Flushes the sink.
   *
   * @return 0 if successful, an error code otherwise
   */
  virtual int flush() = 0;

  /**
   * Prints a formatted message to the sink.
   *
   * @param fmt the format string
   * @param args the arguments
   * @return the number of bytes written
   */
  template<typename... T>
  size_t
  print(fmt::format_string<T...> fmt, T&&... args) {
    auto formatted = fmt::format(fmt, std::forward<T>(args)...);
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
  size_t
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
  size_t
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
  size_t
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
  size_t
  write(char c) {
    return write(std::string_view(&c, 1));
  }

  /**
   * Writes a string to the sink.
   *
   * @param in the string to write
   * @return the number of bytes written
   */
  virtual size_t write(std::string_view in) = 0;

  /**
   * Writes a string to the sink.
   *
   * @param in the string to write
   * @param offset the offset at which to start writing
   * @param n the number of bytes to write
   * @return the number of bytes written
   */
  size_t
  write(std::string_view in, size_t offset, size_t n = std::string_view::npos) {
    return write(in.substr(offset, n));
  }

  /**
   * Writes a string and a line feed to the sink.
   *
   * @param in the string to write
   * @return the number of bytes written
   */
  size_t writeln(std::string_view in);

  /**
   * Writes a string and a line feed to the sink.
   *
   * @param in the string to write
   * @param offset the offset at which to start writing
   * @param n the number of bytes to write
   * @return the number of bytes written
   */
  size_t
  writeln(std::string_view in, size_t offset, size_t n = std::string_view::npos) {
    return writeln(in.substr(offset, n));
  }

protected:

  /// @ctor_default
  Sink() {}
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
  explicit BufferedSink(Sink& underlying, size_t size = DEFAULT_BUFFER_SIZE);

  /// @dtor
  virtual ~BufferedSink() override;

  virtual int close() override;

  virtual int error() const override { return underlying_.error(); } // cppcheck-suppress uselessOverride

  virtual int fd() override;

  virtual int flush() override;

  virtual bool good() const override { return underlying_.good(); } // cppcheck-suppress uselessOverride

  virtual bool open() const override { return underlying_.open(); } // cppcheck-suppress uselessOverride

  virtual size_t write(std::string_view in) override;

ROCKET_TESTING_PRIVATE:

  Sink& underlying_; ///< The underlying sink.
  size_t size_; ///< The size of the buffer.
  std::unique_ptr<char[]> buf_; ///< The buffer.
  size_t pos_ = 0; ///< The current position in the buffer.

  /// Flushes the buffer to the underlying sink.
  void flushBuffer();
};

// `FileSink` -----------------------------------------------------------------------------------------------

/**
 * A file sink, backed by a `FILE` pointer.
 */
struct FileSink : Sink {
  /**
   * Parameters for the #FileSink constructor.
   */
  struct Params {
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
   * Returns default parameters.
   *
   * @return the default parameters
   */
  static consteval Params defaultParams() { return {}; }

  /**
   * @ctor
   *
   * @param file a `FILE` pointer to use
   * @param params the parameters
   */
  explicit FileSink(FILE* file, const Params& params = defaultParams());

  /**
   * @ctor
   *
   * @param path a path to a file
   * @param params the parameters
   */
  explicit FileSink(const std::string& path, const Params& params = { .append=false, .closeOnDestroy=true });

  /// @dtor
  virtual ~FileSink() override;

  virtual int close() override;

  virtual int fd() override;

  virtual int flush() override;

  virtual size_t write(std::string_view in) override;

ROCKET_TESTING_PRIVATE:

  FILE* file_; ///< The `FILE` pointer.
  Params params_; ///< The parameters.
};

// `NullSink` -----------------------------------------------------------------------------------------------

/**
 * A null sink that never writes anything.
 */
struct NullSink : Sink {
  virtual ~NullSink() override;

  virtual int close() override;

  virtual int fd() override;

  virtual int flush() override;

  virtual size_t write(std::string_view in) override;
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
  explicit SpanSink(std::span<char> out) : out_(out) {}

  virtual ~SpanSink() override;

  virtual int close() override;

  virtual int fd() override { return -1; }

  virtual int flush() override;

  virtual size_t write(std::string_view in) override;

private:

  std::span<char> out_;
  size_t pos_ = 0;
};

// `StreamSink` ---------------------------------------------------------------------------------------------

/**
 * The class `StreamSink` provides support for `std::ostream`.
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

  virtual ~StreamSink() override;

  virtual int close() override;

  virtual int fd() override;

  virtual int flush() override;

  virtual size_t write(std::string_view in) override;

private:

  std::ostream& os_;
};

// `StringSink` ---------------------------------------------------------------------------------------------

/**
 * A sink that appends to a string.
 *
 * If the sink is constructed without a string reference, it holds its own managed string that can be
 * accessed via #StringSink::str.
 */
struct StringSink : Sink {
  /**
   * Makes a new `StringSink` with a managed string.
   */
  explicit StringSink() {}

  /**
   * Makes a new `StringSink` with a string reference and no managed string.
   *
   * @param out the string to write to
   */
  explicit StringSink(std::string& out) : out(&out) {}

  virtual ~StringSink() override;

  virtual int close() override;

  virtual int fd() override { return -1; }

  virtual int flush() override;

  /**
   * Returns the managed string.
   *
   * @return the managed string
   * @throws #rocket::InvalidState if the sink has no managed string
   */
  std::string str() const;

  virtual size_t write(std::string_view in) override;

private:

  std::string* out = nullptr;
  std::string managed_;
};

// `SeekMode` -----------------------------------------------------------------------------------------------

/**
 * The seek mode for #rocket::nio::Source#seek.
 */
enum class SeekMode {
  beg, ///< Seek relative to the beginning of the source
  cur, ///< Seek relative to the current position of the source
  end ///< Seek relative to the end of the source
};

// `Source` -------------------------------------------------------------------------------------------------

/**
 * Source base class.
 */
struct Source : Io {
  virtual ~Source() override {}

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
  size_t read(char& out) { return read({ &out, 1 }); }

  /**
   * Reads as many characters as available into a span.
   *
   * @param out the span to read into
   * @return the number of bytes read
   */
  virtual size_t read(std::span<char> out) = 0;

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
  size_t readln(std::span<char> out);

  /**
   * Seeks to a new position in the source.
   *
   * @param offset the offset to seek to
   * @param mode the seek mode
   * @return 0 if successful, an error code otherwise
   */
  virtual int seek(Offset offset, SeekMode mode = SeekMode::beg) = 0;

  /**
   * Returns the current input position
   *
   * @return the current input position, or -1 if that position cannot be determined
   */
  virtual Position tell() = 0;

protected:

  /// @ctor_default
  Source() {}
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
   explicit BufferedSource(Source& underlying, size_t size = DEFAULT_BUFFER_SIZE);

  virtual ~BufferedSource() override;

  virtual int close() override;

  virtual int error() const override { return underlying_.error(); } // cppcheck-suppress uselessOverride

  virtual int fd() override;

  virtual bool good() const override { return underlying_.good(); } // cppcheck-suppress uselessOverride

  virtual bool open() const override { return underlying_.open(); } // cppcheck-suppress uselessOverride

  virtual size_t read(std::span<char> out) override;

  virtual int seek(Offset offset, SeekMode mode = SeekMode::beg) override;

  virtual Position tell() override;

ROCKET_TESTING_PRIVATE:

  Source& underlying_; ///< The underlying source.
  size_t size_; ///< The size of the buffer.
  std::unique_ptr<char[]> buf_; ///< The buffer.
  size_t bufPos_ = -1; ///< Where buffer position 0 maps to in the underlying source.
  size_t pos_ = 0; ///< The current position in the buffer.
  /**
   * This is the actual input size of the buffer, which may be less than its allocated size.
   *
   * If this is 0, #pos_ must be 0, too, and the buffer is considered to be invalid.
   */
  size_t end_ = 0;
};

// `FileSource` ---------------------------------------------------------------------------------------------

/**
 * A file source, backed by a `FILE` pointer.
 */
struct FileSource : Source {
  /**
   * Parameters for the #FileSource constructor.
   */
   struct Params {
    /**
     * Whether to close the file on destruction.
     *
     * The default is `true`. For `stdin`, this is automatically configured to be `false`.
     */
    bool closeOnDestroy = true;
  };

  /**
   * Returns default parameters.
   *
   * @return the default parameters
   */
  static consteval Params defaultParams() { return {}; }

  /**
   * @ctor
   *
   * @param file a `FILE` pointer to use
   * @param params the parameters
   */
  explicit FileSource(FILE* file, const Params& params = defaultParams());

  /**
   * @ctor
   *
   * @param path a path to a file
   * @param params the parameters
   */
  explicit FileSource(const std::string& path, const Params& params = { .closeOnDestroy=true });

  virtual ~FileSource() override;

  virtual int close() override;

  virtual int fd() override;

  virtual size_t read(std::span<char> out) override;

  virtual int seek(Offset offset, SeekMode mode = SeekMode::beg) override;

  virtual Position tell() override;

ROCKET_TESTING_PRIVATE:

  FILE* file_; ///< The `FILE` pointer.
  Params params_; ///< The parameters.
};

// `NullSource` ---------------------------------------------------------------------------------------------

/**
 * A null source that never reads anything.
 */
 struct NullSource : Source {
  virtual ~NullSource() override;

  virtual int close() override;

  virtual int fd() override { return -1; }

  virtual size_t read(std::span<char> out) override;

  virtual int seek(Offset offset, SeekMode mode = SeekMode::beg) override;

  virtual Position tell() override;
};

// `StreamSource` -------------------------------------------------------------------------------------------

/**
 * The class `StreamSource` provides support for `std::istream`.
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

  virtual ~StreamSource() override;

  virtual int close() override;

  virtual int fd() override;

  virtual size_t read(std::span<char> out) override;

  virtual int seek(Offset offset, SeekMode mode = SeekMode::beg) override;

  virtual Position tell() override;

private:

  std::istream& is_;
};

// `StringSource` -------------------------------------------------------------------------------------------

/**
 * A sourcde that reads from a string.
 */
struct StringSource : Source {
  StringSource() {}

  /**
   * @ctor
   *
   * @param in the string to read from
   */
  explicit StringSource(std::string_view in) : in_(in) {}

  virtual ~StringSource() override;

  virtual int close() override;

  virtual int fd() override { return -1; }

  virtual size_t read(std::span<char> out) override;

  virtual int seek(Offset offset, SeekMode mode = SeekMode::beg) override;

  virtual Position tell() override;

private:

  std::string_view in_;
  size_t pos_ = 0;
};

// Variables ------------------------------------------------------------------------------------------------

/// The standard input source.
extern Source& stdin;
/// The standard output sink.
extern Sink& stdout;
/// The standard error sink.
extern Sink& stderr;

} // namespace rocket::nio

// EOF
