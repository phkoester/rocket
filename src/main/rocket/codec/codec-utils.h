/**
 * @file codec-utils.h
 */

#pragma once

#include "rocket/io/io.h"
#include "rocket/nio/nio.h"

#include <scn/istream.h>

#include <chrono>
#include <functional>
#include <vector>

namespace rocket::codec {

// Utilities for encoding -----------------------------------------------------------------------------------

/**
 * Takes care of indentation.
 *
 * @param out the sink to write to
 * @param indent whether to indent the output
 * @param level the current indentation level
 * @param c the character to write
 */
void beginContainer(nio::Sink& out, bool indent, u64& level, char c);

/**
 * Takes care of indentation.
 *
 * @param out the sink to write to
 * @param indent whether to indent the output
 * @param level the current indentation level
 * @param size the size of the container
 * @param c the character to write
 */
void endContainer(nio::Sink& out, bool indent, u64& level, u64 size, char c);

/**
 * Takes care of indentation.
 *
 * @param out the sink to write to
 * @param indent whether to indent the output
 * @param level the current indentation level
 * @param index the index of the element
 */
void nextElem(nio::Sink& out, bool indent, u64 level, u64 index);

// Utilities for decoding -----------------------------------------------------------------------------------

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
 * Throws if there is no comma, advances the source only on success
 *
 * @param in the source to read from
 * @throw #rocket::InputFailure if there is no comma
 */
void expectComma(nio::Source& in);

/**
 * Reads a single expected character, advances the source only on success.
 *
 * @param in the source to read from
 * @param c the character to read
 * @return whether the character was read
 */
[[nodiscard]] bool readChar(nio::Source& in, char c);

/**
 * Reads any of a vector of expected strings, advances the source only on success.
 *
 * The elements in the vector must be nonempty and unique, their order matters: If two elements start with
 * the same prefix, the longer element must come first.
 *
 * There is an optimization for contiguous sources.
 *
 * @param in the source to read from
 * @param values the set of expected strings
 * @param ignoreCase whether to ignore case
 * @return the read string, or null if no string was read
 */
[[nodiscard]] std::optional<std::string_view> readChoice(
  nio::Source& in,
  const std::vector<std::string_view>& values,
  bool ignoreCase = false);

/**
 * If the next character in the source is `'.'`, reads a subsecond string and returns it as nanoseconds.
 *
 * If the next character is not `'.'`, returns zero nanoseconds and leaves the source unchanged.
 *
 * @param in the source to read from
 * @return the read subseconds as nanoseconds
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
 * Reads characters from a source while @p predeciate yields `true`, advances the source only as long as the
 * predicate holds.
 *
 * @param in the source to read from
 * @param predicate the predicate to call
 * @return the read string
 */
std::string readWhilePredicate(nio::Source& in, std::function<bool(char)> predicate);

/**
 * Scans from a source, using `scnlib`.
 *
 * There is an optimization for contiguous sources.
 *
 * @tparam T the type to scan
 * @param in the source to read from
 * @return the scanned value, or null if no value was scanned
 */
template<typename T>
[[nodiscard]] std::optional<T>
scan(nio::Source& in) {
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto str = contiguous->str();
    auto result = scn::scan<T>(str, "{}");
    if (result) {
      in.seek(result->begin() - str.begin(), nio::SeekMode::cur);
      return result->value();
    }
    return {};
  }
#endif

  // Noncontiguous source

  std::istream& is = in.istream();
  auto result = scn::scan<T>(is, "{}");
  if (result) {
    in.seek(io::tellg(is), nio::SeekMode::beg);
    return result->value();
  }
  return {};
}

/**
 * Scans a code point from a source, using `scnlib`.
 *
 * There is an optimization for contiguous sources.
 *
 * @tparam I the integer type to scan
 * @param in the source to read from
 * @return the scanned value, or null if no value was scanned
 */
template<typename I>
[[nodiscard]] std::optional<I>
scanCodePoint(nio::Source& in) {
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto str = contiguous->str();
    auto result = scn::scan<I>(str, "U+{:X}");
    if (result) {
      in.seek(result->begin() - str.begin(), nio::SeekMode::cur);
      return result->value();
    }
    return {};
  }
#endif

  std::istream& is = in.istream();
  auto result = scn::scan<I>(is, "U+{:X}");
  if (result) {
    in.seek(io::tellg(is), nio::SeekMode::beg);
    return result->value();
  }
  return {};
}

/**
 * Scans an integer value from a source, using `scnlib`.
 *
 * There is an optimization for contiguous sources.
 *
 * @tparam I the integer type to scan
 * @param in the source to read from
 * @return the scanned value, or null if no value was scanned
 */
template<typename I>
[[nodiscard]] std::optional<I>
scanInteger(nio::Source& in) {
#ifndef ROCKET_NIO_NO_CONTIGUOUS_SOURCE
  if (const auto* contiguous = dynamic_cast<nio::ContiguousSource*>(&in); contiguous != nullptr) {
    // Contiguous source

    const auto str = contiguous->str();
    // Setting `base` to 0 detects the base from the input
    auto result = scn::scan_int<I>(str, 0);
    if (result) {
      in.seek(result->begin() - str.begin(), nio::SeekMode::cur);
      return result->value();
    }
    return {};
  }
#endif

  // Noncontiguous source

  std::istream& is = in.istream();
  auto result = scn::scan<I>(is, "{:i}");
  if (result) {
    in.seek(io::tellg(is), nio::SeekMode::beg);
    return result->value();
  }
  return {};
}

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
 * If the expected string @p s is not found, the source is advanced to its end. Otherwise, the source is
 * advanced until after the first occurrence of @p s.
 *
 * There is an optimization for contiguous sources.
 *
 * @param in the source to read from
 * @param s the expected string
 * @return whether the expected string was found
 */
bool skipUntilString(nio::Source& in, std::string_view s);

} // namespace rocket::codec

// EOF
