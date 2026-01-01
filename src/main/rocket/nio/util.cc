/*
 * util.cc
 */

#include "util.h"

#include "rocket/InputFailure.h"
#include "rocket/assert.h"

#include <inttypes.h>

using namespace rocket;
using namespace rocket::nio;
using namespace std;

// Local functions ------------------------------------------------------------------------------------------

namespace {

void check(Source& in) {
  if (in.error()) {
    throw InputFailure(in.tell(), "Input error");
  }
}

} // namespace

namespace rocket::nio {

char
getChar(Source& in, char expected) {
  ROCKET_CHECK(expected, isascii(expected));

  char out;
  if (in.read(out) == 0) {
    throw InputFailure(in.tell(), fmt::format("Expected {:?}, got EOF", expected));
  }
  check(in);
  if (out != expected) {
    in.seek(-1, SeekMode::cur);
    throw InputFailure(in.tell(), fmt::format("Expected {:?}, got {:?}", expected, out));
  }
  return out;
}

char
getChar(Source& in, const char* expected, const char* what) {
  char out;
  if (in.read(out) == 0) {
    throw InputFailure(in.tell(), fmt::format("Expected {}, got EOF", what));
  }
  check(in);
  if (not std::strchr(expected, out)) {
    throw InputFailure(in.tell(), fmt::format("Expected {}, got {:?}", what, out));
  }
  return out;
}

unicode::Grapheme
getGrapheme(nio::Source& in) {
  unicode::Grapheme out;
  if (read(in, out) == 0) {
    throw InputFailure(in.tell(), "Expected a UTF-8 grapheme");
  }
  check(in);
  return out;
}

uint32_t
getHex(Source& in, size_t n) {
  ROCKET_CHECK(n, n > 0 && n <= 8);

  auto pos = in.tell();
  try {
    string input;
    for (size_t i = 0; i < n; ++i) {
      char c = getChar(in, "0123456789abcdefABCDEF", "a hexadecimal digit");
      input.push_back(c);
    }

    uint32_t ret;
    std::sscanf(input.c_str(), "%" SCNd32 "x", &ret);
    return ret;
  } catch (const exception&) {
    in.seek(pos);
    throw;
  }
}

optional<char>
getOptionalChar(nio::Source& in) {
  char out;
  if (in.read(out) == 0) {
    return nullopt;
  }
  check(in);
  return out;
}

optional<unicode::Grapheme>
getOptionalGrapheme(nio::Source& in) {
  unicode::Grapheme out;
  if (read(in, out) == 0) {
    return nullopt;
  }
  check(in);
  return out;
}

} // namespace rocket::nio

// EOF
