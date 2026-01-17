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
capitalize(string_view s) {
  if (s.empty()) {
    return string();
  }

  u64 pos = 0;
  auto cp = nextCodePoint(s, pos);
  string ret = static_cast<string>(cp.upper());
  ret.append(s.substr(pos));
  return ret;
}

u32string
capitalize(u32string_view s) {
  if (s.empty())
    return u32string();
  u32string ret(s);
  ret[0] = CodePoint(s[0]).upper();
  return ret;
}

string
lower(string_view s) {
  u32string localS = utf8To32(s);
  lowerIn(localS);
  return utf32To8(localS);
}

u32string
lower(u32string_view s) {
  u32string ret(s);
  lowerIn(ret);
  return ret;
}

void
lowerIn(u32string& s) {
  transform(s.begin(), s.end(), s.begin(), [](char32 c) { return CodePoint(c).lower(); });
}

vector<vector<string>>
paragraphs(string_view s) {
  vector<vector<string>> pars; // The paragraphs we collect
  vector<string> par; // The current paragraph
  string word; // The current word

  auto iter = Iterator(IteratorType::Character, s);
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
      word.append(*c);
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

string
upper(string_view s) {
  u32string localS = utf8To32(s);
  upperIn(localS);
  return utf32To8(localS);
}

u32string
upper(u32string_view s) {
  u32string ret(s);
  upperIn(ret);
  return ret;
}

void
upperIn(u32string& s) {
  transform(s.begin(), s.end(), s.begin(), [](char32 c) { return CodePoint(c).upper(); });
}

string
wrap(string_view s, u64 leftIndent, u64 width) {
  width -= leftIndent;

  // Collect paragraphs, consisting of words

  vector<vector<string>> pars = paragraphs(s);

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
      u64 newLineWidth = lineWidth + wordWidth + (lineWidth > 0 ? 1 : 0);
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
  string indent(leftIndent, ' ');
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
