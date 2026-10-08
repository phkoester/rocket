/*
 * JsonCodec.cc
 */

#include "rocket/codec/JsonCodec.h"

using namespace std;

namespace rocket::codec::internal::json {

// Functions ------------------------------------------------------------------------------------------------

string
readJsonString(nio::Source& in, Positions& positions) {
  const auto pos = in.tell();

  if (not readChar(in, '"')) {
    throw InputFailure(pos, "Expected a string");
  }
  const auto input = readUntilUnescapedChar(in, '"');
  if (not input) {
    throw InputFailure(pos, "Unterminated string literal");
  }

  const string unescaped = str::escape::unescapeCString(*input, {}, &positions);
  // XXX positions: offset in input (größer) -> offset in unescaped (kleiner)
  // XXX Außerdem ist Return-Wert um eins verschoben: 0 in ret ist pos + 1 in In-Source
  // XXX in.tell() ist außerdem um 1 weiter, denn schließendes '"' wurde gelesen
  return unescaped;
}

}; // namespace rocket::codec::internal::json

// EOF
