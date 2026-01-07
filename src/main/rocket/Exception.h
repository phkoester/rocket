/**
 * @file Exception.h
 *
 * A collection of basic exceptions.
 */

#pragma once

#include "rocket/Type.h"
#include "rocket/nio/nio-fwd.h"

#include <optional>
#include <source_location>
#include <stacktrace>
#include <stdexcept>

// Macros ---------------------------------------------------------------------------------------------------

#ifdef NDEBUG
  #define ROCKET_EXCEPTION_SL ::std::nullopt
  #define ROCKET_EXCEPTION_ST ::std::nullopt
#else
  /**
   * Yields null if `NDEBUG` is defined, the current source location otherwise.
   */
  #define ROCKET_EXCEPTION_SL ::std::source_location::current()
  /**
   * Yields null if `NDEBUG` is defined, the current stack trace otherwise.
   */
  #define ROCKET_EXCEPTION_ST ::std::stacktrace::current()
#endif // NDEBUG

namespace rocket {

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
  /// @dtor
  virtual ~Exception() {}

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

// `InvalidArgument` ----------------------------------------------------------------------------------------

/**
 * An exception indicating an invalid argument.
 */
struct InvalidArgument : Exception, std::invalid_argument {
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
      std::optional<std::source_location>&& sl = ROCKET_EXCEPTION_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPTION_ST);

  ~InvalidArgument() override {}
};

// `InvalidState` -------------------------------------------------------------------------------------------

/**
 * An exception indicating an invalid state.
 */
struct InvalidState : Exception, std::runtime_error {
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
      std::optional<std::source_location>&& sl = ROCKET_EXCEPTION_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPTION_ST);

  ~InvalidState() override {}
};

// `Overflow` -----------------------------------------------------------------------------------------------

/**
 * An exception indicating a type overflow.
 */
struct Overflow : Exception, std::overflow_error {
  /// @type_base
  using Base = std::overflow_error;

  /**
   * @ctor
   *
   * @param type the type
   * @param sl the source location
   * @param st the stack trace
   */
  explicit Overflow(
      const Type& type,
      std::optional<std::source_location>&& sl = ROCKET_EXCEPTION_SL,
      std::optional<std::stacktrace>&& st = ROCKET_EXCEPTION_ST) :
      Overflow(type, "", std::move(sl), std::move(st)) {}

  /**
   * @ctor
   *
   * @param type the type
   * @param msg additional message
   * @param sl the source location
   * @param st the stack trace
   */
  Overflow(
    const Type& type,
    std::string_view msg,
    std::optional<std::source_location>&& sl = ROCKET_EXCEPTION_SL,
    std::optional<std::stacktrace>&& st = ROCKET_EXCEPTION_ST);

    ~Overflow() override {}
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Prints detailed information about the exception @p ex to the sink @p sink.
 *
 * If there are nested exceptions, the whole exception hierarchy is processed. The output may span multiple
 * lines, the last printed character is always <code>'\\n'</code>.
 *
 * @param out the sink to print to
 * @param ex the exception
 */
void printException(nio::Sink& out, const std::exception& ex);

/**
 * Prints detailed information about the exception pointer @p ex to the sink @p sink.
 *
 * If there are nested exceptions, the whole exception hierarchy is processed. The output may span multiple
 * lines, the last printed character is always <code>'\\n'</code>.
 *
 * @param out the sink to print to
 * @param ptr the exception pointer. May not be null
 */
void printException(nio::Sink& out, std::exception_ptr ptr);

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

} // namespace rocke

// EOF
