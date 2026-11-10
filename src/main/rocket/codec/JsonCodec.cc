/*
 * JsonCodec.cc
 */

#include "rocket/codec/JsonCodec.h"

using namespace std;

namespace rocket::codec::internal::json {

// Functions ------------------------------------------------------------------------------------------------

u64
jsonStringOffset(u64 pos, u64 unescapedOffset, const Positions& positions) {
  // `positions` only has entries at grapheme-cluster boundaries. For an offset inside a cluster, use the
  // start of that cluster. Offset 0 always has an entry
  auto it = positions.right.find(unescapedOffset);
  while (it == positions.right.end() && unescapedOffset > 0) {
    it = positions.right.find(--unescapedOffset);
  }
  ROCKET_EXPECT(it != positions.right.end());

  // Need to add 1 because of the opening quote
  return pos + 1 + it->second;
}

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

  const string unescaped = unescapeLiteral(*input, pos + 1, { .json=true }, &positions);
  // Postconditions:
  // - `positions` maps offset in `input` (greater) -> offset in `unescaped` (smaller)
  // - Position 0 in `unescaped` is `pos + 1` in `in` (because of the opening quote)
  // - The current position of `in` is after the closing quote
  return unescaped;
}

} // namespace rocket::codec::internal::json

// EOF
