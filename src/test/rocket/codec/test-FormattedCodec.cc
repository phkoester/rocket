/*
 * test-FormattedCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Bimap-codec.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/chrono/chrono.h"
#include "rocket/log/log.h"
#include "rocket/reflect/reflect.h"
#include "rocket/str/location/location.h"

#include <fmt/chrono.h>

using namespace rocket;
using namespace rocket::codec;
using namespace std;

// #MyStruct ------------------------------------------------------------------------------------------------

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

// Local functions ------------------------------------------------------------------------------------------

template<typename T>
string
encode(const T& val, const FormattedConsumerConfig& config = {}) {
  const FormattedCodec codec;
  nio::StringSink out;
  codec.encode(val, out, config);
  return out.str();
}

template<typename T>
T
decode(string_view str) {
  const FormattedCodec codec;
  nio::StringSource in(str);
  return codec.decode<T>(in, { .cComments=true, .shellComments=true });
}

template<typename T>
pair<T, u64>
decodeAndTell(string_view str) {
  const FormattedCodec codec;
  nio::StringSource in(str);
  return { codec.decode<T>(in, { .cComments=true, .shellComments=true }), in.tell() };
}

} // namespace

// #TEST ----------------------------------------------------------------------------------------------------

// #FormattedConsumer .......................................................................................

TEST(FormattedCodec, FormattedConsumerBool) {
  EXPECT_EQ(encode(true), "true");
}

TEST(FormattedCodec, FormattedConsumerChar) {
  EXPECT_EQ(encode('\t'), "'\\t'");
  EXPECT_EQ(encode(U'€'), "'€'");
}

TEST(FormattedCodec, FormattedConsumerEnum) {
  enum Color : u8 { Red, Green, Blue };
  EXPECT_THAT(
    [] { encode(Blue); },
    ThrowsMessage<InvalidState>(containsRegex("Cannot format enum of type `.*Color`")));

  EXPECT_EQ(encode(log::LogLevel::info), "info");
  const log::LogLevel bogus = static_cast<log::LogLevel>(-1);
  EXPECT_THAT(
    [&] { encode(bogus); },
    ThrowsMessage<InvalidState>(HasSubstr("Invalid `rocket::log::LogLevel` value 255")));
}

TEST(FormattedCodec, FormattedConsumerIntegerI64) {
  EXPECT_EQ(encode(-42_i64), "-42");
}

TEST(FormattedCodec, FormattedConsumerFloatF64) {
  using type = f64;
  using limits = numeric_limits<type>;

  EXPECT_EQ(encode(-123.456_f64), "-123.456");
  EXPECT_EQ(encode(-limits::infinity()), "-∞");
  EXPECT_EQ(encode(limits::infinity()), "∞");
}

TEST(FormattedCodec, FormattedConsumerPointer) {
  EXPECT_EQ(encode(reinterpret_cast<void*>(0)), "null");
  EXPECT_THAT(encode(reinterpret_cast<void*>(0x12345678)), matchesRegex("0x[0-9a-f]+"));
}

TEST(FormattedCodec, FormattedConsumerString) {
  EXPECT_EQ(encode("Hello"sv), "\"Hello\"");
  EXPECT_EQ(encode(U"Hello"sv), "\"Hello\"");
  EXPECT_EQ(encode("\x7f"sv), "\"\\x7F\"");
}

TEST(FormattedCodec, FormattedConsumerOptional) {
  using type = optional<string>;

  const type val;
  EXPECT_EQ(encode(val), "null");
  EXPECT_EQ(encode<type>("Hello"), "\"Hello\"");
}

TEST(FormattedCodec, FormattedConsumerTuplePair) {
  EXPECT_EQ(encode(make_pair("answer"sv, 42)), "(\"answer\", 42)");
  EXPECT_EQ(encode(make_pair("answer"sv, 42), { .indent=true }),
    "(\n"
    "  \"answer\",\n"
    "  42\n"
    ")");
}

TEST(FormattedCodec, FormattedConsumerList) {
  EXPECT_EQ(encode(forward_list<i32> { 1, 2, 3 }), "[1, 2, 3]");

  const vector<i32> valVectorI32 { 1, 2, 3 };
  EXPECT_EQ(encode(span<const i32>(valVectorI32)), "[1, 2, 3]");

  EXPECT_EQ(encode(vector<vector<i32>> { { 1, 2, 3 }, { 4, 5, 6 } }), "[[1, 2, 3], [4, 5, 6]]");

  EXPECT_EQ(encode(vector<vector<i32>> { { 1, 2, 3 }, { 4, 5, 6 } }, { .indent=true }),
    "[\n"
    "  [\n"
    "    1,\n"
    "    2,\n"
    "    3\n"
    "  ],\n"
    "  [\n"
    "    4,\n"
    "    5,\n"
    "    6\n"
    "  ]\n"
    "]");
}

TEST(FormattedCodec, FormattedConsumerSet) {
  EXPECT_EQ(encode(set<i32> {}), "{}");
  EXPECT_EQ(encode(set<i32> { 1, 2, 3 }), "{1, 2, 3}");
}

TEST(FormattedCodec, FormattedConsumerMap) {
  EXPECT_EQ(
    encode(map<string, i32> { { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } }),
    "{\"alpha\": 1, \"beta\": 2, \"gamma\": 3}");
}

TEST(FormattedCodec, FormattedConsumerDuration) {
  using namespace std::chrono;

  EXPECT_EQ(encode(1ns), "1ns");
  EXPECT_EQ(encode(2us), "2µs");
  EXPECT_EQ(encode(3ms), "3ms");
  EXPECT_EQ(encode(4s), "4s");
  EXPECT_EQ(encode(5min), "5min");
  EXPECT_EQ(encode(6h), "6h");
  EXPECT_EQ(encode(days(7)), "7d");
  EXPECT_EQ(encode(weeks(8)), "8w");
  EXPECT_EQ(encode(months(9)), "9m");
  EXPECT_EQ(encode(years(10)), "10y");
}

TEST(FormattedCodec, FormattedConsumerYearMonthDay) {
  using namespace std::chrono;

  EXPECT_EQ(encode(year_month_day { 1970y, January, 2d }), "1970-01-02");
}

TEST(FormattedCodec, FormattedConsumerHourMinuteSecond) {
  using namespace std::chrono;

  EXPECT_EQ(encode(hh_mm_ss { 1h + 2min + 3s }), "01:02:03");
  EXPECT_EQ(encode(hh_mm_ss { -(111h + 2min + 3s + 123456us) }), "-111:02:03.123456");
}

TEST(FormattedCodec, FormattedConsumerTimePoint) {
  using namespace std::chrono;

  const auto* const regexS = R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)"; // Seconds
  const auto* const regexNs = R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6,9}Z)"; // Nanoseconds

  const auto now = rocket::chrono::now<system_clock>();
  EXPECT_THAT(encode(time_point_cast<seconds>(now)), matchesRegex(regexS));
  EXPECT_THAT(encode(now), matchesRegex(regexNs));
}

TEST(FormattedCodec, FormattedConsumerZonedTime) {
  using namespace std::chrono;

  const auto* const regexS =
    R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(Z|[+-]\d{2}:\d{2}) \([^ ]+\))";
  const auto* const regexNs =
    R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6,9}(Z|[+-]\d{2}:\d{2}) \([^ ]+\))";

  {
    // Current time zone
    const auto* current = current_zone();
    const auto now = rocket::chrono::now<system_clock>();
    EXPECT_THAT(encode(zoned_time(current, time_point_cast<seconds>(now))), matchesRegex(regexS));
    EXPECT_THAT(encode(zoned_time(current, now)), matchesRegex(regexNs));
  }

  {
    // Time zone UTC
    const auto* utc = locate_zone("UTC");
    const auto now = rocket::chrono::now<system_clock>();
    EXPECT_THAT(encode(zoned_time(utc, time_point_cast<seconds>(now))), matchesRegex(regexS));
    EXPECT_THAT(encode(zoned_time(utc, now)), matchesRegex(regexNs));
  }
}

TEST(FormattedCodec, FormattedConsumerInterval) {
  EXPECT_EQ(encode(math::ClosedInterval<f32>(-4.2F, 4.2F)), "[-4.2,4.2]");
  EXPECT_EQ(encode(math::OpenInterval<f32>()), "∅");
  EXPECT_EQ(encode(math::OpenInterval<f32>(nullopt, nullopt)), "(-∞,∞)");
}

TEST(FormattedCodec, FormattedConsumerDeclared) {
  EXPECT_EQ(
    encode(MyStruct { 42, true, "hello", { 1, 2, 3 } }),
    "(ärger=42, ökonom=true, übermut=\"hello\", vec=[1, 2, 3])");
}

TEST(FormattedCodec, FormattedConsumerCodePoint) {
  using type = unicode::CodePoint;

  EXPECT_EQ(encode(type('a')), "U+0061");
  EXPECT_EQ(encode(type(U'€')), "U+20AC");
  EXPECT_EQ(encode(type(U'\U00010FFF')), "U+10FFF");
}

// #FormattedProducer .......................................................................................

TEST(FormattedCodec, FormattedProducerBool) {
  EXPECT_EQ(decode<bool>("// sup\nTRue"), true);
  EXPECT_EQ(decode<bool>("  /* comment\nanother line in the comment */\r\n# comment\n\ttRUe"), true);
  EXPECT_EQ(decode<bool>("\r\n  1"), true);

  EXPECT_THAT(
    [] { decode<bool>("\r\nx"); },
    throwsInputFailure(2, HasSubstr("Expected a boolean value")));
}

