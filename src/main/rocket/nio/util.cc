/*
 * util.cc
 */

#include "util.h"

#include "rocket/InputFailure.h"
#include "rocket/assert.h"

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

size_t
getChar(nio::Source& in, char& out) {
  size_t ret = in.read(out);
  check(in);
  return ret;
}

size_t
getChar(Source& in, char& out, char expected) {
  ROCKET_CHECK(expected, isascii(expected));

  auto inputPos = in.tell();

  size_t ret;
  if ((ret = in.read(out)) == 0) {
    throw InputFailure(inputPos, fmt::format("Expected {:?} got EOF", expected));
  }
  check(in);
  if (out != expected) {
    throw InputFailure(inputPos, fmt::format("Expected {:?} got {:?}", expected, out));
  }
  return ret;
}

} // namespace rocket::nio

// EOF
