/*
 * test-CborCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Bimap-codec.h"
#include "rocket/chrono/chrono.h"
#include "rocket/codec/CborCodec.h"
#include "rocket/log/log.h"
#include "rocket/math/Interval-codec.h"
#include "rocket/reflect/reflect.h"

#include <fmt/chrono.h>

using namespace rocket::codec;
using namespace rocket::codec::internal::cbor;

// `MyStruct` -----------------------------------------------------------------------------------------------

struct MyStruct {
  i32 ärger = 0;
  bool ökonom = false;
  string übermut;
  vector<i32> vec {}; // NOLINT

  ROCKET_REFLECT_MEMBERS(MyStruct, Index, (ärger)(ökonom)(übermut)(vec));

  ROCKET_REFLECT_MEMBERS(MyStruct, Three, (ärger)(ökonom)(übermut));
};

ROCKET_REFLECT_MEMBERS_DECLARE(, MyStruct, Index); // NOLINT(*-internal-linkage)
ROCKET_REFLECT_MEMBERS_DEFINE(, MyStruct, Index);

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
      ROCKET_FAIL("Cannot decode value \"{}\" at position {}: {}\n", utf8, error.position, error.message);
    } else if constexpr (std::same_as<T, char32>) {
      ROCKET_FAIL("Cannot decode value {} at position {}: {}\n", static_cast<u32>(val), error.position, error.message);
    } else {
      ROCKET_FAIL("Cannot decode value {} at position {}: {}\n", val, error.position, error.message);
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

template<typename T>
void
testHourMinuteSecond(const T& val) {
  const auto result = codec.decode<T>(codec.encode(val));
  if (not result.has_value()) {
    ROCKET_FAIL(
      "Cannot decode hour, minute, and second {} at position {}: {}\n",
      val.to_duration().count(),
      result.error().position,
      result.error().message);
  }
  EXPECT_EQ(result.value().to_duration(), val.to_duration());
}

template<typename T>
void
testZonedTime(const T& val) {
  const auto result = codec.decode<T>(codec.encode(val));
  if (not result.has_value()) {
    ROCKET_FAIL("Cannot decode zoned time at position {}: {}\n",
      result.error().position, result.error().message);
  }
  EXPECT_EQ(result.value(), val);
}

template<typename T>
void
testCharacter(const T& val) {
  const auto result = codec.decode<T>(codec.encode(val));
  if (not result.has_value()) {
    ROCKET_FAIL("Cannot decode character at position {}: {}\n",
      result.error().position, result.error().message);
  }
  EXPECT_EQ(static_cast<typename T::View>(result.value()), static_cast<typename T::View>(val));
}

void
testVarRefs() {
  int a = 0;
  int b = 1;
  int c = 2;
  const auto refs = ROCKET_REFLECT_VARS((a)(b)(c));

  const auto decoded = codec.decode<tuple<int, int, int>>(codec.encode(refs));
  if (not decoded.has_value()) {
    ROCKET_FAIL("Cannot decode variable references at position {}: {}\n",
      decoded.error().position, decoded.error().message);
  }
  EXPECT_EQ(decoded.value(), (tuple<int, int, int> { 0, 1, 2 }));

  const auto one = codec.decode<int>(codec.encode(std::get<1>(refs)));
  if (not one.has_value()) {
    ROCKET_FAIL("Cannot decode variable reference at position {}: {}\n",
      one.error().position, one.error().message);
  }
  EXPECT_EQ(one.value(), 1);
}

void
testTimeZone(TimeZone zone) {
  const auto result = codec.decode<TimeZone>(codec.encode(zone));
  if (not result.has_value()) {
    ROCKET_FAIL("Cannot decode time zone \"{}\" at position {}: {}\n",
      zone->name(), result.error().position, result.error().message);
  }
  EXPECT_EQ(result.value(), zone);
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
  testRoundtrip(make_pair(42, "answer"));

  testRoundtrip(make_tuple());
  testRoundtrip(make_tuple(1, 2, 3));
  testRoundtrip(make_tuple("answer"s, 42));
  testRoundtrip(make_tuple("answer"sv, 42, true));
}

TEST(CborCodec, List) {
  testRoundtrip((array<i32, 0> {}));
  testRoundtrip((array<i32, 3> { 1, 2, 3 }));

  testRoundtrip(list<i32> { 1, 2, 3 });

  testRoundtrip(vector<i32> {});
  testRoundtrip(vector<i32> { 1, 2, 3 });
  testRoundtrip(vector<vector<i32>> { { 1, 2, 3 }, { 4, 5, 6 } });
  testRoundtrip(vector<string> { "alpha"s, "beta"s });
}

TEST(CborCodec, Set) {
  testRoundtrip(set<i32> {});
  testRoundtrip(set<i32> { 1, 2, 2, 3 });
  testRoundtrip(set<string> { "alpha"s, "beta"s });

  testRoundtrip(unordered_set<i32> {});
  testRoundtrip(unordered_set<i32> { 3, 2, 1 });
}

TEST(CborCodec, Map) {
  testRoundtrip(map<i32, i32> {});
  testRoundtrip(map<i32, i32> { { 1, 2 }, { 2, 3 }, { 3, 4 } });
  testRoundtrip(map<string, i32> { { "alpha"s, 1 }, { "beta"s, 2 }, { "gamma"s, 3 } });

  testRoundtrip(unordered_map<i32, string> {});
  testRoundtrip(unordered_map<string, i32> { { "alpha"s, 1 }, { "beta"s, 2 } });
}

TEST(CborCodec, Bimap) {
  testRoundtrip(Bimap<i32, i32> {});
  testRoundtrip(makeBimap<i32, string>({ { 1, "alpha" }, { 2, "beta" }, { 3, "gamma" } }));
  testRoundtrip(makeBimap<string, i32>({ { "alpha", 1 }, { "beta", 2 } }));

  testRoundtrip(UnorderedBimap<i32, i32> {});
  testRoundtrip(makeUnorderedBimap<string, i32>({ { "alpha", 1 }, { "beta", 2 } }));
}

TEST(CborCodec, Duration) {
  using namespace std::chrono;

  testRoundtrip(1ns);
  testRoundtrip(2us);
  testRoundtrip(3ms);
  testRoundtrip(4s);
  testRoundtrip(5min);
  testRoundtrip(-6h);
  testRoundtrip(days(7));
  testRoundtrip(weeks(8));
  testRoundtrip(months(9));
  testRoundtrip(years(10));
}

TEST(CborCodec, Date) {
  using namespace std::chrono;

  testRoundtrip(year_month_day { 1970y, January, 1d });
  testRoundtrip(year_month_day { 1970y, January, 2d });
  testRoundtrip(year_month_day { -100y, January, 2d });
  testRoundtrip(year_month_day { 2026y, October, 23d });
}

TEST(CborCodec, HourMinuteSecond) {
  using namespace std::chrono;

  testHourMinuteSecond(hh_mm_ss { 1h + 2min + 3s });
  testHourMinuteSecond(hh_mm_ss { -(111h + 2min + 3s + 123456us) });
  testHourMinuteSecond(hh_mm_ss<milliseconds> { 1h + 2min + 3s + 123ms });
  testHourMinuteSecond(hh_mm_ss<nanoseconds> { -(111h + 2min + 3s + 123456789ns) });
}

TEST(CborCodec, TimeZone) {
  using namespace std::chrono;

  testTimeZone(locate_zone("Europe/Berlin"));
  testTimeZone(locate_zone("UTC"));
  testTimeZone(locate_zone("Etc/UTC"));
}

TEST(CborCodec, Time) {
  using namespace std::chrono;

  testRoundtrip(sys_seconds { 0s });
  testRoundtrip(sys_seconds { -1000s });
  testRoundtrip(time_point_cast<nanoseconds>(rocket::chrono::now<system_clock>()));
}

TEST(CborCodec, ZonedTime) {
  using namespace std::chrono;

  const auto now = time_point_cast<seconds>(rocket::chrono::now<system_clock>());
  testZonedTime(zoned_time(locate_zone("UTC"), now));
  testZonedTime(zoned_time(locate_zone("Europe/Berlin"), now));
  testZonedTime(zoned_time<nanoseconds>(current_zone(), rocket::chrono::now<system_clock>()));
}

TEST(CborCodec, Interval) {
  using namespace rocket::math;

  testRoundtrip(ClosedInterval<f64>(-4.2_f64, 4.2_f64));
  testRoundtrip(OpenInterval<f64>());
  testRoundtrip(OpenInterval<i64>(nullopt, nullopt));
  testRoundtrip(RightOpenInterval<f64>(-4.2_f64, nullopt));
  testRoundtrip(LeftOpenInterval<i32>(1, 10));
}

TEST(CborCodec, Declared) {
  testRoundtrip(MyStruct { 42, true, "hello", { 1, 2, 3 } });
  testRoundtrip(MyStruct {});
}

TEST(CborCodec, Instance) {
  using type = reflect::Instance<MyStruct, MyStruct::Three>;
  testRoundtrip(type(MyStruct { 42, true, "hello", { 1, 2, 3 } }));
}

TEST(CborCodec, VarRef) {
  testVarRefs();
}

TEST(CborCodec, CodePoint) {
  using type = unicode::CodePoint;
  testRoundtrip(type('a'));
  testRoundtrip(type(U'€'));
  testRoundtrip(type(U'\U0010FFFF'));
}

TEST(CborCodec, Character) {
  testCharacter(unicode::Character<char>("hello"s));
  testCharacter(unicode::Character<char32>(U"€"s));
  testCharacter(unicode::CharacterView<char>("hello"sv));
}

// EOF