TEST(FormattedCodec, FormattedProducerChar) {
  EXPECT_EQ(decode<char>("'a'"), 'a');
  EXPECT_EQ(decode<char>("'\\''"), '\'');
  EXPECT_EQ(decode<char>("'\\t'"), '\t');
  EXPECT_THAT(
    [] { decode<char>("  'ä'"); },
    throwsInputFailure(2, HasSubstr("Invalid character literal")));

  EXPECT_EQ(decode<char32>("'ä'"), U'ä');
  EXPECT_EQ(decode<char32>("'\u20ac'"), U'€');
}

TEST(FormattedCodec, FormattedProducerEnum) {
  enum Color : u8 { Red, Green, Blue };
  EXPECT_THAT(
    [] { decode<Color>("2"); },
    throwsInputFailure(0, containsRegex("Cannot scan enum of type `.*Color`")));

  EXPECT_EQ(decode<log::LogLevel>("  info  "), log::LogLevel::info);
  EXPECT_THAT(
    [] { decode<log::LogLevel>("bogus"); },
    throwsInputFailure(0, HasSubstr("Invalid value for enum `rocket::log::LogLevel`")));
}

TEST(FormattedCodec, FormattedProducerInteger) {
  EXPECT_EQ(decode<i32>("-42"), -42);
  EXPECT_EQ(decode<i32>("0xABcd"), 0xABCD);

  EXPECT_THAT(
    [] { decode<i32>("  x"); },
    throwsInputFailure(2, HasSubstr("Expected an integer value")));
}

