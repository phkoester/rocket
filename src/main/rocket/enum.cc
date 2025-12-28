/*
 * enum.cc
 */

#include "enum.h"

using namespace std;

// Internal -------------------------------------------------------------------------------------------------

namespace rocket::_enum::internal {

string
getEnumString(istream& is, const set<string_view>& values) {
  string buf; // Chars read so far
  auto candidates = values; // Make a copy of the set
  string best; // Best candidate so far
  auto pos = io::tellg(is); // Save the position
  ROCKET_EXPECT(pos >= 0);

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
    cout << "Read char '" << c << "', buf = '" << buf << "'\n";
    for (auto it = candidates.begin(), end = candidates.end(); it != end;) {
      const string_view& candidate = *it;
      if (candidate.size() < index || candidate[index] != c) {
        // Candidate doesn't match: erase it
        string save(candidate);
        it = candidates.erase(it);
        cout << "Erased candidate '" << save << "'\n";
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
    if (candidates.size() == 1 && candidates.begin()->size() == buf.size()) {
      ROCKET_EXPECT(*candidates.begin() == buf);
      break;
    }
  }

  // Did we find a best candidate?

  if (not best.empty()) {
    // We have a best candidate: seek its end and return it
    is.seekg(pos + istream::pos_type(best.size()));
    return best;
  } else {
    // No best candidate: seek back to the start and throw an exception
    is.seekg(pos);
    throw io::InputFailure(is);
  }
}

} // namespace rocket::_enum::internal

// EOF
