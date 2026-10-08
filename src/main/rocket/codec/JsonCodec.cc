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
  // Post conditions:
  // - `positions` maps offset in `input` (greater) -> offset in `unescaped` (smaller)
  // - Position 0 in `unescaped` is `pos + 1` in `in` (because of the opening quote)
  // - The current position of `in` is after the closing quote
  return unescaped;
}

}; // namespace rocket::codec::internal::json

// EOF
