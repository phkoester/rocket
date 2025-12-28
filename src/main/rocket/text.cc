/*
 * text.cc
 */

#include "text.h"

#include "assert.h"
#include "escape.h"
#include "enum.h"
#include "strings.h"
#include "terminal.h"
#include "unicode.h"

#include <numeric>

using namespace rocket;
using namespace std;

namespace rocket::text {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

vector<vector<string>>
paragraphs(string_view s) {
  vector<vector<string>> pars; // The paragraphs we collect
  vector<string> par; // The current paragraph
  string word; // The current word

  auto it = unicode::GraphemeIterator<char>(s);
  while (true) {
    if (it.end() || it->eol()) {
      // Handle EOT/EOL
      if (not word.empty()) {
        par.push_back(word);
        word.clear();
      }
      pars.push_back(par);
      par.clear();

      if (it.end()) {
        break;
      }
      ++it;
      continue;
    }

    unicode::Grapheme gr(*it);
    if (gr.tab()) {
      gr = unicode::Grapheme(U" ");
    }

    if (gr.nbsp()) {
      // Handle NBSP
      if (not word.empty() && not strings::endsWith<char>(word, " "))
        word.push_back(' ');
    } else if (not gr.whitespace()) {
      // Enter/continue word
      word.append(static_cast<string>(gr));
    } else {
      // End word, if any
      if (not word.empty()) {
        par.push_back(word);
        word.clear();
      }
    }

    ++it;
  }

  return pars;
}

} // namespace internal

// `Position` -----------------------------------------------------------------------------------------------

ROCKET_ENUM_DEFINE(Position::Type, Position_Type, (note)(warning)(error));

// Functions ------------------------------------------------------------------------------------------------

