/**
 * @file nio-utils.h
 */

#pragma once

#include "rocket/nio/nio.h"

#include <chrono>
#include <expected>
#include <functional>
#include <set>

namespace rocket::nio {

// `ReadCodePointError`, `ReadCodePointResult` --------------------------------------------------------------

/// The error type for #rocket::nio::readCodePoint.
enum ReadCodePointError : u8 {
  Bad, ///< Bad I/O device
  Eof, ///< End of file
  InvalidUtf8, ///< Invalid UTF-8 byte sequence
};

/// The result type with #rocket::nio::ReadCodePointError.
using ReadCodePointResult = std::expected<unicode::CodePoint, ReadCodePointError>;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Throws if the next character in the source is not the expected character @p c, advances the source only
 * on success.
 *
 * @param in the source to read from
 * @param c the character to read
 * @throw #rocket::InputFailure if the next character in the source is not @p c
 */
void expectChar(nio::Source& in, char c);

/**
 * Throws if there is no colon, advances the source only on success.
 *
 * @param in the source to read from
 * @throw #rocket::InputFailure if there is no colon
 */
void expectColon(nio::Source& in);

/**
 * Throws if there is no comma, advances the source only on success.
 *
 * @param in the source to read from
 * @throw #rocket::InputFailure if there is no comma
 */
void expectComma(nio::Source& in);

/**
 * Reads a single expected character, advances the source only on success.
 *
 * This also works with sources that cannot seek, such as pipes.
 *
 * @param in the source to read from
 * @param c the character to read
 * @return if the character was read
 */
[[nodiscard]] bool readChar(nio::Source& in, char c);

/**
 * Reads any of a set of expected strings, advances the source only on success.
 *
 * There is an optimization for contiguous sources.
 *
 * @param in the source to read from
 * @param values the set of expected strings, all elements must not be empty
 * @param ignoreCase whether to ignore case
 * @param endOfWord whether to check if the end of the returned string matches a word boundary
 * @return the read string, or null if no string was read
 */
[[nodiscard]] std::optional<std::string_view> readChoice(
  nio::Source& in,
  const std::set<std::string_view>& values,
  bool ignoreCase = false,
  bool endOfWord = true);

/**
 * Tries to read a valid code point from a source, advances the source only on success.
 *
 * The result is #rocket::nio::Eof only if no bytes are left. A truncated or otherwise malformed sequence
 * yields #rocket::nio::InvalidUtf8. An I/O error yields #rocket::nio::Bad.
 *
 * A source that cannot report its position, as one over a pipe, cannot go back either. It still advances
 * over the bytes read on failure.
 *
 * There is an optimization for contiguous sources.
 *
 * @param in the source to read from
 * @return a code point if one was read, an error otherwise
 */
[[nodiscard]] ReadCodePointResult readCodePoint(nio::Source& in);

/**
 * Reads a C++ identifier from @p in, advances the source only on success.
 *
 * The first code point must have Unicode property `XID_Start`, or be `_` or `$`. Each following code point
 * must have Unicode property `XID_Continue`, or be `$`. Reading stops before the first code point that is
 * not part of the identifier, and that code point stays in the source. On failure, the source is left
 * unchanged.
 *
 * @param in the source to read from
 * @return the identifier, or null if the next code point cannot start an identifier
 */
[[nodiscard]] std::optional<std::string> readIdentifier(nio::Source& in);

/**
 * Reads an expected string, advances the source only on success.
 *
 * There is an optimization for contiguous sources.
 *
 * @param in the source to read from
 * @param str the string to read, must not be empty
 * @param ignoreCase whether to ignore case
 * @param endOfWord whether to check if the end of the returned string matches a word boundary
 * @return if the string was read
 */
[[nodiscard]] bool readString(
  nio::Source& in,
  std::string_view str,
  bool ignoreCase = false,
  bool endOfWord = true);

/**
 * If the next character in the source is `'.'`, reads a subsecond string and returns it as nanoseconds.
 *
 * If the next character is not `'.'`, returns zero nanoseconds and leaves the source unchanged.
 *
 * @param in the source to read from
 * @return the read subseconds as nanoseconds
 * @throw #rocket::InputFailure if the input is invalid
 */
[[nodiscard]] std::chrono::nanoseconds readSubseconds(nio::Source& in);

/**
 * Reads until an expected character is found, advances the source only on success.
 *
 * The expected character is not included in the returned string, but is consumed from the source.
 *
 * There is an optimization for contiguous sources.
 *
 * @param in the source to read from
 * @param c the character to read until
 * @return the read string, not including @p c, or null if no string was read
 */
[[nodiscard]] std::optional<std::string> readUntilChar(nio::Source& in, char c);

/**
 * Reads until an expected character not preceded by an escaping backslash is found, advances the source only
 * on success.
 *
 * The expected character is not included in the returned string, but is consumed from the source.
 *
 * There is an optimization for contiguous sources.
 *
 * @param in the source to read from
 * @param c the character to read until
 * @return the read string, not including @p c, or null if no string was read
 */
[[nodiscard]] std::optional<std::string> readUntilUnescapedChar(nio::Source& in, char c);

/**
 * Reads characters from a source while @p predicate yields `true`, advances the source only as long as the
 * predicate holds.
 *
 * This also works with sources that cannot seek, such as pipes. There is an optimization for contiguous
 * sources.
 *
 * @param in the source to read from
 * @param predicate the predicate to call
 * @return the read string
 */
std::string readWhilePredicate(nio::Source& in, const std::function<bool(char)>& predicate);

/**
 * Skips whitespace and comments, advances the source only if there is something to skip.
 *
 * @param in the source to read from
 * @param cComments whether to skip C-style comments
 * @param shellComments whether to skip Shell-style comments
 */
void skip(nio::Source& in, bool cComments, bool shellComments);

/**
 * Skips until an expected string is found, advances the source in any case.
 *
 * If the expected string @p str is not found, the source is advanced to its end. Otherwise, the source is
 * advanced until after the first occurrence of @p str.
 *
 * There is an optimization for contiguous sources.
 *
 * @param in the source to read from
 * @param str the expected string
 * @return if the expected string was found
 */
bool skipUntilString(nio::Source& in, std::string_view str);

} // namespace rocket::nio

// EOF
