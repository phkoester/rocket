/**
 * @file scan.h
 *
 * Scanning support.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/io/io.h"

#include <scn/istream.h>

#ifdef ROCKET_HAS_BOOST_INT128

// `scn::scanner<rocket::i128>` -----------------------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::i128}
 *
 * This scanner reads the value via `operator>>`; format specifiers are not supported.
 */
template<typename C> requires rocket::IsChar<C>
struct scn::scanner<rocket::i128, C> : scn::basic_istream_scanner<C> {};

// `scn::scanner<rocket::u128>` -----------------------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::u128}
 *
 * This scanner reads the value via `operator>>`; format specifiers are not supported.
 */
template<typename C> requires rocket::IsChar<C>
struct scn::scanner<rocket::u128, C> : scn::basic_istream_scanner<C> {};

#endif

// EOF
