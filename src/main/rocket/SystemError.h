/// @file SystemError.h
///
/// An exception indicating a system error.

#pragma once

#include "rocket/Exception.h"

#include <cerrno>
#include <fstream>

namespace rocket {

// `SystemError` --------------------------------------------------------------------------------------------

/// An exception indicating a system error, i.e., an error that comes with a `std::error_code`, e.g., when
/// opening a file or reading from or writing to a device fails.
struct SystemError : Exception, std::system_error {
  /// @ctor
  ///
  /// @param ec the error code
  /// @param sl the source location
  /// @param st the stack trace
  explicit SystemError(
    const std::error_code& ec,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<stacktrace>& st = ROCKET_EXCEPTION_ST) :
    SystemError(ec, "system error", sl, st) {}

  /// @ctor
  ///
  /// @param ec the error code
  /// @param msg the message
  /// @param sl the source location
  /// @param st the stack trace
  SystemError(
    const std::error_code& ec,
    std::string_view msg,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<stacktrace>& st = ROCKET_EXCEPTION_ST);

  ~SystemError() override = default;
};

// Functions ------------------------------------------------------------------------------------------------

/// Returns the error code for @p errc.
///
/// @param errc a #std::errc value
/// @return an error code
inline std::error_code
errorCode(std::errc errc) {
  return std::make_error_code(errc);
}

/// Returns the error code for @p errno__, or #std::errc::io_error if @p errno__ is not set.
///
/// @param errno__ the error number
/// @return an error code
inline std::error_code
errorCode(int errno__) {
  return errno__ != 0 ?
    std::error_code(errno__, std::generic_category()) :
    errorCode(std::errc::io_error);
}

/// Returns the error code for a failure of the stream buffer of @p s.
///
/// Only a #std::filebuf leaves a meaningful `errno` behind. Any other stream buffer may have thrown, and the
/// stream swallowed it, and the exception runtime may have left a stale `errno` then, so this is
/// #std::errc::io_error.
///
/// @param s the stream
/// @return an error code
inline std::error_code
errorCode(const std::ios& s) {
  return dynamic_cast<const std::filebuf*>(s.rdbuf()) != nullptr ?
    errorCode(errno) :
    errorCode(std::errc::io_error);
}

} // namespace rocket

// EOF
