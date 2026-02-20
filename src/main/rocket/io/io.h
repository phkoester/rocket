/**
 * @file io.h
 *
 * I/O utilities.
 */

#pragma once

#include "rocket/rocket.h"

#include <cstdio>
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
 * Similar to #std::istream::tellg, but leaves @p is unchanged and returns the actual current position
 * rather than -1 if `is.fail()` returns `true`.
 *
 * @param is the input stream
 * @return the actual current input position, always nonnegative
 */
std::ios::pos_type tellg(std::istream& is) noexcept;

} // namespace rocket::io

// EOF
