/**
 * @file location.h
 *
 * Positions and locations.
 */

#pragma once

#include "rocket/enum.h"
#include "rocket/nio/nio-fwd.h"
#include "rocket/str/Range.h"

#include <vector>

namespace rocket::str::location {

// #PositionType --------------------------------------------------------------------------------------------

/**
 * An enum describing the position type.
 */
enum PositionType {
  note,    ///< A note.
  warning, ///< A warning.
  error    ///< An error.
};

} // namespace rocket::str::location

/// @enum_declare{#rocket::str::location::PositionType}
ROCKET_ENUM_DECLARE(rocket::str::location, PositionType, PositionType);

namespace rocket::str::location {

// #Position ------------------------------------------------------------------------------------------------

/**
 * Input positions that are passed to the #locations function.
 */
struct Position {
  /**
   * The position type.
   */
  PositionType type;
  /**
   * The position to look for.
   *
   * There must be a character bondary at this position. The position is highlighted with a caret (`^`)
   * underneath.
   */
  u64 position;
  /**
   * The ranges associated with this position.
   *
   * There must be character bondaries at the ranges' lower and upper positions. The ranges are underlined
   * with the tilde (`~`).
   */
  Ranges ranges = Ranges();
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
  std::optional<std::string> caption = std::nullopt;
};

// #Location ------------------------------------------------------------------------------------------------

/**
 * Location information.
 */
struct Location {
  PositionType type; ///< Copied from the input position.
  u64 position; ///< Copied from the input position.
  Ranges ranges; ///< Copied from the input position.
  u64 line; ///< The line number, starting with 1.
  u64 column = 0; ///< The column number (counting character widths), starting with 1.
  Range lineRange = Range(); ///< The range of the line containing #position.
  /**
    * This member is only initialized if #LocationsConfig#setLineString was set to `true`.
    */
  std::optional<std::string> lineString = std::nullopt;
  std::string message; ///< Copied from the input position.
  std::optional<std::string> caption; ///< Copied from the input position.
};

// #LocationsConfig -----------------------------------------------------------------------------------------

/**
 * Configuration for the #locations function.
 */
struct LocationsConfig {
  /**
   * If this is set to `true`, then lines are copied to the #rocket::str::location::LocationsResult.
   */
  bool setLineString = false;
  /**
   * A string describing the source of the data.
   *
   * If a source is known, such as a file or an URL, it should be assigned here. If #source is empty, then
   * the #locations function sets this to `"-"` if the source is `stdin`, to `"(input)"` otherwise.
   */
  std::string source = std::string();
  /**
   * Configures the handling of tab characters. If this is null, then there is no special treatment for tab
   * characters---they are displayed as `\t`. Otherwise, a tab expands to at most #tabSize spaces.
   */
  std::optional<u64> tabSize = 8;
};

// #LocationsResult -----------------------------------------------------------------------------------------

/**
 * This is the result of a call to #locations.
 *
 * This class contains information about each inquired position.
 */
struct LocationsResult {
  /**
   * A copy of the configuration that was passed to #locations.
   *
   * The #LocationsConfig#source member is possibly assigned a new value.
   */
  LocationsConfig config;
  /**
   * For each #rocket::str::location::Position passed to #locations, a #rocket::str::location::Location is
   * added to the result. The order of the positions is preserved in the
   * #rocket::str::location::LocationsResult.
   */
  std::vector<Location> locations;
};

// #PrintLocationsConfig ------------------------------------------------------------------------------------

/**
 * Configuration for the #printLocations function.
 */
struct PrintLocationsConfig {
  bool styled = false; ///< Print styled text?
  u64 minLineNumberWidth = 5; ///< The minimum width to use when displaying line numbers.
};

// Functions ------------------------------------------------------------------------------------------------

/**
 * Finds information about the positions @p positions in the Source @p in and returns the gathered data in a
 * #rocket::str::location::LocationsResult.
 *
 * @param input the input string. It must be UTF-8-encoded, using LF (`"\n"`) or CRLF (`"\r\n"`) as line
 *     breaks
 * @param positions the positions to look for. They needn't be sorted in any way. The order of the positions
 *     is preserved in the #rocket::str::location::LocationsResult. The only restriction is that all
 *     #rocket::str::location::Position#position values have to be unique
 * @param config configuration
 * @return a #rocket::str::location::LocationsResult
 */
LocationsResult locations(
    std::string_view input,
    const std::vector<Position>& positions,
    const LocationsConfig& config = {});

/**
 * Prints Clang-style messages for all locations in @p locations to the sink @p out.
 *
 * @param out the sink to print to
 * @param input the entire input as a string view. This may be null, but then, the line strings in
 *     @p locationsResult must be available
 * @param locationsResult the #rocket::str::location::LocationsResult instance that was returned by the
 *     #rocket::str::location::locations function
 * @param config configuration
 */
void printLocations(
    nio::Sink& out,
    std::optional<std::string_view> input,
    const LocationsResult& locationsResult,
    const PrintLocationsConfig& config = {});

} // namespace rocket::str::location

// EOF
