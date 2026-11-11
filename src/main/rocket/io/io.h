/**
 * @file io.h
 *
 * I/O utilities.
 */

#pragma once

#include "rocket/io/spanstream.h"

#include <cstdio>
#include <filesystem>
#include <sstream>
#include <string_view>

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
inline io::ispanstream is(std::string_view str) { return io::ispanstream(str); }

/**
 * Opens a file and returns a `FILE` pointer, or null if the file cannot be opened.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows.
 *
 * @param path the path to the file
 * @param modes the modes to open the file with
 * @return a `FILE` pointer, or null if the file cannot be opened
 */
FILE* open(const std::filesystem::path& path, std::string_view modes);

/**
 * Seeks to the specified position in the stream referenced by @p file.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows.
 *
 * @param file a `FILE` pointer
 * @param offset the offset to seek to
 * @param origin the origin to seek from
 * @return 0 on success, -1 on failure
 */
i32 seek(FILE* file, i64 offset, i32 origin);

/**
 * Returns the current position in the stream referenced by @p file.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows.
 *
 * @param file a `FILE` pointer
 * @return the current position in the stream, or #rocket::NPOS if the position cannot be determined
 */
u64 tell(FILE* file);

/**
 * Similar to #std::istream::tellg, but leaves @p is unchanged and returns the actual current position
 * rather than -1 if `is.fail()` returns `true`.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows.
 *
 * @param is the input stream
 * @return the current position in the stream, or #rocket::NPOS if the position cannot be determined
 */
u64 tellg(std::istream& is) noexcept;

} // namespace rocket::io

#ifndef ROCKET_HAS_INT128_OP_IO

// Support for 128-bit data types ---------------------------------------------------------------------------

/// @op_input{#rocket::i128}
std::istream& operator>>(std::istream& lhs, rocket::i128& rhs);

/// @op_output{#rocket::i128}
std::ostream& operator<<(std::ostream& lhs, rocket::i128 rhs);

/// @op_input{#rocket::u128}
std::istream& operator>>(std::istream& lhs, rocket::u128& rhs);

/// @op_output{#rocket::u128}
std::ostream& operator<<(std::ostream& lhs, rocket::u128 rhs);

#endif

// EOF
