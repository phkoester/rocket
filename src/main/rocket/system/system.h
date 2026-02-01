/**
 * @file system.h
 *
 * System-dependent functions, access to the environment.
 */

#pragma once

#include "rocket/format/format.h"
#include "rocket/str/StringConvert.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace rocket::system {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::optional<std::string> getImpl(std::string_view name);

void setImpl(std::string_view name, const std::optional<std::string>& value, bool replace);

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

/**
 * Executes @p cl and captures the output to standard out.
 *
 * @param cl the command line to execute
 * @return the captured output
 * @throws #rocket::InvalidState if the command cannot be executed
 */
std::vector<char> exec(const std::string& cl);

/**
 * Executes a command and captures the output to standard out.
 *
 * @param args the command line to execute
 * @return the captured output
 * @throws #rocket::InvalidState if the command cannot be executed
 */
std::vector<char> exec(const std::vector<std::string_view>& args);

/**
 * Returns the system-dependent executable suffix.
 *
 * @return the executable suffix
 */
consteval std::string_view
executableSuffix() {
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

/**
 * Returns all environment variables as a set of name-value pairs.
 *
 * This function is thread-safe as long as all callers use this API exclusively.
 *
 * @return a set of name-value pairs
 */
std::unordered_map<std::string, std::string>
get();

/**
 * Returns the value of an environment variable. If the string conversion fails, this function returns null.
 *
 * This function is thread-safe as long as all callers use this API exclusively.
 *
 * @tparam T the type to convert a string value to
 * @param name the name of the environment variable
 * @return null if the environment variable does not exist or if the string conversion fails, otherwise a
 *   value of type @p T
 */
template<typename T> requires (not std::is_same_v<T, std::string_view>)
std::optional<T>
get(std::string_view name) {
  auto v = internal::getImpl(name);
  if (not v) {
    return std::nullopt;
  }
  return str::tryToType<T>(*v);
}

/**
 * Sets an environment variable.
 *
 * This function is thread-safe as long as all callers use this API exclusively.
 *
 * @attention In Windows, setting an environment variable to an empty string unsets the variable.
 *
 * @tparam T the type of the new value
 * @param name the name of the environment variable
 * @param value the new value
 * @param replace if `true`, then this function overwrites an existing value, otherwise it does not
 */
template<typename T>
inline void
set(std::string_view name, T&& value, bool replace = true) {
  internal::setImpl(name, fmt::format("{}", std::forward<T>(value)), replace);
}

/**
 * Unsets an environment variable.
 *
 * This function is thread-safe as long as all callers use this API exclusively.
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
