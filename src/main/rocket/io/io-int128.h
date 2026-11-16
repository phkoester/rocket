/**
 * @file io-int128.h
 *
 * Stream input and output operators for 128-bit integers.
 */

#pragma once

#include "rocket/rocket.h"

#include <iosfwd>

// Support for 128-bit data types ---------------------------------------------------------------------------

// These operators make #rocket::i128 and #rocket::u128 behave like the built-in integer types with
// `std::istream` and `std::ostream`. They follow the standard extractors and inserters for integers
// (`num_get` and `num_put`) as implemented by libstdc++, honoring the stream's format flags, width, fill
// character, exception mask, and locale. With the MSVC STL, extraction follows its `num_get` instead, which
// treats thousands separators differently.

/**
 * Input operator for #rocket::i128.
 *
 * This extracts an integer like the standard extractor for `long long`:
 *
 * - **Whitespace.** Leading whitespace is skipped, unless `std::noskipws` is set.
 * - **Sign.** An optional `+` or `-` may precede the digits.
 * - **Base.** `std::dec`, `std::oct`, or `std::hex` select the base. In base 16, an optional `0x` or `0X`
 *   prefix is skipped. If no base is set, e.g., after `std::setbase(0)`, the base is detected like with
 *   `scanf`'s `%i`: a leading `0x` or `0X` selects base 16, a leading `0` base 8, and anything else base 10.
 *   Hexadecimal digits are accepted in either case.
 * - **Digit grouping.** If the stream's locale groups digits, thousands separators may appear between the
 *   digits, e.g., `1,234,567`. If the groups do not match the locale, the value is assigned, but `failbit`
 *   is set. A separator at the start or right after another one ends the input, and there is no number then.
 *   The locale's decimal point always ends the number.
 *
 * Extraction stops at the first character that cannot continue the number, which is left in the stream. The
 * outcome is reported like with the standard extractors:
 *
 * | Input                                     | @p rhs                                    | State bits set
 * | ----------------------------------------- | ----------------------------------------- | --------------
 * | A valid number                            | The value                                 | None
 * | A number whose groups do not match        | The value                                 | `failbit`
 * | No number, e.g., `abc`                    | 0                                         | `failbit`
 * | A number out of range                     | The greatest or lowest value, by its sign | `failbit`
 * | Nothing, e.g., only whitespace before EOF | Unchanged                                 | `failbit`
 *
 * A number out of range is consumed entirely. In all cases, `eofbit` is set as well if the end of the input
 * was reached. If the stream buffer throws, `badbit` is set, and the exception is rethrown if `badbit` is in
 * the exception mask of @p lhs. Setting any other state bit throws `std::ios::failure` as specified by the
 * exception mask.
 *
 * @param lhs the input stream
 * @param rhs the value to assign to
 * @return @p lhs
 */
std::istream& operator>>(std::istream& lhs, rocket::i128& rhs);

/**
 * Output operator for #rocket::i128.
 *
 * This inserts an integer like the standard inserter for `long long`:
 *
 * - **Base.** `std::dec`, `std::oct`, or `std::hex` select the base. If no base is set, the value is
 *   written in decimal. Like with `printf`'s `%o` and `%x`, a negative value is written as its two's
 *   complement in octal and hexadecimal, e.g., -1 is written as 32 `f`s in hexadecimal.
 * - **Sign.** A negative value in decimal is preceded by `-`. With `std::showpos`, a nonnegative value in
 *   decimal is preceded by `+`.
 * - **Base prefix.** With `std::showbase`, a nonzero value is preceded by `0` in octal, and by `0x` in
 *   hexadecimal.
 * - **Case.** With `std::uppercase`, hexadecimal digits and the `0X` prefix are written in uppercase.
 * - **Digit grouping.** If the stream's locale groups digits, thousands separators are inserted between the
 *   digit groups, in any base, e.g., `1,234,567`.
 * - **Padding.** If the output is shorter than the stream's width, it is padded with the fill character, as
 *   specified by `adjustfield`: `std::right` (the default) pads on the left, `std::left` on the right, and
 *   `std::internal` between the sign or `0x` prefix and the digits. An octal `0` prefix counts as a digit
 *   here, so it is preceded by the padding. The width is reset to 0 afterward, as with any formatted output.
 *
 * @param lhs the output stream
 * @param rhs the value to print
 * @return @p lhs
 */
std::ostream& operator<<(std::ostream& lhs, rocket::i128 rhs);

/**
 * Input operator for #rocket::u128.
 *
 * This works like the input operator for #rocket::i128, with the following difference, matching the
 * standard extractor for `unsigned long long` and `strtoull`: a minus sign is accepted, and the negated
 * magnitude wraps around, e.g., `-1` yields the greatest value. A magnitude out of range yields the greatest
 * value and sets `failbit`, with or without a minus sign.
 *
 * @param lhs the input stream
 * @param rhs the value to assign to
 * @return @p lhs
 */
std::istream& operator>>(std::istream& lhs, rocket::u128& rhs);

/**
 * Output operator for #rocket::u128.
 *
 * This works like the output operator for #rocket::i128, except that no value is negative, and
 * `std::showpos` has no effect, matching the standard inserter for `unsigned long long`.
 *
 * @param lhs the output stream
 * @param rhs the value to print
 * @return @p lhs
 */
std::ostream& operator<<(std::ostream& lhs, rocket::u128 rhs);

// EOF
