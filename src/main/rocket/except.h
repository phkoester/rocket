/**
 * @file except.h
 *
 * A collection of basic exceptions.
 */

#pragma once

#include "Type.h"
#include "io-decl.h"
#include "text.h"

#include <stacktrace>
#include <stdexcept>

// Macros ---------------------------------------------------------------------------------------------------

#ifdef NDEBUG
  #define ROCKET_EXCEPT_DEFAULT_SOURCE_LOC ::std::nullopt
  #define ROCKET_EXCEPT_DEFAULT_STACK_TRACE ::std::nullopt
#else
  /**
   * Yields null if `NDEBUG` is defined, the current source location otherwise.
   */
  #define ROCKET_EXCEPT_DEFAULT_SOURCE_LOC ::std::source_location::current()
  /**
   * Yields null if `NDEBUG` is defined, the current stack trace otherwise.
   */
  #define ROCKET_EXCEPT_DEFAULT_STACK_TRACE ::std::stacktrace::current()
#endif // NDEBUG

namespace rocket::except {

// Messages -------------------------------------------------------------------------------------------------

namespace message {

/**
 * Makes a message to be passed to the standard base classes.
 *
 * @param msg the exception message
 * @param sourceLoc the source location
 * @return a message
 */
std::string baseMessage(std::string_view msg, const std::optional<std::source_location>& sourceLoc);

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
  const std::optional<std::source_location>& sourceLocation() const { return sourceLoc_; }

  /**
   * Returns the stack trace of this exception.
   *
   * @return a stack trace, if present, or null otherwise
   */
  const std::optional<std::stacktrace>& stackTrace() const { return stackTrace_; }

protected:

  /**
   * @ctor
   *
   * @param msg the plain message
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  Exception(
      std::string_view msg,
      std::optional<std::source_location>&& sourceLoc,
      std::optional<std::stacktrace>&& stackTrace) :
      msg_(msg),
      sourceLoc_(std::move(sourceLoc)),
      stackTrace_(std::move(stackTrace)) {}

private:

  const std::string msg_;
  const std::optional<std::source_location> sourceLoc_;
  const std::optional<std::stacktrace> stackTrace_;
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
  using BaseType = std::ios_base::failure;

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * The stored position is set to the value of `rocket::io::tellg(is)`.
   *
   * @param is the input stream
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  explicit InputFailure(
      std::basic_istream<C>& is,
      std::optional<std::source_location>&& sourceLoc = ROCKET_EXCEPT_DEFAULT_SOURCE_LOC,
      std::optional<std::stacktrace>&& stackTrace = ROCKET_EXCEPT_DEFAULT_STACK_TRACE) :
      InputFailure(is, io::tellg(is), std::move(sourceLoc), std::move(stackTrace)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  explicit InputFailure(
      std::basic_istream<C>& is,
      size_t position,
      std::optional<std::source_location>&& sourceLoc = ROCKET_EXCEPT_DEFAULT_SOURCE_LOC,
      std::optional<std::stacktrace>&& stackTrace = ROCKET_EXCEPT_DEFAULT_STACK_TRACE) :
      InputFailure(is, position, "Input failure", std::move(sourceLoc), std::move(stackTrace)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * The stored position is set to the value of `rocket::io::tellg(is)`.
   *
   * @param is the input stream
   * @param msg the message
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  InputFailure(
      std::basic_istream<C>& is,
      std::string_view msg,
      std::optional<std::source_location>&& sourceLoc = ROCKET_EXCEPT_DEFAULT_SOURCE_LOC,
      std::optional<std::stacktrace>&& stackTrace = ROCKET_EXCEPT_DEFAULT_STACK_TRACE) :
      InputFailure(is, io::tellg(is), msg, std::move(sourceLoc), std::move(stackTrace)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param msg the message
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  InputFailure(
      std::basic_istream<C>& is,
      size_t position,
      std::string_view msg,
      std::optional<std::source_location>&& sourceLoc = ROCKET_EXCEPT_DEFAULT_SOURCE_LOC,
      std::optional<std::stacktrace>&& stackTrace = ROCKET_EXCEPT_DEFAULT_STACK_TRACE) :
      BaseType(message::baseMessage(msg, sourceLoc)),
      Exception(msg, std::move(sourceLoc), std::move(stackTrace)),
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
  using BaseType = std::invalid_argument;

  /**
   * @ctor
   *
   * @param name the name of the argument
   * @param msg the message
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  InvalidArgument(
      std::string_view name,
      std::string_view msg,
      std::optional<std::source_location>&& sourceLoc = ROCKET_EXCEPT_DEFAULT_SOURCE_LOC,
      std::optional<std::stacktrace>&& stackTrace = ROCKET_EXCEPT_DEFAULT_STACK_TRACE);
};

// `InvalidState` -------------------------------------------------------------------------------------------

/**
 * An exception indicating an invalid state.
 */
struct InvalidState : std::runtime_error, Exception {
  /// @type_base
  using BaseType = std::runtime_error;

