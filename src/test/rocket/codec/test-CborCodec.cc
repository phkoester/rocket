/*
 * test-CborCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/CborCodec.h"

using namespace rocket::codec;

namespace {

// Local functions ------------------------------------------------------------------------------------------

template<typename T>
void
testRoundtrip(const T& val) {
  CborCodec codec;
  vector<u8> cbor = codec.encode(val);
  const auto result = codec.decode<T>(cbor);
  ROCKET_EXPECT(result.has_value());
  const T& decodedVal = result.value();
  EXPECT_EQ(decodedVal, val);
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(CborCodec, ConsumerBool) {
  testRoundtrip(false);
  testRoundtrip(true);
}

// EOF
