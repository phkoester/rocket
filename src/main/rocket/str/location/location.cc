/*
 * location.cc
 */

#include "location.h"

#include "rocket/assert.h"
#include "rocket/enum.h"
#include "rocket/str/str.h"
#include "rocket/str/escape/escape.h"
#include "rocket/unicode/Character.h"
#include "rocket/unicode/Iterator.h"

#include <numeric>

using namespace rocket;
using namespace std;

// `Position::Type` -----------------------------------------------------------------------------------------

ROCKET_ENUM_DEFINE(rocket::str::location, Position::Type, Position_Type, (note)(warning)(error));

namespace rocket::str::location {

// Functions ------------------------------------------------------------------------------------------------

LocationsResult
locations(string_view input, const vector<Position>& positions, const LocationsParams& params) {
  // Prepare result

  LocationsResult ret;
  ret.params = params;
  if (ret.params.source.empty()) {
    ret.params.source = "(input)";
  }

  // Map input position -> location

  u64 maxPos = 0;
  unordered_map<u64, Location> locations;
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

  // Iterate through input

  u64 line = 0, column = 0, beginLine = 0;
  string lineString;

  vector<u64> pois; // "Positions of interest" in the current line
  bool finish = false; // Finish on next line feed?

  auto iter = unicode::Iterator(unicode::IteratorType::Character, input);
  while (true) {
    auto pos = iter.current();

    // Did we reach a position of interest?
    auto it = locations.find(pos);
    if (it != locations.end()) {
      pois.push_back(pos);
      if (pos == maxPos) {
        finish = true;
      }

      auto& loc = it->second;
      loc.line = line + 1; // Marks this location as processed
      loc.column = column + 1;
      loc.lineRange.lower = beginLine;
    }

    // Get next character from iterator, if any
    auto seg = iter.nextSegment();
    optional<unicode::CharacterView<char>> c;
    if (not seg.empty()) {
      c = unicode::CharacterView<char>(seg);
    }

    if (seg.empty() || c->eol()) {
      // Handle EOI or EOL

      // Exit current line
      for (const auto& poi : pois) {
        auto& loi = locations.find(poi)->second; // "Location of interest"
        loi.lineRange.upper = pos;
        if (params.setLineString) {
          loi.lineString = lineString;
        }
      }

      // Finish?
      if (seg.empty() || finish) {
        break;
      }

      // Enter next line
      ++line;
      column = 0;
      beginLine = iter.current();
      lineString.clear();
      pois.clear();
    } else if (c->tab() && params.tabSize) {
      // Handle tab

      u64 mod = column % *params.tabSize;
      u64 n = *params.tabSize - mod;
      column += n;
      lineString.push_back('\t');
    } else {
      // Add the character

      column += c->width();
      lineString.append(*c);
    }
  }

  // Add processed locations to result, preserve order

  for (const auto& pos : positions) {
    const auto& loc = locations.find(pos.position)->second;
    if (loc.line != NPOS) {
      ret.locations.push_back(loc);
    } else {
      ROCKET_FAIL("Position {} not found in source", loc.position);
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
  u64 maxLine = accumulate(
      locations.begin(),
      locations.end(),
      0UL,
      [](u64 max, const auto& loc) { return std::max(loc.line, max); });
  string maxLineStr =  fmt::format("{}", maxLine);
  u64 lineNumberWidth = max(params.minLineNumberWidth, maxLineStr.size());
  string blankPrefix = string(lineNumberWidth, ' ') + " | ";

  for (const auto& loc : locations) {
    // Print source, line number, column column number, type, and message

    ROCKET_CHECK(locationsResult, not loc.message.empty());
    out.print("{}:{}:{}: ", locationsResult.params.source, loc.line, loc.column);
    if (params.styled) {
      fmt::text_style style;
      switch (loc.type) {
      case Position::note: style = fg(fmt::color::cyan); break;
      case Position::warning: style = fg(fmt::color::yellow); break;
      case Position::error: style = fg(fmt::color::red); break;
      }
      out.print(style | fmt::emphasis::bold, "{}: ", loc.type);
    } else {
      out.print("{}: ", loc.type);
    }
    out.writeln(loc.message);

    // Print the line prefix

    out.print("{: >{}d} | ", loc.line, lineNumberWidth);

    // Escape the line as C-string, take tab setting from `locationsResult`, print the line as characers
    // (skip zero-width characters)

    ROCKET_CHECK(input, input || loc.lineString, "Either `input` or `lineString` must be supplied");
    string line = loc.lineString ?
      *loc.lineString :
      string(input->substr(loc.lineRange.lower, *loc.lineRange.size()));
    escape::Result result;
    string escapedLine = escape::escapeCString(line, { .tabSize=locationsResult.params.tabSize }, &result);

    // For `escapedLine`, map `Character` index -> byte offset
    auto iter = unicode::Iterator<char>(unicode::IteratorType::Character, escapedLine);
    auto segs = iter.nextSegments();
    UnorderedBimap<u64, u64> escapedLinePositions;
    u64 escapedLineWidth = 0;
    u64 offset = 0;
    for (u64 i = 0; i < segs.size(); ++i) {
      escapedLinePositions.insert({ i, offset });
      auto c = unicode::CharacterView<char>(segs[i]);
      offset += c.size();
      escapedLineWidth += c.width();
    }
    escapedLinePositions.insert({ segs.size(), offset });

    // Print characters one by one, skip zero-width characters
    for (const auto& c : segs) {
      if (unicode::CharacterView<char>(c).width() > 0) {
        out.write(c);
      }
    }
    out.write('\n');

    // Print the ranges, the caret, and the caption. This is harder than it looks at first sight, because we
    // need to consider C-string escaping, tabs, UTF-8, and `Character` widths---all at the same time

    // Prepare the indicators string. `indicators` is in `Character`-width coordinates

    u64 width = escapedLineWidth;
    string indicators(width, ' ');

    // Make up a lambda that translates an input `char` position to an `indicators` position. This requires
    // several steps

    auto indicatorPos = [&](u64 pos) -> u64 {
      // 1. Translate input position to line position
      pos -= loc.lineRange.lower;

      // 2. Translate line position to escaped-line position
      auto leftIt = result.positions.left.find(pos);
      ROCKET_EXPECT(leftIt != result.positions.left.end());
      pos = leftIt->second;

      // 3. Translate escaped-line position to escaped-line character position
      auto rightIt = escapedLinePositions.right.find(pos);
      ROCKET_EXPECT(rightIt != escapedLinePositions.right.end());
      pos = rightIt->second;

      // 4. Translate escaped-line character position to `indicators` position
      u64 ret = 0;
      for (u64 i = 0; i < pos; ++i) {
        ret += unicode::CharacterView<char>(segs[i]).width();
      }
      return ret;
    };

    // Place the ranges in `indicators`

    for (auto range : loc.ranges) {
      // Obtain the intersection between range and printed line
      if (auto inter = range & loc.lineRange; not inter.empty()) {
        // Translate intersection into `indicators` positions
        u64 lower = indicatorPos(inter.lower);
        u64 upper = indicatorPos(*inter.upper);
        // Place the range
        indicators.replace(indicators.begin() + lower, indicators.begin() + upper, upper - lower, '~');
      }
    }

    // Place the caret in `indicators`

    u64 caretPos = indicatorPos(loc.position);
    if (caretPos < indicators.size()) {
      indicators[caretPos] = '^';
    } else {
      ROCKET_EXPECT(caretPos == indicators.size());
      indicators.push_back('^');
    }

    // Right-trim, print the indicators

    indicators = str::removeTrailing<char>(indicators, " ");
    out.write(blankPrefix);
    if (params.styled) {
      out.print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", indicators);
    } else {
      out.writeln(indicators);
    }

    // If supplied, print caption

    if (loc.caption) {
      string caption = string(caretPos, ' ') + *loc.caption;
      string escapedCaption = escape::escapeCString(caption, { .tabSize=locationsResult.params.tabSize });
      out.write(blankPrefix);
      if (params.styled) {
        out.print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", escapedCaption);
      } else {
        out.writeln(escapedCaption);
      }
    }
  }
}

} // namespace rocket::str::location

// EOF
