/*
 * test-CborCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/CborCodec.h"

using namespace rocket::codec;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(CborCodec, ConsumerBool) {
  bool val = true;
  nio::StringSink sink;
  CborCodec codec;
  codec.encode(val, sink);
  EXPECT_EQ(sink.str(), "t");
}

// EOF
