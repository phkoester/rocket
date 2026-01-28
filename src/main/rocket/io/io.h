/**
 * @file io.h
 *
 * I/O stream utilities.
 */

#pragma once

#include "rocket/rocket.h"

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
