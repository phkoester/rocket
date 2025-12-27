/**
 * @file except.h
 *
 * A collection of basic exceptions.
 */

#pragma once

#define ROCKET_EXCEPT_H

#include "Type.h"
#include "io-decl.h"
#include "nio.h"
#include "text.h"

#include <source_location>
#include <stacktrace>
#include <stdexcept>

// Macros ---------------------------------------------------------------------------------------------------

#ifdef NDEBUG
  #define ROCKET_EXCEPT_SL ::std::nullopt
  #define ROCKET_EXCEPT_ST ::std::nullopt
#else
  /**
   * Yields null if `NDEBUG` is defined, the current source location otherwise.
   */
  #define ROCKET_EXCEPT_SL ::std::source_location::current()
  /**
   * Yields null if `NDEBUG` is defined, the current stack trace otherwise.
   */
  #define ROCKET_EXCEPT_ST ::std::stacktrace::current()
#endif // NDEBUG

namespace rocket::except {

// Messages -------------------------------------------------------------------------------------------------

namespace message {

/**
 * Makes a message to be passed to the standard base classes.
 *
 * @param msg the exception message
 * @param sl the source location
 * @return a message
 */
std::string baseMessage(std::string_view msg, const std::optional<std::source_location>& sl);

/**
  * Makes a message saying the input @p input cannot be parsed as a value of type @p type.
  *
  * @param input the input
  * @param type a #rocket::Type value
  * @return a message
  */
std::string cannotParseAs(std::string_view input, const Type& type);

/**
  * Makes a message saying there is an overflow for type @p type.
  *
  * @param type a #rocket::Type value
  * @return a message
  */
std::string overflow(const Type& type);

} // namespace message

// `Exception` ----------------------------------------------------------------------------------------------

/**
 * A simple class that all Rocket exceptions derive from in addition to their base classes.
 *
 * It holds
 *
 * - a plain message (without source-location information),
 * - an optional source location,
 * - and an optional stack trace.
 */
struct Exception {
  /**
   * Returns the plain message of this exception, not including any source-location information.
   *
   * @return a message
   */
  const std::string& message() const { return msg_; }

  /**
   * Returns the source location from which this exception was thrown.
   *
   * @return a source location, if present, or null otherwise
   */
  const std::optional<std::source_location>& sourceLocation() const { return sl_; }

  /**
   * Returns the stack trace of this exception.
   *
   * @return a stack trace, if present, or null otherwise
   */
  const std::optional<std::stacktrace>& stackTrace() const { return st_; }

protected:

  /**
   * @ctor
   *
   * @param msg the plain message
   * @param sl the source location
   * @param st the stack trace
   */
  Exception(
      std::string_view msg,
      std::optional<std::source_location>&& sl,
      std::optional<std::stacktrace>&& st) :
      msg_(msg),
      sl_(std::move(sl)),
      st_(std::move(st)) {}

private:

