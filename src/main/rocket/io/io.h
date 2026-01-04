/**
 * @file io.h
 *
 * I/O utilities.
 */

#pragma once

#include <spanstream>
#include <sstream>

namespace rocket::io {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes an empty input stream.
 *
 * @return an empty input stream
 */
inline std::istringstream is() { return std::istringstream(); }

/**
 * Makes an input stream that reads from the string @p s.
 *
 * @param s the string to read from
 * @return an input stream that reads from the string @p s
 */
inline std::ispanstream is(std::string_view s) { return std::ispanstream(s); }

/**
 * Similar to `std::istream::tellg`, but leaves @p is unchanged and returns the actual current position
 * rather than -1 if `is.fail()` returns `true`.
 *
 * @param is the input stream
 * @return the actual current input position, always nonnegative
 */
std::ios::pos_type tellg(std::istream& is) noexcept;

} // namespace rocket::io

// EOF
