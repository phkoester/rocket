/**
 * @file nio-utils.h
 */

#pragma once

#include "rocket/nio/nio.h"
#include "rocket/unicode/Character.h"

#include <chrono>
#include <optional>
#include <functional>
#include <set>
#include <sstream>

namespace rocket::nio {

// `NonseekableBuf` -----------------------------------------------------------------------------------------

/**
 * A stream buffer over a string that cannot seek, like one over a pipe.
 *
 * Useful for testing code with a #rocket::nio::StreamSource that cannot seek.
 */
struct NonseekableBuf : std::stringbuf {
  /**
   * @ctor
   *
   * @param str the string to read from
   */
  explicit NonseekableBuf(const std::string& str) : std::stringbuf(str, std::ios::in) {}

protected:

  pos_type
  seekoff(off_type, std::ios::seekdir, std::ios::openmode) override {
    return pos_type(off_type(-1));
  }

  pos_type seekpos(pos_type, std::ios::openmode) override { return pos_type(off_type(-1)); }
};

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
 * Tries to read a character, i.e., an extended grapheme cluster, from a source, advances the source only on
 * success.
 *
 * At the end of the input, the result is empty. If the first code point of the character is invalid UTF-8,
 * #rocket::InputFailure is thrown at the position where the character should have started, and the source
 * is not advanced. Invalid UTF-8 after the first code point ends the character, and stays in the source. An
 * I/O error throws #rocket::InputFailure at the position where the character should have started; the source
 * is bad then, and its position is unspecified.
 *
 * ASCII characters are checked by peeking. Any other code point must be read as a whole. If it does not
 * continue the character, the source must go back over it, which a source that cannot seek cannot do.
 *
 * @param in the source to read from
 * @return the character read, or an empty optional at the end of the input
 * @throw #rocket::InputFailure on invalid UTF-8 at the start of the character, or on an I/O error
 * @throw #rocket::InvalidState if the source must go back, but @p in cannot report its position
 */
[[nodiscard]] std::optional<unicode::Character<char>> readCharacter(nio::Source& in);

/**
 * Tries to read the longest of a set of expected strings, advances the source only on success.
 *
 * Bytes are only consumed while they continue an expected string, so a source that cannot seek works if the
 * first byte already does not match, or if the source stops right after the match. Otherwise, the source
 * must go back: on any other failure, if a longer expected string fails after a shorter one has matched, or
 * if checking the word boundary requires reading a non-ASCII code point.
 *
 * @param in the source to read from
 * @param values the set of expected strings, all elements must not be empty
 * @param ignoreCase whether to ignore case
 * @param endOfWord whether to check if the end of the returned string matches a word boundary
 * @return the read string, or null if no string was read
 * @throw #rocket::InvalidState if the source must go back, but @p in cannot report its position
 */
[[nodiscard]] std::optional<std::string_view> readChoice(
  nio::Source& in,
  const std::set<std::string_view>& values,
  bool ignoreCase = false,
  bool endOfWord = true);

/**
 * Tries to read a valid UTF-8 encoded code point from a source, advances the source only on success.
 *
 * At the end of the input, the result is empty. A truncated or otherwise malformed sequence, and an I/O
 * error, throw #rocket::InputFailure at the position where the code point should have started. On invalid
 * UTF-8, the source is not advanced. After an I/O error, the source is bad, and its position is unspecified.
 *
 * The first byte is checked by peeking, so a source that cannot seek works unless a sequence of several
 * bytes turns out to be malformed. Then, the source must go back over the bytes read.
 *
 * There is an optimization for contiguous sources.
 *
 * @param in the source to read from
 * @return the code point read, or an empty optional at the end of the input
 * @throw #rocket::InputFailure on invalid UTF-8 or an I/O error
 * @throw #rocket::InvalidState if the source must go back over invalid UTF-8, but @p in cannot report its
 *   position
 */
[[nodiscard]] std::optional<unicode::CodePoint> readCodePoint(nio::Source& in);

/**
 * Tries to read a C++ identifier from @p in, advances the source only on success.
 *
 * The first code point must have Unicode property `XID_Start`, or be `_` or `$`. Each following code point
 * must have Unicode property `XID_Continue`, or be `$`. Reading stops before the first code point that is
 * not part of the identifier, and that code point stays in the source.
 *
 * ASCII characters are checked by peeking, so a source that cannot seek works unless a non-ASCII code point
 * must be read and gone back over: at the start, where it cannot start an identifier, or after the
 * identifier.
 *
 * @param in the source to read from
 * @return the identifier, or null if the next code point cannot start an identifier
 * @throw #rocket::InvalidState if the source must go back over a non-ASCII code point, but @p in cannot
 *   report its position
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