TEST(FormattedCodec, FormattedProducerFloat) {
  using type = f64;
  using limits = numeric_limits<type>;

  EXPECT_EQ(decodeAndTell<type>("  -123.456  "), make_pair(-123.456_f64, 10_u64));
  EXPECT_EQ(decode<type>("-inf"), -limits::infinity());
  EXPECT_EQ(decode<type>("-∞"), -limits::infinity());
  EXPECT_EQ(decode<type>("-inf"), -limits::infinity());
  EXPECT_EQ(decode<type>("∞"), limits::infinity());

  const type val = decode<type>("nan");
  EXPECT_TRUE(isnan(val));
}

TEST(FormattedCodec, FormattedProducerPointer) {
  EXPECT_EQ(decodeAndTell<void*>("  null  "), make_pair(static_cast<void*>(0), 6_u64)); // NOLINT
  EXPECT_EQ(decodeAndTell<void*>("  0x12345678  "), make_pair(reinterpret_cast<void*>(0x12345678), 12_u64));
}

TEST(FormattedCodec, FormattedProducerOptionalString) {
  using type = optional<string>;
  EXPECT_EQ(decode<type>("  null  "), nullopt);
}

TEST(FormattedCodec, FormattedProducerOptionalStringView) {
  const FormattedCodec codec;
  nio::StringSource in("\"Hello\""); // The source must remain valid for the string view
  EXPECT_EQ(codec.decode<optional<basic_string_view<char32>>>(in), U"Hello"sv);
}

TEST(FormattedCodec, FormattedProducerTuple) {
  EXPECT_EQ((decode<pair<string, i32>>("  (  \"answer\"   , 42   )")), make_pair("answer"s, 42_i32));
  EXPECT_EQ(
    (decode<pair<string, i32>>("  (  # comment\n \"answer\"   , 42  /* comment */  , // comment\n  )")),
    make_pair("answer"s, 42_i32));

  EXPECT_EQ((decode<tuple<i32, i32, bool>>("(1, 2, true)")), make_tuple(1_i32, 2_i32, true));
}

