/**
 * @file util.h
 *
 * Additional `nio` utilities.
 */

#pragma once

#include "rocket/assert.h"
#include "rocket/nio/nio-fwd.h"
#include "rocket/unicode/unicode.h"

namespace rocket::nio {

/**
 * Reads the character @p expected from the source @p in.
 *
 * @param in the source
 * @param expected the expected character
 * @return the read character, which is @p expected
 * @throws #rocket::InputFailure if the source @p in is at EOF, if the source has an error, or if the read
 *     character is not @p expected
 */
char getChar(Source& in, char expected);

/**
 * Reads a character from the source @p in, expecting it to be in the string @p expected.
 *
 * @param in the source
 * @param expected the expected characters
 * @param what the description of the expected characters, e.g. `"a hexadecimal digit"`
 * @return the read character
 * @throws #rocket::InputFailure if the source @p in is at EOF, if the source has an error, or if the read
 *    character is not in the string @p expected
 */
char getChar(Source& in, const char* expected, const char* what);

/**
 * Reads a grapheme from the source @p in.
 *
 * @param in the source
 * @return the read grapheme
 * @throws #rocket::InputFailure if the grapheme could not be read or if the source has an error
 */
unicode::Grapheme getGrapheme(Source& in);

/**
 * Reads exactly @p n hexadecimal characters from the source @p in and returns the corresponding `uint32_t`
 * value.
 *
 * @param in the source
 * @param n the number of hexadecimal digits to read
 * @return the read hexadecimal number as a `uint32_t` value
 * @throws #rocket::InputFailure if the hexadecimal number could not be read or if the source has an error
 */
uint32_t getHex(Source& in, size_t n);

/**
 * Reads an optional character from the source @p in, returning null if the source is at EOF.
 *
 * @param in the source
 * @return the read character, or null if the source is at EOF
 * @throws #rocket::InputFailure if the source has an error
 */
std::optional<char> getOptionalChar(Source& in);

/**
 * Reads an optional grapheme from the source @p in, returning null if the source is at EOF.
 *
 * @param in the source
 * @return the read grapheme, or null if the source is at EOF
 * @throws #rocket::InputFailure if the source has an error
 */
 std::optional<unicode::Grapheme> getOptionalGrapheme(Source& in);

} // namespace rocket::nio
