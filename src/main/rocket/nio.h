/**
 * @file nio.h
 *
 * New I/O: effiicient sinks and sources.
 */

#pragma once

#include "format-std.h"

#include <iostream>
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

// `Sink` ---------------------------------------------------------------------------------------------------

struct Sink {
  virtual ~Sink() = default;

  virtual int close() = 0;

  virtual int error() const { return error_; }

  /**
   * Returns the file descriptor.
   *
   * @return the file descriptor of the sink, or -1 if the sink is not connected to a file
   */
  virtual int fd() = 0;

  virtual int flush() = 0;

  virtual bool good() const { return open_ && error_ == 0; }

  virtual bool open() const { return open_; }

  template<typename... T>
  void
  print(fmt::format_string<T...> fmt, T&&... args) {
    auto formatted = fmt::format(fmt, std::forward<T>(args)...);
    write(formatted);
  }

  template<typename... T>
  void
  print(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    auto formatted = fmt::format(locale, fmt, std::forward<T>(args)...);
    write(formatted);
  }

  template<typename... T>
  void
  println(fmt::format_string<T...> fmt, T&&... args) {
    print(fmt, std::forward<T>(args)...);
    write('\n');
    flush();
  }

  template<typename... T>
  void
  println(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    print(locale, fmt, std::forward<T>(args)...);
    write('\n');
    flush();
  }

  size_t
  write(char c) {
    return write(std::string_view(&c, 1));
  }

  virtual size_t write(std::string_view in) = 0;

  size_t
  write(std::string_view in, size_t offset, size_t n = std::string_view::npos) {
    return write(in.substr(offset, n));
  }

  size_t writeln(std::string_view in);

  size_t
  writeln(std::string_view in, size_t offset, size_t n = std::string_view::npos) {
    return writeln(in.substr(offset, n));
  }

protected:

  Sink() {}

  Sink(const Sink& rhs) = delete;

  bool checkOpen();

  int error_ = 0;
  bool open_ = true;
};

// `BufferedSink` -------------------------------------------------------------------------------------------

struct BufferedSink : Sink {
  explicit BufferedSink(Sink& underlying, size_t size = DEFAULT_BUFFER_SIZE);

  virtual ~BufferedSink() override;

  virtual int close() override;

  virtual int error() const override { return underlying_.error(); } // cppcheck-suppress uselessOverride

  virtual int fd() override { return underlying_.fd(); }

  virtual int flush() override;

  virtual bool good() const override { return underlying_.good(); } // cppcheck-suppress uselessOverride

  virtual bool open() const override { return underlying_.open(); } // cppcheck-suppress uselessOverride

  virtual size_t write(std::string_view in) override;

ROCKET_TESTING_PRIVATE:

  Sink& underlying_;
  size_t size_;
  std::unique_ptr<char[]> buf_;
  size_t pos_ = 0;

  void flushBuffer();
};

// `FileSink` -----------------------------------------------------------------------------------------------

struct FileSink : Sink {
  struct Params {
    bool append = false;
    /**
     * Whether to close the file on destruction.
     *
     * The default is `true`. For `stdout` and `stderr`, this is automatically configured to be `false`.
     */
    bool closeOnDestroy = true;
  };

  static consteval Params defaultParams() { return {}; }

  explicit FileSink(FILE* file, const Params& params = defaultParams());

  explicit FileSink(const std::string& path, const Params& params = { .append=false, .closeOnDestroy=true });

  virtual ~FileSink() override;

  virtual int close() override;

  virtual int fd() override;

  virtual int flush() override;

  virtual size_t write(std::string_view in) override;

ROCKET_TESTING_PRIVATE:

  FILE* file_;
  Params params_;
};

// `NullSink` -----------------------------------------------------------------------------------------------

struct NullSink : Sink {
  virtual ~NullSink() override;

  virtual int close() override;

  virtual int fd() override { return -1; }

  virtual int flush() override;

  virtual size_t write(std::string_view in) override;
};

// `SpanSink` -----------------------------------------------------------------------------------------------

struct SpanSink : Sink {
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
 * Using I/O streams is generally discouraged, because it’s not efficient. Wherever possible, use #FileSink
 * instead.
 */
struct StreamSink : Sink {
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

struct StringSink : Sink {
  explicit StringSink() {}

  explicit StringSink(std::string& out) : out(&out) {}

  virtual ~StringSink() override;

  virtual int close() override;

  virtual int fd() override { return -1; }

  virtual int flush() override;

