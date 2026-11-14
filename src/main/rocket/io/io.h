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

#ifndef STDIN_FILENO
  #define STDIN_FILENO  0 ///< Standard input file number.
#endif
#ifndef STDOUT_FILENO
  #define STDOUT_FILENO 1 ///< Standard output file number.
#endif
#ifndef STDERR_FILENO
  #define STDERR_FILENO 2 ///< Standard error file number.
#endif

#else

#include <unistd.h>

#define ROCKET_FILENO fileno ///< Macro for portability.
#define ROCKET_ISATTY isatty ///< Macro for portability.

#endif // ROCKET_OS_WINDOWS

namespace rocket::io {

// Functions working with `FILE` pointers -------------------------------------------------------------------

/**
 * Opens a file and returns a `FILE` pointer, or null if the file cannot be opened.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows. On Windows, the path is passed
 * to the CRT as a wide string, so paths with characters outside the ANSI code page work, and the file is
 * opened without sharing restrictions, matching the POSIX behavior.
 *
 * @param path the path to the file
 * @param modes the modes to open the file with
 * @return a `FILE` pointer, or null if the file cannot be opened
 */
FILE* open(const std::filesystem::path& path, std::string_view modes);

/**
 * Seeks to the specified position in the stream referenced by @p file.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows. It always fails on streams that
 * are not #seekable, even on platforms where the C library would report a meaningless success.
 *
 * @param file a `FILE` pointer, may not be null
 * @param offset the offset to seek to
 * @param origin the origin to seek from
 * @return if the operation succeeded. On failure, `errno` is set
 */
bool seek(FILE* file, i64 offset, i32 origin);

/**
 * Checks if the stream referenced by @p file is seekable.
 *
 * A stream is seekable if it is backed by a regular file (or, on POSIX systems, a block device). Pipes,
 * FIFOs, sockets, and terminals are not seekable.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows. It inspects the type of the
 * underlying file rather than attempting to seek, because on some platforms seeking on a pipe or a terminal
 * may succeed with a meaningless result.
 *
 * On POSIX systems, streams without a file descriptor, such as those made by `fmemopen` or `open_memstream`,
 * are checked by querying the stream position instead.
 *
 * @param file a `FILE` pointer, may not be null
 * @return if the stream is seekable
 */
bool seekable(FILE* file);

/**
 * Returns the current position in the stream referenced by @p file.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows. It always returns
 * #rocket::NPOS for streams that are not #seekable, even on platforms where the C library would report a
 * meaningless position.
 *
 * @param file a `FILE` pointer, may not be null
 * @return the current position in the stream, or #rocket::NPOS if the position cannot be determined
 */
u64 tell(FILE* file);

// Functions working with streams ---------------------------------------------------------------------------

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
 * Checks if the input stream @p is is seekable.
 *
 * The stream is seekable if #tell reports a position for it. Like #tell, this leaves @p is entirely
 * unchanged.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows.
 *
 * @param is the input stream
 * @return if the stream is seekable
 */
bool seekable(std::istream& is) noexcept;

/**
 * Checks if the output stream @p os is seekable.
 *
 * The stream is seekable if its stream buffer reports a position for it. This leaves @p os entirely
 * unchanged.
 *
 * If @p os is #std::cout, #std::cerr, or #std::clog, `stdout` or `stderr` must also be #seekable, because on
 * Windows and macOS, the stream buffers of these streams may report a meaningless position for a pipe or a
 * terminal.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows.
 *
 * @param os the output stream
 * @return if the stream is seekable
 */
bool seekable(std::ostream& os) noexcept;

/**
 * Returns the current read position of the input stream @p is.
 *
 * Unlike #std::istream::tellg, this queries the stream buffer directly. It returns the actual position even
 * if a state bit is set, e.g., `eofbit` after an extraction reached the end of the input, where `tellg` would
 * return -1 and set `failbit`. It leaves @p is entirely unchanged: its state is not modified, its exception
 * mask is not consulted, and its tied stream is not flushed.
 *
 * If @p is is #std::cin, `stdin` must also be #seekable, because on Windows and macOS, the stream buffer of
 * #std::cin may report a meaningless position for a pipe or a terminal.
 *
 * This is a portable implementation that works on Linux, macOS, and Windows.
 *
 * @param is the input stream
 * @return the current position in the stream, or #rocket::NPOS if the position cannot be determined, e.g.,
 *   because @p is has no stream buffer or the stream buffer is not seekable
 */
u64 tell(std::istream& is) noexcept;

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
