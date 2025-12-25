/*
 * io.cc
 */

#include "codec-std-decl.h"
#include "codec-std.h"

#include "io.h"

#include "S.h"
#include "assert.h"

#include <memory>

using namespace std;

namespace rocket::io {

// `Buffer` -------------------------------------------------------------------------------------------------

Buffer::Buffer(istream& is, size_t size) :
    is_(is),
    size_(size),
    pos_(tellg(is)) {
  ROCKET_CHECK(size, size >= MIN_BUFFER_SIZE);
  p_ = make_unique<byte[]>(size);
}

optional<byte>
Buffer::get() {
  // If available, obtain byte that was put back
  if (not intermediate_.empty()) {
    byte ret = intermediate_.back();
    intermediate_.pop_back();
    ++pos_;
    return ret;
  }

  // If needed, load the next chunk of data
  if (index_ == end_) {
    is_.read(reinterpret_cast<char*>(p_.get()), size_);
    index_ = 0;
    end_ = is_.gcount();
    if (end_ == 0)
      return nullopt;
  }
  // Return the next byte
  ++pos_;
  return p_.get()[index_++];
}

optional<vector<byte>>
Buffer::getCodePoint(unicode::CodePoint* cp) {
  size_t pos = pos_;
  auto got = get();
  if (not got)
    return nullopt;

  char c = static_cast<char>(*got);
  auto cpSize = unicode::utf8::codePointSize(c);
  if (cpSize == 0)
    throw except::InputFailure(is_, pos, S << "Invalid UTF-8 byte: " << *got);

  vector<byte> ret;
  ret.reserve(cpSize);
  ret.push_back(*got);

  for (uint8_t i = 1; i < cpSize; ++i) {
    got = get();
    if (not got)
      throw except::InputFailure(is_, pos, "Incomplete UTF-8 byte sequence");
    if (not unicode::utf8::continuationByte(static_cast<char>(*got)))
      throw except::InputFailure(is_, pos, "Invalid UTF-8 byte sequence");
    ret.push_back(*got);
  }

  if (cp) {
    string_view s(reinterpret_cast<const char*>(ret.data()), ret.size());
    *cp = *unicode::CodePointIterator<char>(s);
  }

  return ret;
}

optional<vector<byte>>
Buffer::getGrapheme(unicode::Grapheme* gr) {
  // Read first code point
  unicode::CodePoint cp;
  auto bytes = getCodePoint(&cp);
  if (not bytes)
    return nullopt;

  // Update values
  u32string s { cp };
  vector<byte> ret;
  copy(bytes->begin(), bytes->end(), back_inserter(ret));

  // Read more code points

  while (true) {
    // Read next code point
    bytes = getCodePoint(&cp);

    // If EOF, finish
    if (not bytes) {
      if (gr)
        *gr = unicode::Grapheme(s);
      return ret;
    }

    // Update values I
    s.push_back(cp);

    // If grapheme boundary, put back and finish
    size_t pos = s.size() - 1;
    if (unicode::CodePointIterator<char32_t>(s, pos).graphemeBoundary()) {
      put(*bytes);
      if (gr)
        *gr = unicode::Grapheme(s.substr(0, pos));
      return ret;
    }

    // Update values II
    copy(bytes->begin(), bytes->end(), back_inserter(ret));
  }
}

void
Buffer::put(byte b) {
  intermediate_.push_back(b);
  ROCKET_EXPECT(pos_ > 0);
  --pos_;
}

void
Buffer::put(const vector<byte>& bytes) {
  for (auto it = bytes.rbegin(), end = bytes.rend(); it != end; ++it)
    put(*it);
}

// `Symbols` ------------------------------------------------------------------------------------------------

const set<char> Symbols<char>::Chars::HexDigits {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  'a', 'b', 'c', 'd', 'e', 'f',
  'A', 'B', 'C', 'D', 'E', 'F'
};

const set<char32_t> Symbols<char32_t>::Chars::HexDigits {
  U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7', U'8', U'9',
  U'a', U'b', U'c', U'd', U'e', U'f',
  U'A', U'B', U'C', U'D', U'E', U'F'
};

} // namespace rocket::io

// EOF
