/**
 * @file scan.h
 *
 * Scanning support.
 */

#pragma once

#include "rocket/type-traits.h"
#include "rocket/io/io.h"
#include "rocket/type-traits.h"

#include <scn/istream.h>

#ifdef ROCKET_HAS_BOOST_INT128

// `scn::scanner<i128>` -------------------------------------------------------------------------------------

/**
 * @spec_scn_scanner{#i128}
 *
 * This scanner reads the value via `operator>>`; format specifiers are not supported.
 */
template<typename C> requires rocket::IsChar<C>
struct scn::scanner<i128, C> : scn::basic_istream_scanner<C> {};

// `scn::scanner<u128>` -------------------------------------------------------------------------------------

/**
 * @spec_scn_scanner{#u128}
 *
 * This scanner reads the value via `operator>>`; format specifiers are not supported.
 */
template<typename C> requires rocket::IsChar<C>
struct scn::scanner<u128, C> : scn::basic_istream_scanner<C> {};

#endif

// EOF
