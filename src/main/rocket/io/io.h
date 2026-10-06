/**
 * @file io.h
 *
 * I/O utilities.
 */

#pragma once

#include "rocket/rocket.h"

#include <cstdio>
#include <filesystem>
#include <spanstream>
#include <sstream>

#ifdef ROCKET_OS_WINDOWS

#include <io.h>

#define ROCKET_FILENO _fileno ///< Macro for portability.
#define ROCKET_ISATTY _isatty ///< Macro for portability.

#define STDIN_FILENO  0 ///< Standard input file number.
#define STDOUT_FILENO 1 ///< Standard output file number.
#define STDERR_FILENO 2 ///< Standard error file number.

#else

#include <unistd.h>

#define ROCKET_FILENO fileno ///< Macro for portability.
#define ROCKET_ISATTY isatty ///< Macro for portability.

#endif // ROCKET_OS_WINDOWS

namespace rocket::io {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes an empty input stream.
 *
 * @return an empty input stream
 */
inline std::istringstream is() { return {}; }

/**
 * Makes an input stream that reads from the string @p str.
 *
 * @param str the string to read from
 * @return an input stream that reads from the string @p str
 */
inline std::ispanstream is(std::string_view str) { return std::ispanstream(str); }

/**
 * Opens a file and returns a `FILE` pointer, or null if the file cannot be opened.
 *
 * This function is meant to be a replacement for #std::fopen.
 *
 * @param path the path to the file
 * @param modes the modes to open the file with
 * @return a `FILE` pointer, or null if the file cannot be opened
 */
FILE* open(const std::filesystem::path& path, std::string_view modes);

/**
 * Similar to #std::istream::tellg, but leaves @p is unchanged and returns the actual current position
 * rather than -1 if `is.fail()` returns `true`.
 *
 * @param is the input stream
 * @return the actual current input position, always nonnegative
 */
std::ios::pos_type tellg(std::istream& is) noexcept;

} // namespace rocket::io

// Support for 128-bit data types ---------------------------------------------------------------------------

/// @op_input{#i128}
std::istream& operator>>(std::istream& lhs, i128& rhs);

/// @op_output{#i128}
std::ostream& operator<<(std::ostream& lhs, i128 rhs);

/// @op_input{#u128}
std::istream& operator>>(std::istream& lhs, u128& rhs);

/// @op_output{#u128}
std::ostream& operator<<(std::ostream& lhs, u128 rhs);

// EOF
