/**
 * @file str.h
 *
 * A string library, ready for `char` (UTF-8) and `char32` (UTF-32).
 */

#include "rocket/literal.h"
#include "rocket/type-traits.h"

#include <string>
#include <utility>
#include <vector>

#pragma once

namespace rocket::str {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a new string from @p str that has its first letter capitalized
 *
 * @param str a UTF-8 string
 * @return a new string
 */
std::string capitalize(std::string_view str);

/**
 * Makes a new string from @p str that has its first letter capitalized
 *
 * @param str a UTF-32 string
 * @return a new string
 */
std::u32string capitalize(std::u32string_view str);

/**
 * Joins elements of a range into a string, using a set of different separators.
 *
 * ## Examples
 *
 * To enumerate a list with an Oxford comma:
 *
 * ```
 * using namespace rocket;
 * using namespace std;
 *
 * set<string> s { "red", "green", "blue" };
 * string str = str::join(s.begin(), s.end(), ", ", " and ", ", and");
 * nio::out.println("The colors are {}.", str); // "The colors are red, green, and blue."
 * ```
 *
 * @param begin the beginning of the range
 * @param end the end of the range
 * @param sep the separator to use between elements
 * @param lastSepIfTwo the last separator to use if there are exactly two elements
 * @param lastSepIfMore the last separator to use if there are more than two elements
 * @return a new string
 */
template<typename Iterator>
std::string join(
  Iterator begin,
  Iterator end,
  std::string_view sep,
  std::string_view lastSepIfTwo,
  std::string_view lastSepIfMore) {
  std::string ret;

  u64 count = 0;
  const u64 n = std::distance(begin, end);
  for (Iterator it = begin; it != end; ++it) {
    if (++count > 1) {
      if (count == n) {
        ret.append(count == 2 ? lastSepIfTwo : lastSepIfMore);
      } else {
        ret.append(sep);
      }
    }
    ret.append(*it);
  }

  return ret;
}

/**
 * Splits a string into lines.
 *
 * No strings are ever allocated, except for the separator, so this is a very efficient way to split a string
 * into lines.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned vector.
 *
 * @param str the string to split
 * @return a vector of lines
 */
[[nodiscard]] std::vector<std::string_view> lines(std::string_view str);

/**
 * Converts a UTF-8 string to lower case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::string lower(std::string_view str);

/**
 * Converts a UTF-32 string to lower case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::u32string lower(std::u32string_view str);

/**
 * Converts a UTF-32 string to lower case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 */
void lowerIn(std::u32string& str);

/**
 * Splits a string into paragraphs.
 *
 * - Line breaks (`"\n"` or `"\r\n"`) are recognized.
 * - No-break spaces (U+00A0) are recognized.
 * - Tabs are replaced by spaces.
 * - Consecutive whitespace is collapsed.
 *
 * @param str the string to split into paragraphs
 * @return a vector of paragraphs. Each paragraph is a vector of words. Each word is a UTF-8 string
 */
[[nodiscard]] std::vector<std::vector<std::string>> paragraphs(std::string_view str);

/**
 * Makes a string view such that it has up to @p count leading occurrences of @p sub removed.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned string view.
 *
 * @tparam C the character type
 * @param str a string view
 * @param sub the substring to look for
 * @param count the maximum number of removals
 * @return a string view
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string_view<C>
removeLeading(
  std::basic_string_view<C> str,
  std::basic_string_view<C> sub,
  u64 count = NPOS) {
  if (sub.empty()) {
    return str;
  }

  std::basic_string_view<C> ret(str);
  for (u64 i = 0; i < count; ++i) {
    if (ret.starts_with(sub)) {
      ret.remove_prefix(sub.size());
    } else {
      return ret;
    }
  }
  return ret;
}

/**
 * Makes a string view such that it has up to @p count trailing occurrences of @p sub removed.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned string view.
 *
 * @tparam C the character type
 * @param str a string view
 * @param sub the substring to look for
 * @param count the maximum number of removals
 * @return a string view
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string_view<C>
removeTrailing(
  std::basic_string_view<C> str,
  std::basic_string_view<C> sub,
  u64 count = NPOS) {
  if (sub.empty()) {
    return str;
  }

  std::basic_string_view<C> ret(str);
  for (u64 i = 0; i < count; ++i) {
    if (ret.ends_with(sub)) {
      ret.remove_suffix(sub.size());
    } else {
      return ret;
    }
  }
  return ret;
}

/**
 * Makes a string view such that it has a trailing end-of-line, if any, removed.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned string view.
 *
 * @tparam C the character type
 * @param str a string view
 * @return a string view
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string_view<C>
removeTrailingEol(std::basic_string_view<C> str) {
  constexpr auto CRLF = LiteralString<C, '\r', '\n'>();
  if (str.ends_with(CRLF)) {
    str.remove_suffix(CRLF.size());
    return str;
  }
  constexpr auto LF = LiteralString<C, '\n'>();
  if (str.ends_with(LF)) {
    str.remove_suffix(LF.size());
    return str;
  }
  return str;
}

/**
 * Repeats the string @p str @p count times.
 *
 * @tparam C the character type
 * @param str a string
 * @param count the number of repetitions
 * @return a new string
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string<C>
repeat(const std::basic_string_view<C> str, u64 count) {
  std::basic_string<C> ret;
  ret.reserve(count * str.size());
  for (u64 i = 0; i < count; ++i) {
    ret.append(str);
  }
  return ret;
}

/**
 * Replaces up to @p count occurrences of @p from with @p to in the string @p str.
 *
 * @param str the string to replace in
 * @param from the substring to replace
 * @param to the substring to replace with
 * @param count the maximum number of replacements
 * @return the number of replacements
 */
u64 replaceIn(std::string& str, std::string_view from, std::string_view to, u64 count = NPOS);

/**
 * Replaces up to @p count occurrences of several substrings in the string @p str, in a single pass.
 *
 * The replacements happen simultaneously rather than one after the other, so replaced text is never searched
 * again: Applying `{ { "a", "b" }, { "b", "a" } }` to `"ab"` yields `"ba"`.
 *
 * The string is scanned from left to right. If several `from` substrings match at the same position, the one
 * that comes first in @p pairs wins, so list a longer `from` before any of its prefixes.
 *
 * The string views in @p pairs may refer to @p str itself.
 *
 * @param str the string to replace in
 * @param pairs the `from` and `to` pairs. No `from` substring may be empty
 * @param count the maximum number of replacements, in total across all pairs
 * @return the number of replacements
 */
u64 replaceIn(
  std::string& str,
  const std::vector<std::pair<std::string_view, std::string_view>>& pairs,
  u64 count = NPOS);

/**
 * Returns `"once"`, `"twice"`, or `"... times"` for @p n.
 *
 * @param n the number of times
 * @return `"once"`, `"twice"`, or `"... times"`
 */
[[nodiscard]] std::string times(u64 n);

/**
 * Removes leading whitespace from a string.
 *
 * @tparam C the character type
 * @param str the string to trim
 * @return a new string view
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string_view<C>
trimLeading(std::basic_string_view<C> str) {
  constexpr auto WS = LiteralString<C, '\t', '\n', '\v', '\f', '\r', ' '>();
  const auto pos = str.find_first_not_of(WS);
  if (pos == NPOS) {
    return std::basic_string_view<C>();
  }
  return str.substr(pos);
}

/**
 * Removes trailing whitespace from a string.
 *
 * @tparam C the character type
 * @param str the string to trim
 * @return a new string view
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string_view<C>
trimTrailing(std::basic_string_view<C> str) {
  constexpr auto WS = LiteralString<C, '\t', '\n', '\v', '\f', '\r', ' '>();
  const auto pos = str.find_last_not_of(WS);
  if (pos == NPOS) {
    return std::basic_string_view<C>();
  }
  return str.substr(0, pos + 1);
}

/**
 * Removes leading and trailing whitespace from a string.
 *
 * @tparam C the character type
 * @param str the string to trim
 * @return a new string view
 */
template<typename C> requires IsChar<C>
[[nodiscard]] std::basic_string_view<C>
trim(std::basic_string_view<C> str) {
  return trimLeading(trimTrailing(str));
}

/**
 * Converts a UTF-8 string to upper case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::string upper(std::string_view str);

/**
 * Converts a UTF-32 string to upper case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::u32string upper(std::u32string_view str);

/**
 * Converts a UTF-32 string to upper case, handling Unicode characters correctly.
 *
 * @param str the string to convert
 */
void upperIn(std::u32string& str);

/**
 * Wraps the string @p str to fit the width specified by @p width.
 *
 * - Line breaks (`"\n"` or `"\r\n"`) are recognized.
 * - No-break spaces (U+00A0) are recognized.
 * - Tabs are replaced by spaces.
 * - Consecutive whitespace is collapsed.
 *
 * @param str the string to wrap
 * @param leftIndent the left indentation
 * @param width the width to wrap to
 * @return a new string, containing the wrapped lines separated by @c '\\n'
 */
[[nodiscard]] std::string wrap(std::string_view str, u64 leftIndent = 0, u64 width = 80);

} // namespace rocket::str

// EOF