TEST(FormattedCodec, FormattedProducerList) {
  EXPECT_EQ((decode<array<i32, 3>>("  [ 1, 2, 3   ]")), (array<i32, 3> { 1, 2, 3 }));
  EXPECT_EQ((decode<array<i32, 3>>("  [ 1, 2, 3   , ]")), (array<i32, 3> { 1, 2, 3 }));
  EXPECT_EQ((decode<list<i32>>("  [ 1, 2, 3   , ]")), (list<i32> { 1, 2, 3 }));
  EXPECT_EQ((decode<vector<i32>>("[]")), (vector<i32> {}));
  EXPECT_EQ((decode<vector<i32>>("[1, 2, 3]")), (vector<i32> { 1, 2, 3 }));
  EXPECT_EQ((decode<vector<i32>>("[1, 2, 3,]")), (vector<i32> { 1, 2, 3 }));
  EXPECT_EQ((decode<vector<i32>>("  [ 1, 2, 3   , ]")), (vector<i32> { 1, 2, 3 }));
}

TEST(FormattedCodec, FormattedProducerSet) {
  EXPECT_EQ((decode<set<i32>>("  { 3, 2, 1  ,  }   ")), (set<i32> { 1, 2, 3 }));
}

TEST(FormattedCodec, FormattedProducerBimap) {
  using type = Bimap<string, i32>;
  EXPECT_EQ(
    (decode<type>("  { \"alpha\"\t: 1, \"beta\"  :/* comment */ 2, \"gamma\": 3  ,  }   ")),
    (makeBimap<string, i32>({ { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } })));
}

TEST(FormattedCodec, FormattedProducerBimapUnordered) {
  using type = UnorderedBimap<string, i32>;
  EXPECT_EQ(
    (decode<type>("  { \"alpha\"\t: 1, \"beta\"  :/* comment */ 2, \"gamma\": 3  ,  }   ")),
    (makeUnorderedBimap<string, i32>({ { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } })));
}

TEST(FormattedCodec, FormattedProducerDuration) {
  using namespace std::chrono;

  EXPECT_EQ(decode<milliseconds>("1s"), 1000ms);
  EXPECT_EQ(decode<milliseconds>("1s"), 1s);
  EXPECT_EQ(decode<milliseconds>("-5s"), -5s);

  EXPECT_EQ(decode<seconds>("1000ms"), 1s);

  EXPECT_THAT(
    [] { decode<seconds>("x"); },
    throwsInputFailure(0, HasSubstr("Expected a duration")));
  EXPECT_THAT(
    [] { decode<seconds>("10x"); },
    throwsInputFailure(2, HasSubstr("Expected a time unit")));
}

TEST(FormattedCodec, FormattedProducerYearMonthDay) {
  using namespace std::chrono;

  EXPECT_EQ(decode<year_month_day>("1970-01-02"), (year_month_day { 1970y, January, 2d }));
  EXPECT_EQ(decode<year_month_day>("-100-01-02"), (year_month_day { -100y, January, 2d }));

  EXPECT_THAT(
    [] { decode<year_month_day>("x"); },
    throwsInputFailure(0, HasSubstr("Expected a year, month, and day")));
}