LocationsResult
locations(istream& is, const vector<Position>& positions, const LocationsParams& params) {
  ROCKET_CHECK(is, io::tellg(is) == 0, "Input stream must be at position 0");

  // Prepare result

  LocationsResult ret;
  ret.params = params;
  if (ret.params.source.empty()) {
    ret.params.source = &is == &cin ? "-" : "(input)";
  }

  // Map input position -> location

  size_t maxPos = 0;
  unordered_map<size_t, LocationsResult::Location> locations;
  for (const auto& pos : positions) {
    ROCKET_CHECK(positions, not pos.message.empty());
    ROCKET_CHECK(positions, not (pos.caption && pos.caption->empty()));
    auto emplaceResult = locations.emplace(pos.position, LocationsResult::Location {
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

  // Use a byte buffer to read the input stream

  io::Buffer buf(is, params.bufferSize);

  size_t line = 0, column = 0, beginLine = 0;
  string lineString;

  vector<size_t> poi; // "Positions of interest" in the current line
  bool finish = false; // Finish on next line feed?

  while (true) {
    auto pos = buf.position();

    // Did we reach a position of interest?
    auto it = locations.find(pos);
    if (it != locations.end()) {
      poi.push_back(pos);
      if (pos == maxPos)
        finish = true;

      auto& loc = it->second;
      loc.line = line + 1; // Marks this location as processed
      loc.column = column + 1;
      loc.lineRange.lower = beginLine;
    }

    // Get next grapheme from buffer, if any
    unicode::Grapheme gr;
    auto bytes = buf.getGrapheme(&gr);
    if (not bytes || gr.eol()) {
      // Handle EOF or EOL

      // Exit current line
      for (const auto& p : poi) {
        auto& loi = locations.find(p)->second; // "Location of interest"
        loi.lineRange.upper = pos;
        if (params.setLineString)
          loi.lineString = lineString;
      }

      // Finish?
      if (not bytes || finish)
        break;

      // Enter next line
      ++line;
      column = 0;
      beginLine = buf.position(); // This is `pos + bytes->size()`
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
      for_each(bytes->begin(), bytes->end(), [&](byte b) { lineString.push_back(static_cast<char>(b)); } );
    }
  }

  // Add processed locations to result, preserve order

  for (const auto& pos : positions) {
    const auto& loc = locations.find(pos.position)->second;
    if (loc.line != NPOS) {
      ret.locations.push_back(loc);
    } else {
      throw InvalidState(fmt::format("Position {} not found in input stream", loc.position));
    }
  }

  return ret;
}

void
printLocations(
    ostream& os,
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
      [](size_t max, auto&& loc) { return std::max(loc.line, max); });
  ostringstream oss;
  oss << maxLine;
  size_t lineNumberWidth = max(params.minLineNumberWidth, oss.str().size());
  string blankPrefix = string(lineNumberWidth, ' ') + " | ";

  using namespace terminal;
  Ansi ansi(params.colored && io::isatty(os));

  for (const auto& loc : locations) {
    // Print source, line number, column column number, type, and message

    ROCKET_CHECK(locationsResult, not loc.message.empty());
    os << locationsResult.params.source << ':' << loc.line << ':' << loc.column << ": ";
    switch (loc.type) {
    case Position::note: os << ansi.style(bold | green); break;
    case Position::warning: os << ansi.style(bold | yellow); break;
    case Position::error: os << ansi.style(bold | red); break;
    }
    os << loc.type << ": " << ansi.style() << loc.message << '\n';

    // Print the line prefix

    string linePrefix = fmt::format("{: >{}d}", loc.line, lineNumberWidth);
    linePrefix += " | ";
    os << linePrefix;

    // Escape the line as C-string, take tab setting from `locationsResult`, print the line as graphemes
    // (skip zero-width graphemes)

    ROCKET_CHECK(input, input || loc.lineString, "Either `input` or `lineString` must be supplied");
    string line = loc.lineString ?
      *loc.lineString :
      string(input->substr(loc.lineRange.lower, *loc.lineRange.size()));
    escape::Result result;
    oss.str("");
    oss << escape::escaped<escape::CString>(line, { .tabSize=locationsResult.params.tabSize }, &result);
    string escapedLine = oss.str();
    container::UnorderedBimap<size_t, size_t> grsp;
    unicode::Graphemes grs = unicode::graphemes(escapedLine, &grsp);
    // Print graphemes one by one, skip zero-width graphemes
    for (const auto& gr : grs) {
      if (gr.width > 0) {
        os << static_cast<string>(gr);
      }
    }
    os << '\n';

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

    indicators = strings::removeTrailing<char>(indicators, " ");
    os << blankPrefix << ansi.style(bold | green) << indicators << ansi.style() << '\n';

    // If supplied, print caption

    if (loc.caption) {
      string caption = string(caretPos, ' ') + *loc.caption;
      oss.str("");
      oss << escape::escaped<escape::CString>(caption, { .tabSize=locationsResult.params.tabSize });
      string escapedCaption = oss.str();
      os << blankPrefix << ansi.style(green) << escapedCaption << ansi.style() << "\n";
    }
  }
}

string
wrap(string_view s, const WrapParams& params) {
  size_t width = params.width - params.leftIndent;

  // Collect paragraphs, consisting of words

  vector<vector<string>> pars = internal::paragraphs(s);

  // Turn paragraphs into lines

  vector<string> lines; // The lines we collect

  // Loop through paragraphs

  for (const auto& par : pars) {
    string line; // The current line
    size_t lineWidth = 0; // The display width of the current line

    for (const auto& word : par) {
      auto grs = unicode::graphemes(word);
      size_t wordWidth = unicode::width(grs);
      size_t newLineWidth = lineWidth + wordWidth + (lineWidth > 0 ? 1 : 0);
      if (lineWidth == 0 || newLineWidth < width) {
        if (lineWidth > 0)
          line.push_back(' ');
        line.append(word);
        lineWidth = newLineWidth;
      } else {
        lines.push_back(line);
        line = word;
        lineWidth = wordWidth;
      }
    }

    lines.push_back(line);
  }

  // Concatenate lines, consider left indent

  string ret;
  string indent(params.leftIndent, ' ');
  bool first = true;
  for (const auto& line : lines) {
    if (first) {
      first = false;
    } else {
      ret.push_back('\n');
    }
    ret.append(indent);
    ret.append(line);
  }
  return ret;
}

} // namespace rocket::text

// Namespace `fmt` ------------------------------------------------------------------------------------------

ROCKET_ENUM_DEFINE_FMT_FORMATTER(rocket::text, Position::Type, Position_Type);

// EOF
