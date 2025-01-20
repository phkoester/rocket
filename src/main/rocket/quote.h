/**
 * @file quote.h
 *
 * Quoted strings, based on Rocket's escape functionality.
 */

#pragma once

#include "S.h" // `rocket::raw()`

// Functions ------------------------------------------------------------------------------------------------

namespace rocket::quote {

/**
 * Encloses the string @p s in back ticks (<code>`</code>).
 *
 * @param s the string to quote
 * @return a quoted string
 */
std::string bt(std::string_view s);

/**
 * Encloses the string @p s in curly double quotes (`“` and `”`).
 *
 * @param s the string to quote
 * @return a quoted string
 */
std::string cd(std::string_view s);

/**
 * Encloses the string @p s in typographical single quotes (`‘` and `’`).
 *
 * @param s the string to quote
 * @return a quoted string
 */
std::string cs(std::string_view s);

/**
 * Encloses the string @p s in straight double quotes (`"`).
 *
 * @param s the string to quote
 * @return a quoted string
 */
std::string sd(std::string_view s);

/**
 * Encloses the string @p s in straight single quotes (<code>'</code>).
 *
 * @param s the string to quote
 * @return a quoted string
 */
std::string ss(std::string_view s);

} // namespace rocket::quote

// Macros ---------------------------------------------------------------------------------------------------

/**
 * Convenience macro to call #rocket::quote::bt.
 *
 * @param s the string to quote
 *
 * ## Examples
 *
 * ```
 * string s = S << "This is " << ROCKET_QUOTE_BT("quoted") << ".\n";
 * ```
 */
#define ROCKET_QUOTE_BT(s) ::rocket::raw(::rocket::quote::bt(s))

/**
 * Convenience macro to call #rocket::quote::cd.
 *
 * ## Examples
 *
 * ```
 * string s = S << "This is " << ROCKET_QUOTE_CD("quoted") << ".\n";
 * ```
 *
 * @param s the string to quote
 */
#define ROCKET_QUOTE_CD(s) ::rocket::raw(::rocket::quote::cd(s))

/**
 * Convenience macro to call #rocket::quote::cs.
 *
 * @param s the string to quote
 *
 * ## Examples
 *
 * ```
 * string s = S << "This is " << ROCKET_QUOTE_CS("quoted") << ".\n";
 * ```
 */
#define ROCKET_QUOTE_CS(s) ::rocket::raw(::rocket::quote::cs(s))

/**
 * Convenience macro to call #rocket::quote::sd.
 *
 * @param s the string to quote
 *
 * ## Examples
 *
 * ```
 * string s = S << "This is " << ROCKET_QUOTE_SD("quoted") << ".\n";
 * ```
 */
#define ROCKET_QUOTE_SD(s) ::rocket::raw(::rocket::quote::sd(s))

/**
 * Convenience macro to call #rocket::quote::ss.
 *
 * @param s the string to quote
 *
 * ## Examples
 *
 * ```
 * string s = S << "This is " << ROCKET_QUOTE_SS("quoted") << ".\n";
 * ```
 */
#define ROCKET_QUOTE_SS(s) ::rocket::raw(::rocket::quote::ss(s))

// EOF
