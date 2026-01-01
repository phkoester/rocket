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

inline std::istringstream is() { return std::istringstream(); }

inline std::ispanstream is(std::string_view s) { return std::ispanstream(s); }

/**
 * Similar to `std::istream::tellg`, but leaves @p is unchanged and returns the actual current
 * position rather than -1 if `is.fail()` returns `true`.
 *
 * @param is the input stream
 * @return the actual current position as a `size_t`
 */
std::ios::pos_type tellg(std::istream& is) noexcept;

} // namespace rocket::io

// EOF
