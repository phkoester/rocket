/**
 * @file codec-global.h
 *
 * Global codec.
 */

#pragma once

#include "base.h"

#ifdef ROCKET_CODEC_H
#error `codec.h` must be included after this file
#endif

// Functions ------------------------------------------------------------------------------------------------

/// @fn_parseRon{`bool`}
std::istream& parseRon(std::istream& is, bool& v);

/// @fn_parseRon{`char`}
std::istream& parseRon(std::istream& is, char& v);

/// @fn_parseRon{`unsigned char`}
std::istream& parseRon(std::istream& is, unsigned char& v);

/// @fn_parseRon{`char32_t`}
std::istream& parseRon(std::istream& is, char32_t& v);

/// @fn_parseRon{`int16_t`}
std::istream& parseRon(std::istream& is, int16_t& v);

/// @fn_parseRon{`uint16_t`}
std::istream& parseRon(std::istream& is, uint16_t& v);

/// @fn_parseRon{`int32_t`}
std::istream& parseRon(std::istream& is, int32_t& v);

/// @fn_parseRon{`uint32_t`}
std::istream& parseRon(std::istream& is, uint32_t& v);

/// @fn_parseRon{`int64_t`}
std::istream& parseRon(std::istream& is, int64_t& v);

/// @fn_parseRon{`uint64_t`}
std::istream& parseRon(std::istream& is, uint64_t& v);

/// @fn_parseRon{#int128_t}
std::istream& parseRon(std::istream& is, int128_t& v);

/// @fn_parseRon{#uint128_t}
std::istream& parseRon(std::istream& is, uint128_t& v);

/// @fn_parseRon_precision{`float`}
std::istream& parseRon(std::istream& is, float& v, int precision = rocket::DEFAULT_PRECISION);

/// @fn_parseRon_precision{`double`}
std::istream& parseRon(std::istream& is, double& v, int precision = rocket::DEFAULT_PRECISION);

/// @fn_parseRon_precision{`long double`}
std::istream& parseRon(std::istream& is, long double& v, int precision = rocket::DEFAULT_PRECISION);

// EOF
