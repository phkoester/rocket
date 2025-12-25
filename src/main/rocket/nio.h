/**
 * @file nio.h
 *
 * New I/O.
 */

#pragma once

#include <ostream>
#include <string>
#include <fmt/format.h>

namespace rocket::nio {

// Internal -------------------------------------------------------------------------------------------------

struct Sink;

namespace internal {


} // namespace internal

// `Sink` ---------------------------------------------------------------------------------------------------

struct Sink {
  Sink() = default;

  Sink(const Sink& rhs) = delete;

  virtual ~Sink() = default;

  virtual int close() = 0;

  int error() const { return error_; }

  virtual int flush() = 0;

  bool good() const { return open_ && error_ == 0; }

  bool open() const { return open_; }

  template<typename... T>
  int print(fmt::format_string<T...> fmt, T&&... args) {
    return vprint(fmt, fmt::make_format_args(args...));
  }

  template<typename... T>
  int print(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    return vprint(locale, fmt, fmt::make_format_args(args...));
  }

  template<typename... T>
  int println(fmt::format_string<T...> fmt, T&&... args) {
    return vprintln(fmt, fmt::make_format_args(args...));
  }

  template<typename... T>
  int println(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    return vprintln(locale, fmt, fmt::make_format_args(args...));
  }

  int vprint(fmt::string_view fmt, fmt::format_args args) {
    return vprint({}, fmt, args);
  }

  int vprint(fmt::locale_ref locale, fmt::string_view fmt, fmt::format_args args);

  int vprintln(fmt::string_view fmt, fmt::format_args args) {
    return vprintln({}, fmt, args);
  }

  int vprintln(fmt::locale_ref locale, fmt::string_view fmt, fmt::format_args args);

  virtual int write(std::string_view data) = 0;

  int write(std::string_view data, size_t offset, size_t n = std::string_view::npos) {
    return write(data.substr(offset, n));
  }

  int writeln(std::string_view data);

  int writeln(std::string_view data, size_t offset, size_t n = std::string_view::npos);

protected:

  int error_ = 0;
  bool open_ = true;
};

// `FileSink` -----------------------------------------------------------------------------------------------

struct FileSink : Sink {
  FileSink(FILE* file, bool closeOnDestroy = true);

  FileSink(const std::string& path, bool closeOnDestroy = true);

  virtual ~FileSink() override;

  virtual int close() override;

  virtual int flush() override;

  bool stderr() const { return file_ == ::stderr; }

  bool stdout() const { return file_ == ::stdout; }

  virtual int write(std::string_view data) override;

private:

  FILE* file_;
  bool closeOnDestroy_;
};

// `NullSink` -----------------------------------------------------------------------------------------------

struct NullSink : Sink {
  virtual int close() override;

  virtual int flush() override;

  virtual int write(std::string_view data) override;
};

// `StreamSink` ---------------------------------------------------------------------------------------------

struct StreamSink : Sink {
  StreamSink(std::ostream& os) : os_(os) {}

  virtual int close() override;

  virtual int flush() override;

  virtual int write(std::string_view data) override;

private:

  std::ostream& os_;
};

// `StringSink` ---------------------------------------------------------------------------------------------

struct StringSink : Sink {
  StringSink(std::string& buf) : buf_(buf) {}

  virtual int close() override;

  virtual int flush() override;

  virtual int write(std::string_view data) override;

private:

  std::string& buf_;
};

// Variables ------------------------------------------------------------------------------------------------

extern FileSink stderr;

extern FileSink stdout;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Returns a file descriptor for a #rocket::nio::Sink.
 *
 * @param sink the sink
 * @return `STDOUT_FILENO`, `STDERR_FILENO`, or -1 if a file descriptor cannot be determined
 */
int fd(const nio::Sink& sink);

/**
 * Returns `true` if the sink @p sink is connected to a terminal.
 *
 * @param sink the sink
 * @return `true` if @p sink is connected to a terminal
 */
bool isatty(const nio::Sink& sink);

} // namespace rocket::nio

// EOF
