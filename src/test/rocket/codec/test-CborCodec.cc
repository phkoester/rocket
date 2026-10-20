/*
 * test-CborCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/CborCodec.h"
#include "rocket/log/log.h"

using namespace rocket::codec;

namespace {

// Local functions ------------------------------------------------------------------------------------------

template<typename T>
void
testRoundtrip(const T& val) {
  CborCodec codec;
  vector<u8> cbor = codec.encode(val);
  const auto result = codec.decode<T>(cbor);
  if (not result.has_value()) {
    const auto& error = result.error();
    if constexpr (std::same_as<T, char32>) {
      ROCKET_FAIL("Failed to decode value {} at position {}: {}\n", static_cast<u32>(val), error.position, error.message);
    } else {
      ROCKET_FAIL("Failed to decode value {} at position {}: {}\n", val, error.position, error.message);
    }
  }
  const T& decodedVal = result.value();
  EXPECT_EQ(decodedVal, val);
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(CborCodec, ConsumerBool) {
  testRoundtrip(false);
  testRoundtrip(true);
}

TEST(CborCodec, ConsumerChar) {
  testRoundtrip('a');
  testRoundtrip('\x00');
  testRoundtrip('\x7f');
  testRoundtrip('\xff');

  testRoundtrip(U'a');
  testRoundtrip(U'€');
}

TEST(CborCodec, ConsumerEnum) {
  testRoundtrip(log::LogLevel::info);
  testRoundtrip<log::LogLevel>(static_cast<log::LogLevel>(-1));
}

TEST(CborCodec, ConsumerInteger) {
  testRoundtrip(100_i8);
  testRoundtrip(100_u8);

  testRoundtrip(1000_i16);
  testRoundtrip(1000_u16);

  testRoundtrip(100000_i32);
  testRoundtrip(100000_u32);


  testRoundtrip(1000000000_i64);
  testRoundtrip(1000000000_u64);
}

TEST(CborCodec, ConsumerInteger128) {
  testRoundtrip(0_i128);
}

// EOF