  /**
   * @ctor
   *
   * @param msg the message
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  explicit InvalidState(
      std::string_view msg,
      std::optional<std::source_location>&& sourceLoc = ROCKET_EXCEPT_DEFAULT_SOURCE_LOC,
      std::optional<std::stacktrace>&& stackTrace = ROCKET_EXCEPT_DEFAULT_STACK_TRACE);
};

// `ParseFailure` -------------------------------------------------------------------------------------------

/**
 * Instances of this class are thrown when parsing from an input stream fails.
 *
 * @tparam C the character type
 */
template<typename C> requires Character<C>
struct ParseFailure : InputFailure<C> {
  /// @type_base
  using BaseType = InputFailure<C>;

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param msg the message
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  ParseFailure(
      std::basic_istream<C>& is,
      size_t position,
      std::string_view msg,
      std::optional<std::source_location>&& sourceLoc = ROCKET_EXCEPT_DEFAULT_SOURCE_LOC,
      std::optional<std::stacktrace>&& stackTrace = ROCKET_EXCEPT_DEFAULT_STACK_TRACE) :
      ParseFailure(is, position, {}, msg, std::move(sourceLoc), std::move(stackTrace)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param range the range to store
   * @param msg the message
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  ParseFailure(
      std::basic_istream<C>& is,
      size_t position,
      text::Range range,
      std::string_view msg,
      std::optional<std::source_location>&& sourceLoc = ROCKET_EXCEPT_DEFAULT_SOURCE_LOC,
      std::optional<std::stacktrace>&& stackTrace = ROCKET_EXCEPT_DEFAULT_STACK_TRACE) :
      ParseFailure(is, position, { range }, msg, std::move(sourceLoc), std::move(stackTrace)) {}

  /**
   * @ctor
   *
   * @attention Unless it is set already, this constructor sets the fail bit of the input stream.
   *
   * @param is the input stream
   * @param position the position to store
   * @param ranges the ranges to store
   * @param msg the message
   * @param sourceLoc the source location
   * @param stackTrace the stack trace
   */
  ParseFailure(
      std::basic_istream<C>& is,
      size_t position,
      std::initializer_list<text::Range> ranges,
      std::string_view msg,
      std::optional<std::source_location>&& sourceLoc = ROCKET_EXCEPT_DEFAULT_SOURCE_LOC,
      std::optional<std::stacktrace>&& stackTrace = ROCKET_EXCEPT_DEFAULT_STACK_TRACE):
      BaseType(is, position, msg, std::move(sourceLoc), std::move(stackTrace)),
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

// Functions ------------------------------------------------------------------------------------------------

/**
 * Prints detailed information about the exception @p ex to the output stream @p os.
 *
 * If there are nested exceptions, the whole exception hierarchy is processed. The output may span multiple
 * lines, the last printed character is always <code>'\\n'</code>.
 *
 * @param os the output stream to print to
 * @param ex the exception
 */
void printException(std::ostream& os, const std::exception& ex);

/**
 * Prints detailed information about the exception pointer @p ex to the output stream @p os.
 *
 * If there are nested exceptions, the whole exception hierarchy is processed. The output may span multiple
 * lines, the last printed character is always <code>'\\n'</code>.
 *
 * @param os the output stream to print to
 * @param ptr the exception pointer. May not be null
 */
void printException(std::ostream& os, std::exception_ptr ptr);

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
