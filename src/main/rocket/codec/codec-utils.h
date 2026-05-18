/**
 * @file codec-utils.h
 */

#pragma once

#include "rocket/nio/nio.h"

#include <scn/istream.h>

#include <set>

namespace rocket::codec {

// Utilities for encoding -----------------------------------------------------------------------------------

/**
 * Takes care of indentation.
 */
void beginContainer(nio::Sink& out, bool indent, u64& level, char c);

/**
 * Takes care of indentation.
 */
void endContainer(nio::Sink& out, bool indent, u64& level, u64 size, char c);

/**
 * Takes care of indentation
 */
void nextElem(nio::Sink& out, bool indent, u64 level, u64 index);

// Utilities for decoding -----------------------------------------------------------------------------------

/**
 * Throws if there is no colon, advances the source only on success.
 */
void expectColon(nio::Source& in);

/**
 * Throws if there is no comma, advances the source only on success
 */
void expectComma(nio::Source& in);

/**
 * Reads a single expected character, advances the source only on success.
 */
[[nodiscard]] bool read(nio::Source& in, char c);

/**
 * Reads any of a set of expected strings, advances the source only on success.
 *
 * There is an optimization for contiguous sources.
 */
[[nodiscard]] std::optional<std::string_view> read(
  nio::Source& in,
  const std::set<std::string_view>& values,
  bool ignoreCase = false);

/**
 * Reads until an expected character is found, advances the source only on success.
 *
 * The expected character is not included in the returned string, but is consumed from the source.
 *
 * There is an optimization for contiguous sources.
 */
[[nodiscard]] std::optional<std::string> readUntil(nio::Source& in, char c);

/**
 * Reads until an expected character not preceded by an escaping backslash is found, advances the source only
 * on success.
 *
 * The expected character is not included in the returned string, but is consumed from the source.
 *
 * There is an optimization for contiguous sources.
 */
[[nodiscard]] std::optional<std::string> readUntilUnescaped(nio::Source& in, char c);

/**
 * Scans from a source, using `scnlib`.
 *
 * There is an optimization for contiguous sources.
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
    return result->value();
  }
  return {};
}

/**
 * Scans a code point from a source, using `scnlib`.
 *
 * There is an optimization for contiguous sources.
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
    return result->value();
  }
  return {};
}

/**
 * Scans an integer value from a source, using `scnlib`.
 *
 * There is an optimization for contiguous sources.
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
  auto result = scn::scan<I>(is, "{}");
  if (result) {
    return result->value();
  }
  return {};
}

/**
 * Skips whitespace and comments, advances the source only if there is something to skip.
 */
void skip(nio::Source& in, bool cComments, bool shellComments);

/**
 * Skips until an expected string is found, advances the source in any case.
 *
 * If the expected string @p s is not found, the source is advanced to its end. Otherwise, the source is
 * advanced until after the first occurrence of @p s.
 *
 * There is an optimization for contiguous sources.
 */
bool skipUntil(nio::Source& in, std::string_view s);

} // namespace rocket::codec

// EOF
