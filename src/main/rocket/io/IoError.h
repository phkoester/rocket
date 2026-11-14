/**
 * @file IoError.h
 *
 * An exception indicating an I/O error.
 */

#pragma once

#include "rocket/Exception.h"

namespace rocket::io {

// `IoError` ------------------------------------------------------------------------------------------------

/**
 * An exception indicating an I/O error, e.g., when reading from or writing to a device fails.
 */
struct IoError : Exception, std::runtime_error {
  /// @type_base
  using Base = std::runtime_error;

  /**
   * @ctor
   *
   * @param msg the message
   * @param sl the source location
   * @param st the stack trace
   */
  explicit IoError(
    std::string_view msg,
    const std::optional<std::source_location>& sl = ROCKET_EXCEPTION_SL,
    const std::optional<stacktrace>& st = ROCKET_EXCEPTION_ST);

  ~IoError() override = default;
};

} // namespace rocket::io

// EOF
