/**
 * @file escape.h
 *
 * Escaped strings, offering an interface similar to `std::quoted`.
 */

#pragma once

#include "assert.h"
#include "container.h"
#include "format.h"
#include "io.h"
#include "unicode-iterator.h"

#include <optional>
#include <string>

namespace rocket::escape {

// `CString` ------------------------------------------------------------------------------------------------

/**
 * C-string escaping.
 */
struct CString {
  /**
   * Parameters for C-string escaping.
   */
  struct Params {
    /**
     * Is `true` if the escaped string is to be enclosed in #quote characters.
     */
    bool enclosed = false;
    /**
     * The quote character to escape.
     *
     * This must be <code>'\0'</code>, <code>'"'</code>, or <code>'\''</code>, otherwise it is invalid.
     */
    char quote = '\0';
    /**
     * Configures the handling of tab characters.
     *
     * If this is null, then tab characters are escaped as `"\\t"`. Otherwise, a tab expands to at most
     * #tabSize spaces.
     */
    std::optional<size_t> tabSize;

    /**
     * Returns `true` if the escaped string is actually to be enclosed.
     *
     * @return `true` if the escaped string is actually to be enclosed
     */
    inline bool enclosing() const { return enclosed && quote != '\0'; }
  };
};

// `Regex` --------------------------------------------------------------------------------------------------

/**
 * Regular-expression escaping.
 */
struct Regex {
  /**
   * Parameters for regular-expression escaping.
   */
  struct Params {};
};

// `Result` -------------------------------------------------------------------------------------------------

/**
 * The result of an escape/unescape operation.
 */
struct Result {
  /**
   * The input of the escape/unescape operation.
   */
  std::string input;
  /**
   * Translated positions after escaping/unescaping.
   *
   * For each grapheme in the input string and for end-of-string, its character offset—i.e. either its `char`
   * or `char32_t` offset—, is mapped to a character offset in the output string.
   */
  container::UnorderedBimap<size_t, size_t> positions;
};

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

template<typename Schema>
struct EscapedString {
  std::string& s;
  Schema::Params params;
  Result* result;

  EscapedString(
      std::string& s,
      const Schema::Params& params,
      Result* result) : s(s), params(params), result(result) {}
};

// `EscapedString<CString>` .................................................................................

// XXX std::string escapeCStringHex(unicode::CodePoint, size_t&);

// XXX std::string escapeCStringTab(size_t&, const CString::Params&);

std::string escapeCString(unicode::CodePoint cp, size_t& column, const CString::Params& params);

std::string escapeCStringHex(unicode::CodePoint cp, size_t& column);

std::string escapeCStringTab(size_t& column, const CString::Params& params);

std::istream& operator>>(std::istream& lhs, const EscapedString<CString>& rhs);

std::ostream& operator<<(std::ostream& lhs, const EscapedString<CString>& rhs);

// `EscapedString<Regex>` ...................................................................................

std::string escapeRegex(unicode::CodePoint cp, size_t& column);

std::istream& operator>>(std::istream& lhs, const EscapedString<Regex>& rhs);

std::ostream& operator<<(std::ostream& lhs, const EscapedString<Regex>& rhs);

} // namespace internal

// `Escaped` ------------------------------------------------------------------------------------------------

/**
 * A concept for valid escaping schemata.
 *
 * Currently, these are
 *
 * - #rocket::escape::CString
 * - #rocket::escape::Regex
 *
 * @tparam Schema the escaping schema
 */
template<typename Schema>
concept Escaped = std::is_same_v<Schema, CString> || std::is_same_v<Schema, Regex>;

// Functions ------------------------------------------------------------------------------------------------

/**
 * `escaped` overload with a nonconst string reference.
 *
 * Use this function to unescape a string with `operator>>`.
 *
 * @tparam Schema the escaping Schema
 * @param s a nonconst string reference
 * @param params parameters for unescaping
 * @param result pointer to a #Result instance. If nonnull, then the members of this #Result are initialized
 * @return an internal representation of an escaped string
 *
 * ## Examples
 *
 * ```
 * using namespace rocket;
 * using namespace std;
 *
 * stringstream ss;
 * string in = "a\"b";
 * escaped::CString::Params params { .enclosed=true, .quote='"' };
 * ss << escaped::escaped<escaped::CString>(in, params);
 * cout << ss.str() << '\n'; // Quotation mark, 'a', backslash, quotation mark, 'b'
 * string out;
 * ss >> escaped::escaped<escaped::CString>(out, params);
 * assert(out == in); // After unescaping, `out` equals `in`
 * ```
 */
template<typename Schema> requires Escaped<Schema>
internal::EscapedString<Schema>
escaped(
    std::string& s,
    const typename Schema::Params& params = {},
    Result* result = nullptr) {
  return internal::EscapedString<Schema>(s, params, result);
}

/**
 * `escaped` overload with a const string reference.
 *
 * Use this function to escape a string with `operator<<`.
 *
 * @tparam Schema the escaping Schema
 * @param s a const string reference
 * @param params parameters for escaping
 * @param result pointer to a #Result instance. If nonnull, then the members of this #Result are initialized
 * @return an internal representation of an escaped string
 *
 * ## Examples
 *
 * ```
 * using namespace rocket;
 * using namespace std;
 *
 * stringstream ss;
 * string in = "a\"b";
 * escaped::CString::Params params { .enclosed=true, .quote='"' };
 * ss << escaped::escaped<escaped::CString>(in, params);
 * cout << ss.str() << '\n'; // ", a, \, ", b, "
 * string out;
 * ss >> escaped::escaped<escaped::CString>(out, params);
 * assert(out == in); // After unescaping, `out` equals `in`
 * ```
 */
template<typename Schema> requires Escaped<Schema>
internal::EscapedString<Schema>
escaped(
    const std::string& s,
    const typename Schema::Params& params = {},
    Result* result = nullptr) {
  return internal::EscapedString<Schema>(const_cast<std::string&>(s), params, result);
}

} // namespace rocket::escape

// EOF