TEST(FormattedCodec, FormattedProducerHourMinuteSecond) {
  using namespace std::chrono;

  EXPECT_EQ(
    decode<hh_mm_ss<seconds>>("01:02:03").to_duration(),
    (hh_mm_ss { 1h + 2min + 3s }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<seconds>>("01:02:03.123").to_duration(),
    (hh_mm_ss { 1h + 2min + 3s }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<milliseconds>>("01:02:03.123456").to_duration(),
    (hh_mm_ss { 1h + 2min + 3s + 123ms }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<microseconds>>("-111:02:03.123456789").to_duration(),
    (hh_mm_ss { -(111h + 2min + 3s + 123456us) }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<nanoseconds>>("-111:02:03.123456789").to_duration(),
    (hh_mm_ss { -(111h + 2min + 3s + 123456789ns) }).to_duration());

  EXPECT_THAT(
    [] { decode<hh_mm_ss<milliseconds>>("x"); },
    throwsInputFailure(0, HasSubstr("Expected an hour, minute, and second")));
  EXPECT_THAT(
    [] { decode<hh_mm_ss<milliseconds>>("01:02:"); },
    throwsInputFailure(0, HasSubstr("Expected an hour, minute, and second")));
  EXPECT_THAT(
    [] { decode<hh_mm_ss<milliseconds>>("01:02:03."); },
    throwsInputFailure(9, HasSubstr("Expected subseconds")));
}

TEST(FormattedCodec, FormattedProducerTimePoint) {
  using namespace std::chrono;

  using TimePoint = time_point<system_clock, nanoseconds>;

  // Test equality on roundtrip
  const TimePoint val1 = rocket::chrono::now<system_clock>();
  string encoded = encode(val1);
  nio::out.println("VAL1: {}", encoded);
  const TimePoint val2 = decode<TimePoint>(encoded); // NOLINT
  nio::out.println("VAL2: {}", encode(val2));
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(val2.time_since_epoch().count(), val1.time_since_epoch().count());
}

TEST(FormattedCodec, FormattedProducerZonedTime) {
  using namespace std::chrono;

  using ZonedTime = zoned_time<nanoseconds>;

  // Test equality on roundtrip
  const auto* current = current_zone();
  const auto now = rocket::chrono::now<system_clock>();
  const ZonedTime val1 = zoned_time(current, now);
  string encoded = encode(val1);
  nio::out.println("VAL1: {}", encoded);
  const ZonedTime val2 = decode<ZonedTime>(encoded); // NOLINT
  nio::out.println("VAL2: {}", encode(val2));
  EXPECT_EQ(val2, val1);
}

TEST(FormattedCodec, FormattedProducerInterval) {
  using namespace rocket::math;
  EXPECT_EQ(decode<ClosedInterval<f64>>("[-4.2,4.2]"), ClosedInterval<f64>(-4.2_f64, 4.2_f64));
  EXPECT_EQ(decode<OpenInterval<f64>>("∅"), OpenInterval<f64>());
  EXPECT_EQ(
    decode<OpenInterval<i64>>("  (  -∞ /* Comment */, ∞)  // Comment"),
    OpenInterval<i64>(nullopt, nullopt));
}

TEST(FormattedCodec, FormattedProducerDeclared) {
  EXPECT_EQ(
    (decode<MyStruct>("  ( ärger  =  42, ökonom=true, übermut=\"hello\", vec=[1, 2, 3] )   ")),
    (MyStruct { 42, true, "hello", { 1, 2, 3 } }));
}

TEST(FormattedCodec, FormattedProducerDeclaredFileSource) {
  const auto path = testSource("test-FormattedCodec-MyStruct.txt");

  string input;
  {
    FILE* file = fopen(path.string().c_str(), "rb");
    nio::FileSource in(file);
    input = in.readString();
  }

  FILE* file = fopen(path.string().c_str(), "rb");
  nio::FileSource in(file);

  MyStruct val;
  try {
    const FormattedCodec codec;
    val = codec.decode<MyStruct>(in, { .cComments=true, .shellComments=true });
  } catch (const InputFailure& ex) {
    namespace loc = rocket::str::location;
    const loc::Position pos {
      .type=loc::error, .position=ex.position(), .ranges=ex.ranges(), .message=ex.message()
    };
    const auto result = loc::locations(input, { pos }, { .setLineString=true, .source=path.string() });
    loc::printLocations(nio::out, input, result, { .styled=true });
    throw;
  }

  EXPECT_EQ(val.ärger, 16);
  EXPECT_EQ(val.ökonom, true);
  EXPECT_EQ(val.übermut, "a test\nstring");
  EXPECT_EQ(val.vec, (vector<i32> { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }));
}

TEST(FormattedCodec, FormattedProducerInstance) {
  using type = reflect::Instance<MyStruct, MyStruct::Three>;
  const MyStruct val { 42, true, "hello" };
  EXPECT_EQ(
    (decode<type>("  ( ärger  =  42, ökonom=true, übermut=\"hello\",  )   ")),
    (type(val)));
}

TEST(FormattedCodec, FormattedProducerCodePoint) {
  using type = unicode::CodePoint;
  EXPECT_EQ(decode<type>("'a',"), type('a'));
  EXPECT_EQ(decode<type>("'€'"), type(U'€'));
  EXPECT_EQ(decode<type>("U+0061,"), type('a'));
  EXPECT_EQ(decode<type>("U+20AC"), type(U'€'));
  EXPECT_EQ(decode<type>("U+10FFF"), type(U'\U00010FFF'));
  EXPECT_EQ(decode<type>("U+10FFFF"), type(U'\U0010FFFF'));
}

// EOF
