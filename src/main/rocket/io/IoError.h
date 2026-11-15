/**
 * @file IoError.h
 *
 * An exception indicating an I/O error.
 */

#pragma once

#include "rocket/Exception.h"

#include <cerrno>

namespace rocket::io {

// `IoError` ------------------------------------------------------------------------------------------------

/**
 * An exception indicating an I/O error, e.g., when reading from or writing to a device fails.
 */
struct IoError : Exception, std::system_error {
  /**
   * @ctor
   *
   * @param ec the error code
   * @param sl the source location
   * @param st the stack trace
   */
  explicit IoError(
    const std::error_code& ec,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<stacktrace>& st = ROCKET_EXCEPTION_ST) :
    IoError(ec, "I/O error", sl, st) {}

  /**
   * @ctor
   *
   * @param ec the error code
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  IoError(
    const std::error_code& ec,
    std::string_view msg,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<stacktrace>& st = ROCKET_EXCEPTION_ST);

  ~IoError() override = default;
};

} // namespace rocket::io

// EOF
