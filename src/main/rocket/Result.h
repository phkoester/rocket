/**
 * @file Result.h
 *
 * A general-purpose result, to be used with `std::expected`.
 */

#pragma once

#include <expected>
#include <string>

namespace rocket {

// `Error` --------------------------------------------------------------------------------------------------

/// A general-purpose error, carrying a message.
struct Error {
  std::string message; ///< The error message.
};

// `Result` -------------------------------------------------------------------------------------------------

/// A general-purpose result, to be used with `std::expected`.
template<typename T>
using Result = std::expected<T, Error>;

} // namespace rocket

// EOF
