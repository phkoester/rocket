/**
 * @file system.h
 *
 * System-dependent functions, access to the environment.
 */

#pragma once

#include "rocket/Result.h"
#include "rocket/format.h"
#include "rocket/str/StringConvert.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace rocket::system {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::optional<std::string> getImpl(std::string_view name);

void setImpl(std::string_view name, const std::optional<std::string>& val, bool replace);

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

/**
 * Executes @p cl and captures the output to standard out.
 *
 * @param cl the command line to execute
 * @return the captured output
 * @throw #rocket::InvalidState if the command cannot be executed
 */
std::vector<char> exec(const std::string& cl);

/**
 * Executes a command and captures the output to standard out.
 *
 * @param args the command line to execute
 * @return the captured output
 * @throw #rocket::InvalidState if the command cannot be executed
 */
std::vector<char> exec(const std::vector<std::string_view>& args);

/**
 * Returns the system-dependent executable suffix.
 *
 * @return the executable suffix
 */
consteval std::string_view
executableSuffix() {
  using namespace std;
#ifdef ROCKET_OS_WINDOWS
  return ".exe"sv;
#else
  return {};
#endif
}

/**
 * Returns the system-dependent file separator.
 *
 * @return the file separator
 */
consteval char
fileSeparator() {
#ifdef ROCKET_OS_WINDOWS
  return '\\';
#else
  return '/';
#endif
}

/**
 * Converts a command-line string to a vector of arguments.
 *
 * @param cl the command-line string
 * @return a vector of arguments
 */
std::vector<std::string> makeArgs(std::string_view cl);

/**
 * Returns the system-dependent path separator.
 *
 * @return the path
 */
consteval char
pathSeparator() {
#ifdef ROCKET_OS_WINDOWS
  return ';';
#else
  return ':';
#endif
}

namespace env {

// Environment ----------------------------------------------------------------------------------------------

// `EnvError` ...............................................................................................

/// An error related to environment variables.
struct EnvError : Error {
  /// Error codes.
  enum Code {
    ConversionFailed, ///< The conversion to the target type failed.
    NotFound ///< The environment variable was not found.
  };

  /// The error code.
  Code code;

  /**
   * @ctor
   *
   * @param code the error code
   */
  EnvError(Code code) : Error({}), code(code) {}

  /**
   * @ctor
   *
   * @param message the error message
   * @param code the error code
   */
  EnvError(std::string_view message, Code code) : Error(message), code(code) {}

  /// @member_op_eq
  bool operator==(const EnvError& rhs) const = default;
};

// `GetError`, `GetResult` ..................................................................................

/// Error for #rocket::system::env::get(std::string_view).
enum GetError {
  ConversionFailed, ///< The conversion to the target type failed.
  NotFound ///< The environment variable was not found.
};

/// Result with #rocket::system::env::GetError.
template<typename T>
using GetResult = std::expected<T, GetError>;

// Functions ................................................................................................

/**
 * Returns all environment variables as a set of name-value pairs.
 *
 * This function is thread-safe as long as all callers use this API from `system.h` exclusively.
 *
 * @return a set of name-value pairs
 */
std::unordered_map<std::string, std::string> get();

/**
 * Returns the value of an environment variable.
 *
 * This function is thread-safe as long as all callers use this API from `system.h` exclusively.
 *
 * @tparam T the type to convert a string value to
 * @param name the name of the environment variable
 * @return the value of the environment variable, converted to type @p T, a #rocket::system::env::EnvError
 *   otherwise
 */
template<typename T> requires (not std::same_as<T, std::string_view>)
GetResult<T>
get(std::string_view name) {
  auto val = internal::getImpl(name);
  if (not val) {
    return std::unexpected(NotFound);
  }
  auto result = rocket::str::toType<T>(*val).transform_error([&](const auto&) {
    return ConversionFailed;
  });
  if (not result) {
    return std::unexpected(result.error());
  }
  return result.value();
}

/**
 * Sets an environment variable.
 *
 * This function is thread-safe as long as all callers use this API from `system.h` exclusively.
 *
 * @attention In Windows, setting an environment variable to an empty string unsets the variable.
 *
 * @tparam T the type of the new value
 * @param name the name of the environment variable
 * @param val the new value
 * @param replace if `true`, then this function overwrites an existing value, otherwise it does not
 */
template<typename T>
inline void
set(std::string_view name, T&& val, bool replace = true) {
  internal::setImpl(name, fmt::format("{}", std::forward<T>(val)), replace);
}

/**
 * Unsets an environment variable.
 *
 * This function is thread-safe as long as all callers use this API from `system.h` exclusively.
 *
 * @param name the name of the environment variable
 */
inline void
unset(std::string_view name) {
  internal::setImpl(name, std::nullopt, true);
}

} // namespace env

} // namespace rocket::system

// EOF
