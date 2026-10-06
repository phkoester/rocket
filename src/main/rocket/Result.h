/**
 * @file Result.h
 *
 * A general-purpose result.
 */

#pragma once

#include <expected>
#include <string>

namespace rocket {

// `Error` --------------------------------------------------------------------------------------------------

/**
 * A general-purpose error, carrying a message.
 *
 * This may serve as a base class for more specific errors.
 */
struct Error {
  /// The error message.
  std::string message;

  /**
   * @ctor
   *
   * @param message the error message
   */
  explicit Error(std::string_view message) : message(message) {}

  /// @dtor
  virtual ~Error() = default;

  /// @member_op_eq
  bool operator==(const Error& rhs) const = default;
};

// `Result` -------------------------------------------------------------------------------------------------

/// The result type with #rocket::Error.
template<typename T>
using Result = std::expected<T, Error>;

} // namespace rocket

// EOF
