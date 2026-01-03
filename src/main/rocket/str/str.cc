/*
 * str.cc
 */

#include "str.h"

#include "rocket/unicode/unicode.h"
#include "rocket/unicode/iterator.h"

#include <algorithm>

using namespace std;

namespace rocket::str {

// Functions ------------------------------------------------------------------------------------------------

string
capitalize(string_view s) {
  if (s.empty())
    return string();
  unicode::CodePointIterator<char> it(s);
  unicode::CodePoint upper = (*it++).upper();
  string ret = static_cast<string>(upper);
  ret.append(s.substr(it.position()));
  return ret;
}

u32string
capitalize(u32string_view s) {
  if (s.empty())
    return u32string();
  u32string ret(s);
  ret[0] = unicode::CodePoint(s[0]).upper();
  return ret;
}

string
lower(string_view s) {
  u32string localS = unicode::utf8To32(s);
  lowerIn(localS);
  return unicode::utf32To8(localS);
}

u32string
lower(u32string_view s) {
  u32string ret(s);
  lowerIn(ret);
  return ret;
}

void
lowerIn(u32string& s) {
  transform(s.begin(), s.end(), s.begin(), [](char32_t c) { return unicode::CodePoint(c).lower(); });
}

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
      if (not word.empty() && not str::endsWith<char>(word, " ")) {
        word.push_back(' ');
      }
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

string
upper(string_view s) {
  u32string localS = unicode::utf8To32(s);
  upperIn(localS);
  return unicode::utf32To8(localS);
}

u32string
upper(u32string_view s) {
  u32string ret(s);
  upperIn(ret);
  return ret;
}

void
upperIn(u32string& s) {
  transform(s.begin(), s.end(), s.begin(), [](char32_t c) { return unicode::CodePoint(c).upper(); });
}

string
wrap(string_view s, size_t leftIndent, size_t width) {
  width -= leftIndent;

  // Collect paragraphs, consisting of words

  vector<vector<string>> pars = paragraphs(s);

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
