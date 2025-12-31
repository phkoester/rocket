/**
 * @file escape.h
 *
 * Escaped strings, offering an interface similar to `std::quoted`.
 */

#pragma once

#include "container.h"

#include <optional>
#include <string>

namespace rocket::escape {

// `CStringParams` ------------------------------------------------------------------------------------------

struct CStringParams {
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

// `Result` -------------------------------------------------------------------------------------------------

/**
 * The result of an escape/unescape operation.
 */
struct Result {
  /**
   * Translated positions after escaping/unescaping.
   *
   * For each grapheme in the input string and for end-of-string, its character offset—i.e. either its `char`
   * or `char32_t` offset—, is mapped to a character offset in the output string.
   */
  container::UnorderedBimap<size_t, size_t> positions;
};

// Functions ------------------------------------------------------------------------------------------------

std::string escapeCString(std::string_view input, const CStringParams& params = {}, Result* result = nullptr);

std::string unescapeCString(std::string_view input, const CStringParams& params = {}, Result* result = nullptr);

std::string escapeRegex(std::string_view input, Result* result = nullptr);

std::string unescapeRegex(std::string_view input, Result* result = nullptr);

} // namespace rocket::escape

// EOF
