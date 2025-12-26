/**
 * @file nio.h
 *
 * New I/O: effiicient sinks and sources.
 */

#pragma once

#include <fmt/format.h>

#include <memory>
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
static constexpr size_t MIN_BUFFER_SIZE = 128;

// `Sink` ---------------------------------------------------------------------------------------------------

struct Sink {
  Sink() = default;

  Sink(const Sink& rhs) = delete;

  virtual ~Sink() = default;

  virtual int close() = 0;

  virtual int error() const { return error_; }

  /**
   * Returns the file descriptor.
   *
   * @return the file descriptor of the sink, or -1 if the sink is not connected to a file
   */
  virtual int fd() const = 0;

  virtual int flush() = 0;

  virtual bool good() const { return open_ && error_ == 0; }

  virtual bool open() const { return open_; }

  template<typename... T>
  int print(fmt::format_string<T...> fmt, T&&... args) {
    auto formatted = fmt::format(fmt, std::forward<T>(args)...);
    return write(formatted);
  }

  template<typename... T>
  int print(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    auto formatted = fmt::format(locale, fmt, std::forward<T>(args)...);
    return write(formatted);
  }

  template<typename... T>
  int println(fmt::format_string<T...> fmt, T&&... args) {
    print(fmt, std::forward<T>(args)...);
    write('\n');
    return flush();
  }

  template<typename... T>
  int println(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    print(locale, fmt, std::forward<T>(args)...);
    write('\n');
    return flush();
  }

  int write(char c) {
    return write(std::string_view(&c, 1));
  }

  virtual int write(std::string_view data) = 0;

  int write(std::string_view data, size_t offset, size_t n = std::string_view::npos) {
    return write(data.substr(offset, n));
  }

  int writeln(std::string_view data);

  int writeln(std::string_view data, size_t offset, size_t n = std::string_view::npos) {
    return writeln(data.substr(offset, n));
  }

protected:

  int error_ = 0;
  bool open_ = true;
};

// `BufferedSink` -------------------------------------------------------------------------------------------

struct BufferedSink : Sink {
  explicit BufferedSink(Sink& underlying, size_t size = DEFAULT_BUFFER_SIZE);

  virtual int close() override;

  virtual int error() const override { return underlying_.error(); } // cppcheck-suppress uselessOverride

  virtual int fd() const override { return underlying_.fd(); }

  virtual int flush() override;

  virtual bool good() const override { return underlying_.good(); } // cppcheck-suppress uselessOverride

  virtual bool open() const override { return underlying_.open(); } // cppcheck-suppress uselessOverride

  virtual int write(std::string_view data) override;

private:

  Sink& underlying_;
  const size_t size_;
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

  explicit FileSink(FILE* file, const Params& params = { .append=false, .closeOnDestroy=true });

  explicit FileSink(const std::string& path, const Params& params = { .append=false, .closeOnDestroy=true });

  virtual ~FileSink() override;

  virtual int close() override;

  virtual int fd() const override;

  virtual int flush() override;

  virtual int write(std::string_view data) override;

ROCKET_PRIVATE:

  FILE* file_;
  Params params_;
};

// `NullSink` -----------------------------------------------------------------------------------------------

struct NullSink : Sink {
  virtual int close() override;

  virtual int fd() const override { return -1; }

  virtual int flush() override;

  virtual int write(std::string_view data) override;
};

// `StreamSink` ---------------------------------------------------------------------------------------------

/**
 * The class `StreamSink` provides support for I/O streams.
 *
 * Using I/O streams is generally discouraged, because it’s not efficient. Wherever possible, use #FileSink
 * instead.
 */
struct StreamSink : Sink {
  explicit StreamSink(std::ostream& os) : os_(os) {}

  virtual int close() override;

  virtual int fd() const override;

  virtual int flush() override;

  virtual int write(std::string_view data) override;

private:

  std::ostream& os_;
};

// `StringSink` ---------------------------------------------------------------------------------------------

struct StringSink : Sink {
  explicit StringSink(std::string& buf) : buf_(buf) {}

  virtual int close() override;

  virtual int fd() const override { return -1; }

  virtual int flush() override;

  virtual int write(std::string_view data) override;

private:

  std::string& buf_;
};

// Variables ------------------------------------------------------------------------------------------------

extern Sink& stdout;
extern Sink& stderr;

} // namespace rocket::nio

// EOF
