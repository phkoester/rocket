/**
 * @file Exception.h
 *
 * A collection of basic exceptions.
 */

#pragma once

#include "rocket/format/format.h"
#include "rocket/rocket.h" // #std::type_info for MSVC
#include "rocket/nio/nio-fwd.h"
#include "rocket/unicode/ConvertTo.h"

#include <optional>
#include <source_location>
#include <stacktrace>
#include <stdexcept>
#include <typeinfo>

// Macros ---------------------------------------------------------------------------------------------------

/// The current source location.
#define ROCKET_EXCEPTION_SL ::std::source_location::current()

/// The current stack trace.
#define ROCKET_EXCEPTION_ST ::std::stacktrace::current()

#ifdef NDEBUG

/// The current source location, or null if `NDEBUG` is defined.
#define ROCKET_DEBUG_EXCEPTION_SL ::std::nullopt

/// The current source location, or null if `NDEBUG` is defined.
#define ROCKET_DEBUG_EXCEPTION_ST ::std::nullopt

#else

/// The current source location, or null if `NDEBUG` is defined.
#define ROCKET_DEBUG_EXCEPTION_SL ROCKET_EXCEPTION_SL

/// The current source location, or null if `NDEBUG` is defined.
#define ROCKET_DEBUG_EXCEPTION_ST ROCKET_EXCEPTION_ST

#endif // NDEBUG

namespace rocket {

// #Exception -----------------------------------------------------------------------------------------------

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
  virtual ~Exception() = default;

  /**
   * Returns the plain message of this exception, not including any source-location information.
   *
   * @return a message
   */
  [[nodiscard]] const std::string& message() const { return msg_; }

  /**
   * Returns the source location from which this exception was thrown.
   *
   * @return a source location, if present, or null otherwise
   */
  [[nodiscard]] const std::optional<std::source_location>& sourceLocation() const { return sl_; }

  /**
   * Returns the stack trace of this exception.
   *
   * @return a stack trace, if present, or null otherwise
   */
  [[nodiscard]] const std::optional<std::stacktrace>& stackTrace() const { return st_; }

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
      const std::optional<std::source_location>& sl,
      const std::optional<std::stacktrace>& st) :
      msg_(msg),
      sl_(sl),
      st_(st) {}

private:

  std::string msg_;
  std::optional<std::source_location> sl_;
  std::optional<std::stacktrace> st_;
};

// #InvalidArgument -----------------------------------------------------------------------------------------

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
      const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
      const std::optional<std::stacktrace>& st = ROCKET_EXCEPTION_ST);

  ~InvalidArgument() override = default;
};

// #InvalidState --------------------------------------------------------------------------------------------

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
      const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
      const std::optional<std::stacktrace>& st = ROCKET_EXCEPTION_ST);

  ~InvalidState() override = default;
};

// #Overflow ------------------------------------------------------------------------------------------------

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
      const std::type_info& type,
      const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
      const std::optional<std::stacktrace>& st = ROCKET_EXCEPTION_ST) :
      Overflow(type, "", sl, st) {}

  /**
   * @ctor
   *
   * @param type the type
   * @param msg additional message
   * @param sl the source location
   * @param st the stack trace
   */
  Overflow(
    const std::type_info& type,
    std::string_view msg,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<std::stacktrace>& st = ROCKET_EXCEPTION_ST);

  ~Overflow() override = default;
};

// #Underflow ------------------------------------------------------------------------------------------------

/**
 * An exception indicating a type underflow.
 */
struct Underflow : Exception, std::underflow_error {
  /// @type_base
  using Base = std::underflow_error;

  /**
   * @ctor
   *
   * @param type the type
   * @param sl the source location
   * @param st the stack trace
   */
  explicit Underflow(
      const std::type_info& type,
      const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
      const std::optional<std::stacktrace>& st = ROCKET_EXCEPTION_ST) :
      Underflow(type, "", sl, st) {}

  /**
   * @ctor
   *
   * @param type the type
   * @param msg additional message
   * @param sl the source location
   * @param st the stack trace
   */
  Underflow(
    const std::type_info& type,
    std::string_view msg,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<std::stacktrace>& st = ROCKET_EXCEPTION_ST);

  ~Underflow() override = default;
};

// #WrappedException ----------------------------------------------------------------------------------------

/**
 * An exception wrapper which has its own #fmt::formatter specialization.
 */
struct WrappedException {
  explicit WrappedException(const std::exception& ex) : ex_(&ex) {}

  explicit WrappedException(std::exception_ptr ptr) : ptr_(ptr) {}

  const std::exception* exception() const { return ex_; }

  std::exception_ptr ptr() const { return ptr_; }

private:

  const std::exception* ex_ = nullptr;
  std::exception_ptr ptr_;
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

} // namespace rocket

// #fmt::formatter<#rocket::WrappedException> ---------------------------------------------------------------

/**
 * @spec_fmt_formatter{#rocket::WrappedException}
 *
 * - If the `?` format specifier is used, then the stack trace is included.
 * - If the `t` format specifier is used, then the type of the exception is included.
 */
template<typename C>
struct fmt::formatter<rocket::WrappedException, C> {
  /// @cond undocumented

  template<typename FormatContext>
  FormatContext::iterator
  format(const rocket::WrappedException& val, FormatContext& ctx) const{
    // If requested, append type

    auto out = ctx.out();
    if (withType_) {
      const std::type_info* type; // NOLINT
      if (val.exception()) {
        type = &typeid(*val.exception());
      } else {
#ifdef ROCKET_COMPILER_MSVC
        type = nullptr;
#else
        type = val.ptr().__cxa_exception_type();
#endif
      }

      if (type != nullptr) {
        const std::string typeName = fmt::format("{}", *type);
        if constexpr (std::is_same_v<C, char>) {
          out = format_to(out, "`{}`: ", rocket::unicode::ConvertTo<C>::apply(typeName));
        } else {
          out = format_to(out, U"`{}`: ", rocket::unicode::ConvertTo<C>::apply(typeName));
        }
      }
    }

    // Append message

    std::string what;
    if (val.exception()) {
      what = rocket::what(*val.exception());
    } else {
      what = rocket::what(val.ptr());
    }
    out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(what));

    // If debug, append stack trace

    if (debug_) {
      const auto* const p = dynamic_cast<const rocket::Exception*>(val.exception());
      if (p && p->stackTrace()) {
        out = detail::write<C>(out, static_cast<C>('\n'));
        std::ostringstream os;
        os << *p->stackTrace();
        std::string str = os.str();
        str.pop_back(); // Remove trailing '\n'
        out = detail::write<C>(out, rocket::unicode::ConvertTo<C>::apply(str));
      }
    }
    return out;
  }

  constexpr const C*
  parse(parse_context<C>& ctx) {
    auto it = ctx.begin();
    auto end = ctx.end();
    if (it != end && *it == '?') {
      debug_ = true;
      ++it;
    }
    if (it != end && *it == 't') {
      withType_ = true;
      ++it;
    }
    return it;
  }

  constexpr void
  set_debug_format(bool val = true) {
    debug_ = val;
  }

  /// @endcond

private:

  bool debug_ = false;
  bool withType_ = false;
};

// EOF
