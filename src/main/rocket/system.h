/**
 * @file system.h
 *
 * System-dependent functions, access to the environment.
 */

#pragma once

#include "StringConvert.h"

#include <cstdlib>
#include <optional>
#include <string_view>
#include <vector>

namespace rocket::system {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Executes @p cl and captures the output to standard out.
 *
 * @param cl the command line to execute
 * @return the captured output
 */
std::vector<std::byte> exec(const std::string& cl);

/**
 * Executes a command and captures the output to standard out.
 *
 * @param args the command line to execute
 * @return the captured output
 */
std::vector<std::byte> exec(const std::vector<std::string_view>& args);

/**
 * Returns the system-dependent executable suffix.
 *
 * @return the executable suffix
 */
std::string_view executableSuffix();

/**
 * Returns the system-dependent file separator.
 *
 * @return the file separator
 */
char fileSeparator();

/**
 * Returns the system-dependent path separator.
 *
 * @return the path
 */
char pathSeparator();

namespace env {

// Environment ----------------------------------------------------------------------------------------------

/**
 * Returns the value of an environment variable. If the string conversion fails, this function returns null.
 *
 * @tparam T the type to convert a string value to
 * @param name the name of the environment variable
 * @return null if the environment variable does not exist or if the string conversion fails, otherwise a
 *     value of type @p T
 */
template<typename T>
std::optional<T>
get(const std::string& name) {
  const char* p = getenv(name.c_str());
  if (not p)
    return std::nullopt;
  std::string_view s(p);
  return tryToType<T>(s);
}

/**
 * Sets an environment variable.
 *
 * @tparam T the type of the new value
 * @param name the name of the environment variable
 * @param value the new value
 * @param replace if `true`, then this function overwrites an existing value, otherwise it does not
 */
template<typename T>
inline void
set(const std::string& name, T&& value, bool replace = true) {
  setenv(name.c_str(), fmt::format("{}", std::forward<T>(value)).c_str(), replace ? 1 : 0);
}

/**
 * Unsets an environment variable.
 *
 * @param name the name of the environment variable
 */
void unset(const std::string& name);

} // namespace env

} // namespace rocket::system

// EOF
