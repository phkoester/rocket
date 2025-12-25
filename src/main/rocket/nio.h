/**
 * @file nio.h
 *
 * New I/O.
 */

#pragma once

#include "basic.h"
#include "strings.h"

#include <fmt/format.h>

#include <functional>
#include <ostream>
#include <string>

namespace rocket::nio {

// `Format` -------------------------------------------------------------------------------------------------

struct Format {
  struct Params {
    std::string formatted_;
    std::unordered_map<std::string_view, std::string> tagged_;

    template<typename... T>
    void set(fmt::format_string<T...> fmt, T&&... args) {
      formatted_ = fmt::format(fmt, std::forward<T>(args)...);
    }

    template<typename... T>
    void set(const std::locale& locale,fmt::format_string<T...> fmt, T&&... args) {
      formatted_ = fmt::format(locale, fmt, std::forward<T>(args)...);
    }

    template<typename... T>
    void tag(std::string_view tag, fmt::format_string<T...> fmt, T&&... args) {
      tagged_.emplace(tag, fmt::format(fmt, std::forward<T>(args)...));
    }

    template<typename... T>
    void tag(std::string_view tag, const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
      tagged_.emplace(tag, fmt::format(locale, fmt, std::forward<T>(args)...));
    }
  };

  using ParamsProducer = std::function<Params()>;

  static Params params() {
    return {};
  }

  template<typename... T>
  static Params params(fmt::format_string<T...> fmt, T&&... args) {
    Params ret;
    ret.set(fmt, std::forward<T>(args)...);
    return ret;
  }

  template<typename... T>
  static Params params(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    Params ret;
    ret.set(locale, fmt, std::forward<T>(args)...);
    return ret;
  }

  Format(ParamsProducer&& f) : params_(f()) {}

  const Params& get() const { return params_; }

private:

  Params params_;
};

} // namespace rocket::nio

/// @spec_fmt_formatter{#rocket::nio::Format)
template<>
struct fmt::formatter<rocket::nio::Format> {
  template<typename FormatContext>
  constexpr auto format(const rocket::nio::Format& v, FormatContext& ctx) const {
    const auto& params = v.get();
    auto formatted = params.formatted_;
    for (const auto& [tag, value] : params.tagged_) {
      rocket::strings::replaceIn<char>(formatted, tag, value);
    }
    return format_to(ctx.out(), "{}", formatted);
  }

  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }
};

namespace rocket::nio {

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
    write("\n");
    return flush();
  }

  template<typename... T>
  int println(const std::locale& locale, fmt::format_string<T...> fmt, T&&... args) {
    print(locale, fmt, std::forward<T>(args)...);
    write("\n");
    return flush();
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

// `FileSink` -----------------------------------------------------------------------------------------------

struct FileSink : Sink {
  struct Params {
    bool append = false;
    bool closeOnDestroy = true;
  };

  FileSink(FILE* file, const Params& params);

  FileSink(const std::string& path, const Params& params);

  virtual ~FileSink() override;

  virtual int close() override;

  virtual int flush() override;

  bool stderr() const { return file_ == ::stderr; }

  bool stdout() const { return file_ == ::stdout; }

  virtual int write(std::string_view data) override;

ROCKET_PRIVATE:

  FILE* file_;
  Params params_;
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
