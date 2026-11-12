/**
 * @file system.h
 *
 * System-dependent functions, access to the environment.
 */

#pragma once

#include "rocket/format.h"
#include "rocket/str/StringConvert.h"

#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace rocket::system {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::optional<std::string> getImpl(std::string_view name);

void setImpl(std::string_view name, const std::optional<std::string>& val, bool replace);

} // namespace internal

// `ExecResult` ---------------------------------------------------------------------------------------------

/**
 * The result of #rocket::system::exec.
 *
 * A command that ran has either #status or #signal set. A command that could not be started has neither,
 * and #error tells why.
 */
struct ExecResult {
  /// The output to stdout. If the command ran with `combined`, the output to stderr as well.
  std::string out;

  /// The output to stderr. Always empty if the command ran with `combined`.
  std::string err;

  /**
   * The exit status if the command exited normally, otherwise empty.
   *
   * On Windows, this is the exit code of the process, converted to #rocket::i32.
   */
  std::optional<i32> status;

  /// Linux, macOS: The number of the signal that terminated the command, otherwise empty. Always empty on
  /// Windows.
  std::optional<i32> signal;

  /**
   * The system error if the command could not be started or its output could not be read completely,
   * otherwise empty.
   *
   * A command that cannot be found is reported here by the overload taking arguments. The overload taking a
   * command line hands it to a shell, which reports it with an exit status instead: 127 on Linux and macOS,
   * 9009 on Windows.
   */
  std::error_code error;

  /**
   * Returns whether the command ran, exited with status 0, and its output was read completely.
   *
   * @return `true` on success, otherwise `false`
   */
  [[nodiscard]] bool
  ok() const {
    return not error && status == 0;
  }
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Executes the command line @p cl with the shell and captures the output to stdout and stderr.
 *
 * On Linux and macOS, the shell is `/bin/sh -c`. On Windows, it is `cmd.exe /c`.
 *
 * The command inherits stdin. A nonzero exit status, a signal, or a failure to start the command is not an
 * error of this function: it is reported in the result.
 *
 * @param cl the command line to execute. It may not contain NUL characters
 * @param combined if `true`, the output to stderr goes to #rocket::system::ExecResult::out as well, in the
 *   order the command writes it. Note that a command commonly buffers its output to stdout when it writes
 *   to a pipe, but not its output to stderr
 * @return the result
 * @throw #rocket::InvalidArgument if @p cl contains a NUL character
 */
ExecResult exec(const std::string& cl, bool combined = false);

/**
 * Executes a command and captures the output to stdout and stderr.
 *
 * Each argument reaches the command literally, as a single argument. No shell interprets it, so it is safe
 * to pass untrusted values. The program is looked up in the `PATH` if it does not contain a path separator.
 *
 * The command inherits stdin. A nonzero exit status, a signal, or a failure to start the command is not an
 * error of this function: it is reported in the result.
 *
 * @attention On Windows, the program may not be a batch file (`.bat` or `.cmd`). Windows hands batch files
 * to `cmd.exe`, which would interpret the arguments. Use #exec(const std::string&, bool) to run a batch file
 * with arguments that are known to be harmless.
 *
 * @param args the command line to execute. The first element is the program. It may not be empty, nor
 *   contain quotation marks on Windows. No element may contain NUL characters
 * @param combined if `true`, the output to stderr goes to #rocket::system::ExecResult::out as well, in the
 *   order the command writes it. Note that a command commonly buffers its output to stdout when it writes
 *   to a pipe, but not its output to stderr
 * @return the result
 * @throw #rocket::InvalidArgument if @p args is empty, or an element of @p args contains a character that
 *   cannot be passed, or the program is a batch file on Windows
 */
ExecResult exec(const std::vector<std::string_view>& args, bool combined = false);

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
 * @throw #rocket::InputFailure for unterminated quotes and trailing backslashes
 */
std::vector<std::string> makeArgs(std::string_view cl);

/**
 * Returns the system-dependent path separator.
 *
 * @return the path separator
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

// `GetError`, `GetResult` ..................................................................................

/// The error type for #rocket::system::env::get(std::string_view).
enum GetError : u8 {
  ConversionFailed, ///< The conversion to the target type failed.
  NotFound ///< The environment variable was not found.
};

/// The result type with #rocket::system::env::GetError.
template<typename T>
using GetResult = std::expected<T, GetError>;

// Functions ................................................................................................

/**
 * Returns all environment variables as a map of name-value pairs.
 *
 * This function is thread-safe as long as all callers use this API from `system.h` exclusively.
 *
 * @return a map of name-value pairs
 */
[[nodiscard]] std::unordered_map<std::string, std::string> get();

/**
 * Returns the value of an environment variable.
 *
 * This function is thread-safe as long as all callers use this API from `system.h` exclusively.
 *
 * @tparam T the type to convert a string value to
 * @param name the name of the environment variable
 * @return the value of the environment variable, converted to type @p T, otherwise an error
 */
template<typename T> requires (not std::same_as<T, std::string_view>)
[[nodiscard]] GetResult<T>
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
 * @attention On Windows, setting an environment variable to an empty string unsets the variable.
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
