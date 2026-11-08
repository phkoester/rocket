/*
 * str.cc
 */

#include "str.h"

#include "rocket/unicode/Character.h"
#include "rocket/unicode/Iterator.h"

using namespace rocket::unicode;
using namespace std;

namespace rocket::str {

// Functions ------------------------------------------------------------------------------------------------

string
capitalize(string_view str) {
  if (str.empty()) {
    return {};
  }

  u64 pos = 0;
  auto cp = nextCodePoint(str, pos);
  string ret = static_cast<string>(cp.upper());
  ret.append(str.substr(pos));
  return ret;
}

u32string
capitalize(u32string_view str) {
  if (str.empty()) {
    return {};
  }
  u32string ret(str);
  ret[0] = CodePoint(str[0]).upper();
  return ret;
}

string
lower(string_view str) {
  u32string localStr = convertUtf8To32(str);
  lowerIn(localStr);
  return convertUtf32To8(localStr);
}

u32string
lower(u32string_view str) {
  u32string ret(str);
  lowerIn(ret);
  return ret;
}

void
lowerIn(u32string& str) {
  ranges::transform(str, str.begin(), [](char32 c) { return CodePoint(c).lower(); });
}

vector<vector<string>>
paragraphs(string_view str) {
  vector<vector<string>> pars; // The paragraphs we collect
  vector<string> par; // The current paragraph
  string word; // The current word

  auto iter = Iterator(IteratorType::Character, str);
  while (true) {
    // Get next character from iterator, if any
    auto seg = iter.nextSegment();
    optional<CharacterView<char>> c;
    if (not seg.empty()) {
      c = CharacterView<char>(seg);
    }

    if (seg.empty() || c->eol()) {
      // EOI/EOL

      if (not word.empty()) {
        par.push_back(word);
        word.clear();
      }
      pars.push_back(par);
      par.clear();

      if (seg.empty()) {
        // EOI
        break;
      }
      // EOL
      continue;
    }

    // Replace tab by space
    if (c->tab()) {
      c = " "_cv;
    }

    if (c->nbsp()) {
      // Handle NO-BREAK SPACE
      word.push_back(' ');
    } else if (not c->isWhitespace()) {
      // Enter/continue word
      word.append(static_cast<string_view>(*c));
    } else {
      // End word, if any
      if (not word.empty()) {
        par.push_back(word);
        word.clear();
      }
    }
  }

  return pars;
}

u64
replaceIn(string& str, string_view from, string_view to, u64 count) {
  ROCKET_CHECK(from, not from.empty());

  if (str.empty() || count == 0) {
    return 0;
  }

  u64 ret = 0;
  for (u64 pos = str.find(from); pos != NPOS; pos = str.find(from, pos + to.size())) {
    str.replace(pos, from.size(), to);
    ++ret;
    if (count != NPOS && ret >= count) {
      break;
    }
  }
  return ret;
}

u64
replaceIn(string& str, const vector<pair<string_view, string_view>>& pairs, u64 count) {
  if (pairs.empty()) {
    return 0;
  }

  const string_view src(str);

  // For each pair, cache the position of its next match. A cached position stays valid until the scan moves
  // past it, so each pair only searches again when a replacement has consumed its match
  vector<u64> matches;
  matches.reserve(pairs.size());
  for (const auto& [from, to] : pairs) {
    ROCKET_CHECK(pairs, not from.empty());
    matches.push_back(src.find(from));
  }

  string dest; // Only allocated once there is a match
  u64 ret = 0;
  u64 cursor = 0; // The end of the part of `src` that has been processed

  while (ret < count) {
    // `min_element` returns the first minimum, so on ties the earliest pair wins. `NPOS` sorts last
    const auto it = ranges::min_element(matches);
    const u64 pos = *it;
    if (pos == NPOS) {
      break;
    }

    if (ret == 0) {
      dest.reserve(src.size());
    }

    const auto& [from, to] = pairs[it - matches.begin()];
    dest.append(src.substr(cursor, pos - cursor));
    dest.append(to);
    cursor = pos + from.size();
    ++ret;

    for (u64 i = 0; i < matches.size(); ++i) {
      if (matches[i] < cursor) {
        matches[i] = src.find(pairs[i].first, cursor);
      }
    }
  }

  if (ret > 0) {
    dest.append(src.substr(cursor));
    str = std::move(dest);
  }
  return ret;
}

string
times(u64 n) {
  if (n == 1) {
    return "once";
  }
  if (n == 2) {
    return "twice";
  }
  return fmt::format("{} times", n);
}

string
upper(string_view str) {
  u32string localStr = convertUtf8To32(str);
  upperIn(localStr);
  return convertUtf32To8(localStr);
}

u32string
upper(u32string_view str) {
  u32string ret(str);
  upperIn(ret);
  return ret;
}

void
upperIn(u32string& str) {
  ranges::transform(str, str.begin(), [](char32 c) { return CodePoint(c).upper(); });
}

string
wrap(string_view str, u64 leftIndent, u64 width) {
  ROCKET_CHECK(width, width > leftIndent);

  width -= leftIndent;

  // Collect paragraphs, consisting of words

  const vector<vector<string>> pars = paragraphs(str);

  // Turn paragraphs into lines

  vector<string> lines; // The lines we collect

  // Loop through paragraphs

  for (const auto& par : pars) {
    string line; // The current line
    u64 lineWidth = 0; // The display width of the current line

    for (const auto& word : par) {
      u64 wordWidth = 0;
      auto iter = Iterator<char>(IteratorType::Character, word);
      auto segs = iter.nextSegments();
      for (const auto& seg : segs) {
        wordWidth += CharacterView<char>(seg).width();
      }
      const u64 newLineWidth = lineWidth + wordWidth + (lineWidth > 0 ? 1 : 0);
      if (lineWidth == 0 || newLineWidth < width) {
        if (lineWidth > 0) {
          line.push_back(' ');
        }
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
  const string indent(leftIndent, ' ');
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

} // namespace rocket::str

// EOF