  std::string str() const;

  virtual size_t write(std::string_view in) override;

private:

  std::string* out = nullptr;
  std::string managed_;
};

// `SeekMode` -----------------------------------------------------------------------------------------------

enum class SeekMode { beg, cur, end };

// `Source` -------------------------------------------------------------------------------------------------

struct Source {
  virtual ~Source() = default;

  virtual int close() = 0;

  virtual int error() const { return error_; }

  /**
   * Returns the file descriptor.
   *
   * @return the file descriptor of the sink, or -1 if the sink is not connected to a file
   */
  virtual int fd() = 0;

  virtual bool good() const { return open_ && error_ == 0; }

  virtual bool open() const { return open_; }

  std::string read();

  size_t read(char& out) { return read({ &out, 1 }); }

  virtual size_t read(std::span<char> out) = 0;

  std::string readln();

  size_t readln(std::span<char> out);

  virtual int seek(long pos, SeekMode mode = SeekMode::beg) = 0;

protected:

  Source() {}

  Source(const Sink& rhs) = delete;

  bool checkOpen();

  int error_ = 0;
  bool open_ = true;
};

// `BufferedSource` -----------------------------------------------------------------------------------------

struct BufferedSource : Source {
  explicit BufferedSource(Source& underlying, size_t size = DEFAULT_BUFFER_SIZE);

  virtual ~BufferedSource() override;

  virtual int close() override;

  virtual int error() const override { return underlying_.error(); } // cppcheck-suppress uselessOverride

  virtual int fd() override { return underlying_.fd(); }

  virtual bool good() const override { return underlying_.good(); } // cppcheck-suppress uselessOverride

  virtual bool open() const override { return underlying_.open(); } // cppcheck-suppress uselessOverride

  virtual size_t read(std::span<char> out) override;

  virtual int seek(long pos, SeekMode mode = SeekMode::beg) override;

ROCKET_TESTING_PRIVATE:

  Source& underlying_;
  size_t size_;
  std::unique_ptr<char[]> buf_;
  size_t pos_ = 0;
  size_t end_ = 0;
};

// `FileSource` ---------------------------------------------------------------------------------------------

struct FileSource : Source {
  struct Params {
    /**
     * Whether to close the file on destruction.
     *
     * The default is `true`. For `stdin`, this is automatically configured to be `false`.
     */
    bool closeOnDestroy = true;
  };

  static consteval Params defaultParams() { return {}; }

  explicit FileSource(FILE* file, const Params& params = defaultParams());

  explicit FileSource(const std::string& path, const Params& params = { .closeOnDestroy=true });

  virtual ~FileSource() override;

  virtual int close() override;

  virtual int fd() override;

  virtual size_t read(std::span<char> out) override;

  virtual int seek(long pos, SeekMode mode = SeekMode::beg) override;

ROCKET_TESTING_PRIVATE:

  FILE* file_;
  Params params_;
};

// `NullSource` ---------------------------------------------------------------------------------------------

struct NullSource : Source {
  virtual ~NullSource() override;

  virtual int close() override;

  virtual int fd() override { return -1; }

  virtual size_t read(std::span<char> out) override;

  virtual int seek(long pos, SeekMode mode = SeekMode::beg) override;
};

// `StreamSource` -------------------------------------------------------------------------------------------

/**
 * The class `StreamSource` provides support for `std::istream`.
 *
 * Using I/O streams is generally discouraged, because it’s not efficient. Wherever possible, use #FileSource
 * instead.
 */
struct StreamSource : Source {
  explicit StreamSource(std::istream& is) : is_(is) {}

  virtual ~StreamSource() override;

  virtual int close() override;

  virtual int fd() override;

  virtual size_t read(std::span<char> out) override;

  virtual int seek(long pos, SeekMode mode = SeekMode::beg) override;

  private:

  std::istream& is_;
};

// `StringSource` -------------------------------------------------------------------------------------------

struct StringSource : Source {
  explicit StringSource(std::string_view in) : in_(in) {}

  virtual ~StringSource() override;

  virtual int close() override;

  virtual int fd() override { return -1; }

  virtual size_t read(std::span<char> out) override;

  virtual int seek(long pos, SeekMode mode = SeekMode::beg) override;

private:

  std::string_view in_;
  size_t pos_ = 0;
};

// Variables ------------------------------------------------------------------------------------------------

extern Source& stdin;
extern Sink& stdout;
extern Sink& stderr;

} // namespace rocket::nio

// EOF
