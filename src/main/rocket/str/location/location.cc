/*
 * location.cc
 */

#include "location.h"

#include "rocket/assert.h"
#include "rocket/enum.h"
#include "rocket/str/str.h"
#include "rocket/str/escape/escape.h"
#include "rocket/system/terminal/terminal.h"
#include "rocket/unicode/iterator.h"

#include <numeric>

using namespace rocket;
using namespace std;

// `Position::Type` -----------------------------------------------------------------------------------------

ROCKET_ENUM_DEFINE(rocket::str::location, Position::Type, Position_Type, (note)(warning)(error));

namespace rocket::str::location {

// Functions ------------------------------------------------------------------------------------------------

LocationsResult
locations(nio::Source& in, const vector<Position>& positions, const LocationsParams& params) {
  ROCKET_CHECK(in, in.tell() == 0, "Source must be at position 0");

  // Prepare result

  LocationsResult ret;
  ret.params = params;
  if (ret.params.source.empty()) {
    ret.params.source = in.fd() == STDIN_FILENO ? "-" : "(input)";
  }

  // Map input position -> location

  size_t maxPos = 0;
  unordered_map<size_t, Location> locations;
  for (const auto& pos : positions) {
    ROCKET_CHECK(positions, not pos.message.empty());
    ROCKET_CHECK(positions, not (pos.caption && pos.caption->empty()));
    auto emplaceResult = locations.emplace(pos.position, Location {
      .type = pos.type,
      .position = pos.position,
      .ranges = pos.ranges,
      .line = NPOS, // Marks this location as unprocessed
      .message = pos.message,
      .caption = pos.caption
    });
    ROCKET_CHECK(positions, emplaceResult.second, "Duplicate position {}", pos.position);

    maxPos = max(pos.position, maxPos);
  }

  // Read from source

  size_t line = 0, column = 0, beginLine = 0;
  string lineString;

  vector<size_t> poi; // "Positions of interest" in the current line
  bool finish = false; // Finish on next line feed?

  while (true) {
    auto pos = in.tell();

    // Did we reach a position of interest?
    auto it = locations.find(pos);
    if (it != locations.end()) {
      poi.push_back(pos);
      if (pos == maxPos) {
        finish = true;
      }

      auto& loc = it->second;
      loc.line = line + 1; // Marks this location as processed
      loc.column = column + 1;
      loc.lineRange.lower = beginLine;
    }

    // Get next grapheme from source, if any
    unicode::Grapheme gr;
    size_t n = read(in, gr);
    if (n == 0 || gr.eol()) {
      // Handle EOF or EOL

      // Exit current line
      for (const auto& p : poi) {
        auto& loi = locations.find(p)->second; // "Location of interest"
        loi.lineRange.upper = pos;
        if (params.setLineString)
          loi.lineString = lineString;
      }

      // Finish?
      if (n == 0 || finish)
        break;

      // Enter next line
      ++line;
      column = 0;
      beginLine = in.tell();
      lineString.clear();
      poi.clear();
    } else if (gr.tab() && params.tabSize) {
      // Handle tab

      size_t mod = column % *params.tabSize;
      size_t n = *params.tabSize - mod;
      column += n;
      lineString.push_back('\t');
    } else {
      // Add the grapheme

      column += gr.width;
      lineString.append(static_cast<string>(gr));
    }
  }

  // Add processed locations to result, preserve order

  for (const auto& pos : positions) {
    const auto& loc = locations.find(pos.position)->second;
    if (loc.line != NPOS) {
      ret.locations.push_back(loc);
    } else {
      throw InvalidState(fmt::format("Position {} not found in source", loc.position));
    }
  }

  return ret;
}

void
printLocations(
    nio::Sink& out,
    optional<string_view> input,
    const LocationsResult& locationsResult,
    const PrintLocationsParams& params) {
  ROCKET_CHECK(input, not (input && input->empty()), "May not be empty");
  ROCKET_CHECK(locationsResult, not locationsResult.params.source.empty());

  // Find out line-number width and format
  const auto& locations = locationsResult.locations;
  size_t maxLine = accumulate(
      locations.begin(),
      locations.end(),
      0UL,
      [](size_t max, const auto& loc) { return std::max(loc.line, max); });
  string maxLineStr =  fmt::format("{}", maxLine);
  size_t lineNumberWidth = max(params.minLineNumberWidth, maxLineStr.size());
  string blankPrefix = string(lineNumberWidth, ' ') + " | ";

  using namespace system::terminal;
  Ansi ansi(params.colored && ::isatty(out.fd()));

  for (const auto& loc : locations) {
    // Print source, line number, column column number, type, and message

    ROCKET_CHECK(locationsResult, not loc.message.empty());
    out.print("{}:{}:{}: ", locationsResult.params.source, loc.line, loc.column);
    switch (loc.type) {
    case Position::note: out.write(ansi.style(bold | green)); break;
    case Position::warning: out.write(ansi.style(bold | yellow)); break;
    case Position::error: out.write(ansi.style(bold | red)); break;
    }
    out.println("{}: {}{}", loc.type, ansi.style(), loc.message);

    // Print the line prefix

    out.print("{: >{}d} | ", loc.line, lineNumberWidth);

    // Escape the line as C-string, take tab setting from `locationsResult`, print the line as graphemes
    // (skip zero-width graphemes)

    ROCKET_CHECK(input, input || loc.lineString, "Either `input` or `lineString` must be supplied");
    string line = loc.lineString ?
      *loc.lineString :
      string(input->substr(loc.lineRange.lower, *loc.lineRange.size()));
    escape::Result result;
    string escapedLine = escape::escapeCString(line, { .tabSize=locationsResult.params.tabSize }, &result);
    UnorderedBimap<size_t, size_t> grsp;
    unicode::Graphemes grs = unicode::graphemes(escapedLine, &grsp);
    // Print graphemes one by one, skip zero-width graphemes
    for (const auto& gr : grs) {
      if (gr.width > 0) {
        out.write(static_cast<string>(gr));
      }
    }
    out.write('\n');

    // Print the ranges, the caret, and the caption. This is harder than it looks at first sight, because we
    // need to consider C-string escaping, tabs, UTF-8, and grapheme widths---all at the same time

    // Prepare the indicators string. `indicators` is in "grapheme-width coordinates"

    size_t width = unicode::width(grs);
    string indicators(width, ' ');

    // Make up a lambda that translates an input `char` position to an `indicators` position. This requires
    // several steps

    auto indicatorPos = [&](size_t pos) -> size_t {
      // 1. Translate input position to line position
      pos -= loc.lineRange.lower;

      // 2. Translate line position to escaped-line position
      auto leftIt = result.positions.left.find(pos);
      ROCKET_EXPECT(leftIt != result.positions.left.end());
      pos = leftIt->second;

      // 3. Translate escaped-line position to escaped-line grapheme position
      auto rightIt = grsp.right.find(pos);
      ROCKET_EXPECT(rightIt != grsp.right.end());
      pos = rightIt->second;

      // 4. Translate escaped-line grapheme position to `indicators` position
      return unicode::width(grs, 0, pos);
    };

    // Place the ranges in `indicators`

    for (auto range : loc.ranges) {
      // Obtain the intersection between range and printed line
      if (auto inter = range & loc.lineRange) {
        // Translate intersection into `indicators` positions
        size_t lower = indicatorPos(inter.lower);
        size_t upper = indicatorPos(*inter.upper);
        // Place the range
        indicators.replace(indicators.begin() + lower, indicators.begin() + upper, upper - lower, '~');
      }
    }

    // Place the caret in `indicators`

    size_t caretPos = indicatorPos(loc.position);
    if (caretPos < indicators.size()) {
      indicators[caretPos] = '^';
    } else {
      ROCKET_EXPECT(caretPos == indicators.size());
      indicators.push_back('^');
    }

    // Right-trim, print the indicators

    indicators = str::removeTrailing<char>(indicators, " ");
    out.println("{}{}{}{}", blankPrefix, ansi.style(bold | green), indicators, ansi.style());

    // If supplied, print caption

    if (loc.caption) {
      string caption = string(caretPos, ' ') + *loc.caption;
      string escapedCaption = escape::escapeCString(caption, { .tabSize=locationsResult.params.tabSize });
      out.println("{}{}{}{}", blankPrefix, ansi.style(green), escapedCaption, ansi.style());
    }
  }
}

} // namespace rocket::str::location

// EOF
