/**
 * @file codec-global.h
 *
 * Global codec.
 */

#pragma once

#include "basic.h"

#ifdef ROCKET_CODEC_H
#error 'codec.h' must be included after this file
#endif

// Functions ------------------------------------------------------------------------------------------------

/// @fn_parseRon{@c bool}
std::istream& parseRon(std::istream& is, bool& v);

/// @fn_printRon{@c bool}
std::ostream& printRon(std::ostream& os, bool v);

/// @fn_parseRon{@c char}
std::istream& parseRon(std::istream& is, char& v);

/// @fn_printRon{@c char}
std::ostream& printRon(std::ostream& os, char v);

/// @fn_parseRon{<tt>unsigned char</tt>}
std::istream& parseRon(std::istream& is, unsigned char& v);

/// @fn_printRon{<tt>unsigned char</tt>}
std::ostream& printRon(std::ostream& os, unsigned char v);

/// @fn_parseRon{@c char32_t}
std::istream& parseRon(std::istream& is, char32_t& v);

/// @fn_printRon{@c char32_t}
std::ostream& printRon(std::ostream& os, char32_t v);

/// @fn_parseRon{@c int16_t}
std::istream& parseRon(std::istream& is, int16_t& v);

/// @fn_printRon{@c int16_t}
std::ostream& printRon(std::ostream& os, int16_t v);

/// @fn_parseRon{@c uint16_t}
std::istream& parseRon(std::istream& is, uint16_t& v);

/// @fn_printRon{@c uint16_t}
std::ostream& printRon(std::ostream& os, uint16_t v);

/// @fn_parseRon{@c int32_t}
std::istream& parseRon(std::istream& is, int32_t& v);

/// @fn_printRon{@c int32_t}
std::ostream& printRon(std::ostream& os, int32_t v);

/// @fn_parseRon{@c uint32_t}
std::istream& parseRon(std::istream& is, uint32_t& v);

/// @fn_printRon{@c uint32_t}
std::ostream& printRon(std::ostream& os, uint32_t v);

/// @fn_parseRon{@c int64_t}
std::istream& parseRon(std::istream& is, int64_t& v);

/// @fn_printRon{@c int64_t}
std::ostream& printRon(std::ostream& os, int64_t v);

/// @fn_parseRon{@c uint64_t}
std::istream& parseRon(std::istream& is, uint64_t& v);

/// @fn_printRon{@c uint64_t}
std::ostream& printRon(std::ostream& os, uint64_t v);

/// @fn_parseRon{#int128_t}
std::istream& parseRon(std::istream& is, int128_t& v);

/// @fn_printRon{#int128_t}
std::ostream& printRon(std::ostream& os, int128_t v);

/// @fn_parseRon{#uint128_t}
std::istream& parseRon(std::istream& is, uint128_t& v);

/// @fn_printRon{#uint128_t}
std::ostream& printRon(std::ostream& os, uint128_t v);

/// @fn_parseRon_precision{@c float}
std::istream& parseRon(std::istream& is, float& v, int precision = rocket::DEFAULT_PRECISION);

/// @fn_printRon_precision{@c float}
std::ostream& printRon(std::ostream& os, float v, int precision = rocket::DEFAULT_PRECISION);

/// @fn_parseRon_precision{@c double}
std::istream& parseRon(std::istream& is, double& v, int precision = rocket::DEFAULT_PRECISION);

/// @fn_printRon_precision{@c double}
std::ostream& printRon(std::ostream& os, double v, int precision = rocket::DEFAULT_PRECISION);

/// @fn_parseRon_precision{<tt>long double</tt>}
std::istream& parseRon(std::istream& is, long double& v, int precision = rocket::DEFAULT_PRECISION);

/// @fn_printRon_precision{<tt>long double</tt>}
std::ostream& printRon(std::ostream& os, long double v, int precision = rocket::DEFAULT_PRECISION);

// EOF
