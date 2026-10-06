/**
 * @file escape.h
 *
 * Escaped strings, offering an interface similar to #std::quoted.
 */

#pragma once

#include "rocket/Bimap.h"
#include "rocket/rocket.h"

#include <optional>
#include <string>

namespace rocket::str::escape {

// `CStringConfig` ------------------------------------------------------------------------------------------

/**
 * Configuration for the #escapeCString and #unescapeCString functions.
 */
struct CStringConfig {
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
  [[nodiscard]] bool quoted() const { return quote != '\0'; }
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Escapes an input string to a C string.
 *
 * @param input the input string
 * @param config the configuration, see #rocket::str::escape::CStringConfig
 * @param positions a pointer to a #rocket::Positions. If it is nonnull, then the positions are populated
 * @return the escaped string
 */
std::string escapeCString(
  std::string_view input,
  const CStringConfig& config = {},
  Positions* positions = nullptr);

/**
 * Unescapes a C string.
 *
 * @param input the C string
 * @param config the configuration, see #rocket::str::escape::CStringConfig
 * @param positions a pointer to a #rocket::Positions. If it is nonnull, then the positions are populated
 * @return the unescaped string
 */
std::string unescapeCString(
  std::string_view input,
  const CStringConfig& config = {},
  Positions* positions = nullptr);

/**
 * Escapes an input string to a regular expression.
 *
 * @param input the input string
 * @param positions a pointer to a #rocket::Positions. If it is nonnull, then the positions are populated
 * @return the escaped string
 */
std::string escapeRegex(std::string_view input, Positions* positions = nullptr);

/**
 * Unescapes a regular expression.
 *
 * @param input the regular expression
 * @param positions a pointer to a #rocket::Positions. If it is nonnull, then the positions are populated
 * @return the unescaped string
 */
std::string unescapeRegex(std::string_view input, Positions* positions = nullptr);

} // namespace rocket::str::escape

// EOF