  const std::string msg_;
  const std::optional<std::source_location> sl_;
  const std::optional<std::stacktrace> st_;
};

// `InputFailure` -------------------------------------------------------------------------------------------

/**
 * Instances of this class are thrown when reading from an input stream failed.
 *
 * @tparam C the character type
 */
template<typename C> requires Character<C>
struct InputFailure : std::ios_base::failure, Exception {
  /// @type_base
  using Base = std::ios_base::failure;

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * The stored position is set to the value of `rocket::io::tellg(is)`.
   *
   * @param is the input stream
   * @param sl the source location
   * @param st the stack trace
   */
  explicit InputFailure(
      std::basic_istream<C>& is,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      InputFailure(is, io::tellg(is), std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param sl the source location
   * @param st the stack trace
   */
  explicit InputFailure(
      std::basic_istream<C>& is,
      size_t position,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      InputFailure(is, position, "Input failure", std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * The stored position is set to the value of `rocket::io::tellg(is)`.
   *
   * @param is the input stream
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InputFailure(
      std::basic_istream<C>& is,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      InputFailure(is, io::tellg(is), msg, std::move(sl), std::move(sl)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InputFailure(
      std::basic_istream<C>& is,
      size_t position,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      Base(message::baseMessage(msg, sl)),
      Exception(msg, std::move(sl), std::move(st)),
      pos_(position) {
    if (not is.fail())
      is.setstate(std::ios::failbit);
  }

  /**
   * Returns the stored position.
   *
   * @return the stored position
   */
  size_t position() const { return pos_; }

private:

  const size_t pos_;
};

// `InvalidArgument` ----------------------------------------------------------------------------------------

/**
 * An exception indicating an invalid argument.
 */
struct InvalidArgument : std::invalid_argument, Exception {
  /// @type_base
  using Base = std::invalid_argument;

  /**
   * @ctor
   *
   * @param name the name of the argument
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  InvalidArgument(
      std::string_view name,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST);
};

template<typename... T>
[[noreturn]] void
throwInvalidArgument(
    const std::source_location& sl,
    const char* name,
    fmt::format_string<T...> fmt,
    T&&... args) {
  nio::StringSink msg;
  msg.print(fmt, std::forward<T>(args)...);
  throw InvalidArgument(name, msg.str(), sl);
}

// `InvalidState` -------------------------------------------------------------------------------------------

/**
 * An exception indicating an invalid state.
 */
struct InvalidState : std::runtime_error, Exception {
  /// @type_base
  using Base = std::runtime_error;

  /**
   * @ctor
   *
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  explicit InvalidState(
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST);
};

template<typename... T>
[[noreturn]] void
throwInvalidState(
    const std::source_location& sl,
    fmt::format_string<T...> fmt,
    T&&... args) {
  nio::StringSink msg;
  msg.print(fmt, std::forward<T>(args)...);
  throw InvalidState(msg.str(), sl);
}

// `ParseFailure` -------------------------------------------------------------------------------------------

/**
 * Instances of this class are thrown when parsing from an input stream fails.
 *
 * @tparam C the character type
 */
template<typename C> requires Character<C>
struct ParseFailure : InputFailure<C> {
  /// @type_base
  using Base = InputFailure<C>;

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  ParseFailure(
      std::basic_istream<C>& is,
      size_t position,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      ParseFailure(is, position, {}, msg, std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param range the range to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  ParseFailure(
      std::basic_istream<C>& is,
      size_t position,
      text::Range range,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST) :
      ParseFailure(is, position, { range }, msg, std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param ranges the ranges to store
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  ParseFailure(
      std::basic_istream<C>& is,
      size_t position,
      std::initializer_list<text::Range> ranges,
      std::string_view msg,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPT_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPT_ST):
      Base(is, position, msg, std::move(sl), std::move(st)),
      ranges_(ranges) {}

  /**
   * Returns the stored position ranges.
   *
   * @return the stored position ranges
   */
  const text::Ranges& ranges() const { return ranges_; }

private:

  const text::Ranges ranges_;
};

template<typename C, typename... T>
[[noreturn]] void
throwParseFailure(
    const std::source_location& sl,
    std::basic_istream<C>& is,
    size_t position,
    fmt::format_string<T...> fmt,
    T&&... args) {
  nio::StringSink msg;
  msg.print(fmt, std::forward<T>(args)...);
  throw ParseFailure(is, position, msg.str(), sl);
}

template<typename C, typename... T>
[[noreturn]] void
throwParseFailure(
    const std::source_location& sl,
    std::basic_istream<C>& is,
    size_t position,
    text::Range range,
    fmt::format_string<T...> fmt,
    T&&... args) {
  nio::StringSink msg;
  msg.print(fmt, std::forward<T>(args)...);
  throw ParseFailure(is, position, range, msg.str(), sl);
}

template<typename C, typename... T>
[[noreturn]] void
throwParseFailure(
    const std::source_location& sl,
    std::basic_istream<C>& is,
    size_t position,
    std::initializer_list<text::Range> ranges,
    fmt::format_string<T...> fmt,
    T&&... args) {
  nio::StringSink msg;
  msg.print(fmt, std::forward<T>(args)...);
  throw ParseFailure(is, position, ranges, msg.str(), sl);
}

// Functions ------------------------------------------------------------------------------------------------

/**
 * Prints detailed information about the exception @p ex to the sink @p sink.
 *
 * If there are nested exceptions, the whole exception hierarchy is processed. The output may span multiple
 * lines, the last printed character is always <code>'\\n'</code>.
 *
 * @param sink the sink to print to
 * @param ex the exception
 */
void printException(nio::Sink& sink, const std::exception& ex);

/**
 * Prints detailed information about the exception pointer @p ex to the sink @p sink.
 *
 * If there are nested exceptions, the whole exception hierarchy is processed. The output may span multiple
 * lines, the last printed character is always <code>'\\n'</code>.
 *
 * @param sink the sink to print to
 * @param ptr the exception pointer. May not be null
 */
void printException(nio::Sink& sink, std::exception_ptr ptr);

/**
 * Extracts the `what` message from an exception.
 *
 * If there are nested exceptions, the whole exception hierarchy is processed.
 *
 * @param ex the exception
 * @return a string
 */
std::string what(const std::exception& ex);

/**
 * Extracts the `what` message from an exception pointer.
 *
 * If there are nested exceptions, the whole exception hierarchy is processed.
 *
 * @param ptr the exception pointer. May not be null
 * @return a string
 */
std::string what(std::exception_ptr ptr);

} // namespace rocket::except

// EOF
