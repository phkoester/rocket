/*
 * test-CborCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/CborCodec.h"
#include "rocket/log/log.h"

using namespace rocket::codec;
using namespace rocket::codec::internal::cbor;

namespace {

// Local functions ------------------------------------------------------------------------------------------

CborItem<pair<u32, u32>> makeCborItemArray();

void
assignToCborItemArray(CborItem<pair<u32, u32>>& out) {
  auto array = makeCborItemArray();
  out = std::move(array);
}

CborItem<pair<u32, u32>>
makeCborItemArray() {
  auto array = CborItem<pair<u32, u32>>(cbor_new_definite_array(2));
  auto item0 = CborItem<i32>(cbor_build_uint32(42));
  ROCKET_EXPECT(cbor_array_push(array.ptr(), item0.ptr()));
  auto item1 = CborItem<u32>(cbor_build_uint32(43));
  ROCKET_EXPECT(cbor_array_push(array.ptr(), item1.ptr()));

  EXPECT_EQ(cbor_refcount(array.ptr()), 1);
  EXPECT_EQ(cbor_refcount(item0.ptr()), 2);
  EXPECT_EQ(cbor_refcount(item1.ptr()), 2);

  return array;
}

CborItem<u8>
someFuncReturningCborItem() {
  return CborItem<u8>(cbor_build_uint8(42));
}

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

// `CborItem` ...............................................................................................

TEST(CborCodec, CborItemAsReturnValue) {
  auto item = someFuncReturningCborItem();
  EXPECT_EQ(cbor_refcount(item.ptr()), 1);
}

TEST(CborCodec, CborItemArrayAsReturnValue) {
  {
    auto array = makeCborItemArray();
    EXPECT_EQ(cbor_refcount(array.ptr()), 1);
    auto item0 = CborItem<u32>(cbor_array_get(array.ptr(), 0));
    EXPECT_EQ(cbor_refcount(item0.ptr()), 2);
    auto item1 = CborItem<u32>(cbor_array_get(array.ptr(), 1));
    EXPECT_EQ(cbor_refcount(item1.ptr()), 2);
  }

  nio::out.println("end of CborCodec.CborItemArray");
}

TEST(CborCodec, CborItemAssigningToArray) {
  CborItem<pair<u32, u32>> array;
  assignToCborItemArray(array);
}

// `CborCodec` ..............................................................................................

TEST(CborCodec, Bool) {
  testRoundtrip(false);
  testRoundtrip(true);
}

TEST(CborCodec, Char) {
  testRoundtrip('a');
  testRoundtrip('\x00');
  testRoundtrip('\x7f');
  testRoundtrip('\xff');

  testRoundtrip(U'a');
  testRoundtrip(U'€');
}

TEST(CborCodec, Enum) {
  testRoundtrip(log::LogLevel::info);
  testRoundtrip<log::LogLevel>(static_cast<log::LogLevel>(-1));
}

TEST(CborCodec, Integer) {
  testRoundtrip(100_i8);
  testRoundtrip(100_u8);

  testRoundtrip(1000_i16);
  testRoundtrip(1000_u16);

  testRoundtrip(100000_i32);
  testRoundtrip(100000_u32);


  testRoundtrip(1000000000_i64);
  testRoundtrip(1000000000_u64);
}

TEST(CborCodec, Integer128) {
  testRoundtrip(numeric_limits<i128>::min());
  testRoundtrip(numeric_limits<i128>::max());
  testRoundtrip(numeric_limits<u128>::max());
  testRoundtrip(340282366920938463463374607431768211455_u128);
}

// EOF
