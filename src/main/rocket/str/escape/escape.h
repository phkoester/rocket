/**
 * @file escape.h
 *
 * Escaped strings, offering an interface similar to #std::quoted.
 */

#pragma once

#include "rocket/UnorderedBimap.h"

#include <optional>
#include <string>

namespace rocket::str::escape {

// #CStringParams -------------------------------------------------------------------------------------------

/**
 * Parameters for the #escapeCString and #unescapeCString functions.
 */
struct CStringParams {
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
  std::optional<u64> tabSize = std::nullopt;

  /**
    * Checks if the escaped string is to be quoted.
    *
    * @return whether the escaped string is to be quoted
    */
  inline bool quoted() const { return quote != '\0'; }
};

// #Result --------------------------------------------------------------------------------------------------

/**
 * The result of an escape/unescape operation.
 */
struct Result {
  /**
   * Translated positions after escaping/unescaping.
   *
   * For each character in the input string and for EOI, its `char` offset is mapped to a `char` offset in
   * the output string.
   */
  UnorderedBimap<u64, u64> positions;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Escapes an input string to a C string.
 *
 * @param input the input string
 * @param params the parameters, see #rocket::str::escape::CStringParams
 * @param result a pointer to a #rocket::str::escape::Result. If it is nonnull, then the result is populated
 * @return the escaped string
 */
std::string escapeCString(std::string_view input, const CStringParams& params = {}, Result* result = nullptr);

/**
 * Unescapes a C string.
 *
 * @param input the C string
 * @param params the parameters, see #rocket::str::escape::CStringParams
 * @param result a pointer to a #rocket::str::escape::Result. If it is nonnull, then the result is populated
 * @return the unescaped string
 */
std::string unescapeCString(std::string_view input, const CStringParams& params = {}, Result* result = nullptr);

/**
 * Escapes an input string to a regular expression.
 *
 * @param input the input string
 * @param result a pointer to a #rocket::str::escape::Result. If it is nonnull, then the result is populated
 * @return the escaped string
 */
std::string escapeRegex(std::string_view input, Result* result = nullptr);

/**
 * Unescapes a regular expression.
 *
 * @param input the regular expression
 * @param result a pointer to a #rocket::str::escape::Result. If it is nonnull, then the result is populated
 * @return the unescaped string
 */
std::string unescapeRegex(std::string_view input, Result* result = nullptr);

} // namespace rocket::str::escape

// EOF
