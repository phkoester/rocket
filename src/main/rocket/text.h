/**
 * @file text.h
 *
 * Text utilities, higher-order string functions.
 */

#pragma once

#include "enum-decl.h"
#include "io-decl.h"
#include "math.h"

#include <vector>

namespace rocket::text {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

std::vector<std::vector<std::string>> paragraphs(std::string_view s);

} // namespace internal

// `Range` --------------------------------------------------------------------------------------------------

/**
 * The type used to store a position range.
 */
using Range = math::RightOpenInterval<size_t>;

// `Ranges` -------------------------------------------------------------------------------------------------

/**
 * The type used to store position ranges.
 */
using Ranges = std::vector<Range>;

// `Position` -----------------------------------------------------------------------------------------------

/**
 * Input positions that are passed to the #locations function.
 */
struct Position {
  /**
   * An enum describing the position type.
   */
  enum Type { note, warning, error };

  /**
   * The position type.
   */
  Type type;
  /**
   * The position to look for.
   *
   * There must be a grapheme boundary at this position. The position is highlighted with a caret (`^`)
   * underneath.
   */
  size_t position;
  /**
   * The ranges associated with this position.
   *
   * There must be grapheme boundaries at the ranges' lower and upper positions. The ranges are underlined
   * with the tilde (`~`).
   */
  Ranges ranges;
  /**
   * The message associated with this position.
   *
   * THe message may not be empty. It is part of the error message.
   */
  std::string message;
  /**
   * The optional caption associated with this position.
   *
   * The caption may be null, bot not empty. It is displayed underneath the caret (`^`).
   */
  std::optional<std::string> caption;
};

// `LocationsParams` ----------------------------------------------------------------------------------------

/**
 * Parameters for the #locations function.
 */
struct LocationsParams {
  /**
   * The size of the internal buffer used for reading the input stream.
   *
   * @see #rocket::io::DEFAULT_BUFFER_SIZE
   */
  size_t bufferSize = io::DEFAULT_BUFFER_SIZE;
  /**
   * If this is set to `true`, then lines are copied to the #rocket::text::LocationsResult.
   */
  bool setLineString = false;
  /**
   * A string describing the source of the data.
   *
   * If a source is known, such as a file or an URL, it should be assigned here. If #source is empty, then
   * the #locations function sets this to `"-"` if the input stream is `std::cin`, to `"(input)"` otherwise.
   */
  std::string source;
  /**
   * Configures the handling of tab characters. If this is null, then there is no special treatment for tab
   * characters---they are displayed as `\t`. Otherwise, a tab expands to at most #tabSize spaces.
   */
  std::optional<size_t> tabSize = 8;
};

// `LocationsResult` ----------------------------------------------------------------------------------------

/**
 * This is the result of a call to #locations.
 *
 * This class contains information about each inquired position.
 */
struct LocationsResult {
  /**
   * Location information.
   */
  struct Location {
    Position::Type type; ///< Copied from the input position.
    size_t position; ///< Copied from the input position.
    Ranges ranges; ///< Copied from the input position.
    size_t line; ///< The line number, starting with 1.
    size_t column; ///< The column number (counting Unicode grapheme widths), starting with 1.
    Range lineRange; ///< The range of the line containing #position.
    /**
     * This member is only initialized if #LocationsParams#setLineString was set to `true`.
     */
    std::optional<std::string> lineString;
    std::string message; ///< Copied from the input position.
    std::optional<std::string> caption; ///< Copied from the input position.
  };

  /**
   * A copy of the parameters that were passed to #locations.
   *
   * The #LocationsParams#source member is possibly assigned a new value.
   */
  LocationsParams params;
  /**
   * For each #rocket::text::Position passed to #locations, a #rocket::text::LocationsResult::Location is
   * added to the result. The order of the positions is preserved in the #rocket::text::LocationsResult.
   */
  std::vector<Location> locations;
};

// `PrintLocationsParams` -----------------------------------------------------------------------------------

/**
 * Parameters for the #printLocations function.
 */
struct PrintLocationsParams {
  bool colored = true; ///< Use colors when printing to a terminal?
  size_t minLineNumberWidth = 5; ///< The minimum width to use when displaying line numbers.
};

// `WrapParams` ---------------------------------------------------------------------------------------------

/**
 * Parameters for the #wrap function.
 */
struct WrapParams {
  /**
   * Columns to indent.
   */
  size_t leftIndent = 0;
  /**
   * Maximum line width. For example, if this is 80, created lines will be at most 79 columns wide (in
   * Unicode grapheme-width coordinates) if word wrapping is feasible.
   *
   * @see #rocket::terminal::size
   */
  size_t width = 80;
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Finds information about the positions @p positions in the input stream @p is and returns the gathered
 * data in a #rocket::text::LocationsResult.
 *
 * @param is the input stream. The input must be UTF-8-encoded, using LF (`"\n"`) or CRLF (`"\r\n"`) as
 *     line breaks
 * @param positions the positions to look for. They needn't be sorted in any way. The order of the positions
 *     is preserved in the #rocket::text::LocationsResult. The only restriction is that all
 *     #rocket::text::Position#position values have to be unique
 * @param params parameters to configure the operation
 * @return a #rocket::text::LocationsResult
 */
LocationsResult locations(
    std::istream& is,
    const std::vector<Position>& positions,
    const LocationsParams& params = {});

/**
 * Prints Clang-style messages for all locations in @p locations to the output stream @p os.
 *
 * @param os the output stream
 * @param input the entire input as a string view. This may be null, but then, the line strings in
 *     @p locationsResult must be available
 * @param locationsResult the #rocket::text::LocationsResult instance that was returned by the
 *    #rocket::text::locations function
 * @param params parameters to configure the operation
 */
void printLocations(
    std::ostream& os,
    std::optional<std::string_view> input,
    const LocationsResult& locationsResult,
    const PrintLocationsParams& params = {});

/**
 * Wraps the string @p s to fit the width specified by @p params.
 *
 * - Line breaks (`"\n"`, `"\r\n"`) are recognized.
 * - Non-breaking spaces (U+00A0) are recognized.
 * - Tabs are replaced by spaces.
 * - Consecutive whitespace is collapsed.
 *
 * @param s the string to wrap
 * @param params parameters configuring the operation
 * @return a new string
 */
std::string wrap(std::string_view s, const WrapParams& params = {});

} // namespace rocket::text

ROCKET_ENUM_DECLARE_FMT_FORMATTER(rocket::text::Position::Type);

// EOF
