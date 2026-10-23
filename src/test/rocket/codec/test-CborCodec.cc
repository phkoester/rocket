/*
 * test-CborCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/codec/CborCodec.h"
#include "rocket/log/log.h"

using namespace rocket::codec;
using namespace rocket::codec::internal::cbor;

namespace {

// Local variables ------------------------------------------------------------------------------------------

const CborCodec codec;

// Local functions ------------------------------------------------------------------------------------------

CborItem<pair<u32, u32>> makeCborItemArray();

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
  const vector<u8> cbor = codec.encode(val);

  const auto result = codec.decode<T>(cbor);
  if (not result.has_value()) {
    const auto& error = result.error();
    if constexpr (std::same_as<T, std::u32string> || std::same_as<T, std::u32string_view>) {
      std::string utf8(unicode::ConvertTo<char>::apply(val));
      ROCKET_FAIL("Failed to decode value \"{}\" at position {}: {}\n", utf8, error.position, error.message);
    } else if constexpr (std::same_as<T, char32>) {
      ROCKET_FAIL("Failed to decode value {} at position {}: {}\n", static_cast<u32>(val), error.position, error.message);
    } else {
      ROCKET_FAIL("Failed to decode value {} at position {}: {}\n", val, error.position, error.message);
    }
  }
  const T& decodedVal = result.value();

  if constexpr (IsFloat<T>) {
    if (std::isnan(val)) {
      EXPECT_TRUE(std::isnan(decodedVal));
    } else {
      EXPECT_EQ(decodedVal, val);
    }
  } else {
    EXPECT_EQ(decodedVal, val);
  }
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
  testRoundtrip<log::LogLevel>(static_cast<log::LogLevel>(-1)); // NOLINT
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

TEST(CborCodec, Float) {
  testRoundtrip(1.0_f32);
  testRoundtrip(numeric_limits<f32>::min());
  testRoundtrip(numeric_limits<f32>::max());
  testRoundtrip(numeric_limits<f32>::infinity());
  testRoundtrip(numeric_limits<f32>::quiet_NaN());
  testRoundtrip(numeric_limits<f32>::signaling_NaN());

  testRoundtrip(1.0_f64);
  testRoundtrip(numeric_limits<f64>::min());
  testRoundtrip(numeric_limits<f64>::max());
  testRoundtrip(numeric_limits<f64>::infinity());
  testRoundtrip(numeric_limits<f64>::quiet_NaN());
  testRoundtrip(numeric_limits<f64>::signaling_NaN());
}

TEST(CborCodec, Pointer) {
  testRoundtrip(reinterpret_cast<void*>(0x12345678));
  testRoundtrip(reinterpret_cast<void*>(0x1234567890abcdef));
}

TEST(CborCodec, String) {
  testRoundtrip("abc € 🧑‍🌾"s);
  testRoundtrip("abc € 🧑‍🌾"sv);

  testRoundtrip(U"abc € 🧑‍🌾"s);
  testRoundtrip(U"abc € 🧑‍🌾"sv);
}

TEST(CborCodec, Optional) {
  testRoundtrip(std::optional<u32> {});
  testRoundtrip(std::optional<u32>(42));

  testRoundtrip(std::optional<string> {});
  testRoundtrip(std::optional<string>("What is the answer?"));
}

TEST(CborCodec, Tuple) {
  testRoundtrip(make_tuple());
  testRoundtrip(make_tuple(1, 2, 3));
  testRoundtrip(make_tuple("answer"s, 42));
  testRoundtrip(make_tuple("answer"sv, 42, true));
}

// EOF
