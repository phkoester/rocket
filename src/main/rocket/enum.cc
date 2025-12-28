/*
 * enum.cc
 */

#include "enum.h"

using namespace std;

// Internal -------------------------------------------------------------------------------------------------

namespace rocket::_enum::internal {

// XXX -> io.h, wenn io-decl.h weg ist
inline void seek(istream& is, size_t position) {
  // Only seekg if the position is different; this retains the EOF bit
  if (io::tellg(is) != position) {
    io::seekg(is, position);
  }
}

string
getEnumString(istream& is, const set<string_view>& values) {
  string buf; // Chars read so far
  auto candidates = values; // Make a copy of the set
  string best; // Best candidate so far
  auto pos = io::tellg(is); // Save the position

  while (true) {
    // Read one char

    char c;
    is >> c;
    if (is.eof()) {
      // EOF: clear fail bit, exit loop
      is.clear(is.rdstate() & ~ios::failbit);
      break;
    }
    if (is.fail()) {
      throw io::InputFailure(is);
    }

    // Eliminate candidates that don't match the current char

    auto index = buf.size();
    buf.push_back(c);
    for (auto it = candidates.begin(), end = candidates.end(); it != end;) {
      const string_view& candidate = *it;
      if (candidate.size() < index || candidate[index] != c) {
        // Candidate doesn't match: erase it
        string save(candidate);
        it = candidates.erase(it);
      } else {
        // Candidate matches: check if it's the best candidate so far
        if (candidate == buf) {
          // It is: update the best candidate
          best = candidate;
        }
        ++it;
      }
    }

    // Are there any candidates left?

    if (candidates.empty()) {
      break;
    }
  }

  // Did we find a best candidate?

  if (not best.empty()) {
    // We have a best candidate: seek its end and return it
    seek(is, pos + istream::pos_type(best.size()));
    return best;
  } else {
    // No best candidate: seek back to the start and throw an exception
    seek(is, pos);
    throw io::InputFailure(is);
  }
}

} // namespace rocket::_enum::internal

// EOF
