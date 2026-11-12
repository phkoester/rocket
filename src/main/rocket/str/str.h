/**
 * @file str.h
 *
 * A string library, mostly for UTF-8 strings.
 */

#include "rocket/rocket.h"

#include <string>
#include <vector>

#pragma once

namespace rocket::str {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Makes a new string from @p str that has its first letter capitalized
 *
 * @param str a string
 * @return a new string
 */
std::string capitalize(std::string_view str);

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
 * Converts a UTF-8 string to lower case, handling Unicode code points correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::string lower(std::string_view str);

/**
 * Converts a UTF-32 string to lower case, handling Unicode code points correctly.
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
 * @return a vector of paragraphs. Each paragraph is a vector of words. Each word is a string
 */
[[nodiscard]] std::vector<std::vector<std::string>> paragraphs(std::string_view str);

/**
 * Makes a string view such that it has up to @p n leading occurrences of @p sub removed.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned string view.
 *
 * @param str a string view
 * @param sub the substring to look for
 * @param n the maximum number of removals
 * @return a string view
 */
std::string_view removeLeading(std::string_view str, std::string_view sub, u64 n = NPOS);

/**
 * Makes a string view such that it has up to @p n trailing occurrences of @p sub removed.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned string view.
 *
 * @param str a string view
 * @param sub the substring to look for
 * @param n the maximum number of removals
 * @return a string view
 */
std::string_view removeTrailing(std::string_view str, std::string_view sub, u64 n = NPOS);

/**
 * Makes a string view such that it has a trailing end-of-line, if any, removed.
 *
 * The underlying string of @p str must remain valid for the lifetime of the returned string view.
 *
 * @param str a string view
 * @return a string view
 */
std::string_view removeTrailingEol(std::string_view str);

/**
 * Repeats the string @p str @p n times.
 *
 * @param str a string
 * @param n the number of repetitions
 * @return a new string
 */
std::string repeat(std::string_view str, u64 n);

/**
 * Replaces up to @p n occurrences of @p from with @p to in the string @p str.
 *
 * @param str the string to replace in
 * @param from the substring to replace
 * @param to the substring to replace with
 * @param n the maximum number of replacements
 * @return the number of replacements
 */
u64 replaceIn(std::string& str, std::string_view from, std::string_view to, u64 n = NPOS);

/**
 * Replaces up to @p n occurrences of several substrings in the string @p str, in a single pass.
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
 * @param n the maximum number of replacements, in total across all pairs
 * @return the number of replacements
 */
u64 replaceIn(
  std::string& str,
  const std::vector<std::pair<std::string_view, std::string_view>>& pairs,
  u64 n = NPOS);

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
 * @param str the string to trim
 * @return a new string view
 */
std::string_view trimLeading(std::string_view str);

/**
 * Removes trailing whitespace from a string.
 *
 * @param str the string to trim
 * @return a new string view
 */
std::string_view trimTrailing(std::string_view str);

/**
 * Removes leading and trailing whitespace from a string.
 *
 * @param str the string to trim
 * @return a new string view
 */
inline std::string_view
trim(std::string_view str) {
  return trimLeading(trimTrailing(str));
}

/**
 * Converts a UTF-8 string to upper case, handling Unicode code points correctly.
 *
 * @param str the string to convert
 * @return a new string
 */
[[nodiscard]] std::string upper(std::string_view str);

/**
 * Converts a UTF-32 string to upper case, handling Unicode code points correctly.
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
