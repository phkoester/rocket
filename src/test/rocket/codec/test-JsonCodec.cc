/*
 * test-JsonCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/enum-format.h"
#include "rocket/enum-scan.h"
#include "rocket/int128-format.h"
#include "rocket/int128-scan.h"
#include "rocket/scope.h"
#include "rocket/codec/JsonCodec.h"
#include "rocket/chrono/chrono-format.h"
#include "rocket/chrono/chrono-scan.h"
#include "rocket/log/log.h"
#include "rocket/math/Interval-format.h"
#include "rocket/math/Interval-scan.h"
#include "rocket/reflect/reflect.h"
#include "rocket/str/location/location.h"
#include "rocket/system/system.h"
#include "rocket/unicode/unicode-format.h"
#include "rocket/unicode/unicode-scan.h"

using namespace rocket;
using namespace rocket::codec;
using namespace std;

#define MODULE rocketCodecJsonCodec

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

const JsonCodec jsonCodec;

// Local functions ------------------------------------------------------------------------------------------

template<typename T>
string
encode(const T& val, const JsonConsumerConfig& config = {}) {
  nio::StringSink out;
  jsonCodec.encode(val, out, config);
  return out.str();
}

template<typename T>
[[nodiscard]] DecodeResult<T>
decode(string_view str) {
  nio::StringSource in(str);
  return jsonCodec.decode<T>(in);
}

template<typename T>
[[nodiscard]] pair<DecodeResult<T>, u64>
decodeAndTell(string_view str) {
  nio::StringSource in(str);
  return { jsonCodec.decode<T>(in), in.position() };
}

// `Character` has no default constructor, so `JsonCodec::decode` cannot create one
template<typename T>
[[nodiscard]] DecodeResult<T>
decodeInto(T val, string_view str) {
  nio::StringSource in(str);
  return jsonCodec.Base::decode<T>(val, in);
}

// Local test functions -------------------------------------------------------------------------------------

template<typename T>
void
expectFailure(string_view input, string_view message, u64 position, const str::Ranges& ranges = {}) {
  const auto result = decode<T>(input);
  ASSERT_FALSE(result.has_value()) << input;
  EXPECT_EQ(result.error().message, message) << input;
  EXPECT_EQ(result.error().position, position) << input;
  EXPECT_EQ(result.error().ranges, ranges) << input;
}

} // namespace

// Tests ====================================================================================================

// `JsonConsumer` -------------------------------------------------------------------------------------------

TEST(MODULE, ConsumerBool) {
  EXPECT_EQ(encode(true), "true");
  EXPECT_EQ(encode(false), "false");
}

TEST(MODULE, ConsumerChar) {
  EXPECT_EQ(encode('\x7F'), "\"\\u007F\"");
  EXPECT_EQ(encode('\t'), "\"\\t\"");
  EXPECT_EQ(encode(U'€'), "\"€\"");
  EXPECT_EQ(encode(U'\u200B'), "\"\\u200B\"");
  EXPECT_EQ(encode(U'\U0001F971'), "\"\U0001F971\""); // Printable, so verbatim
  EXPECT_EQ(encode(U'\U0010FFFF'), "\"\\uDBFF\\uDFFF\""); // Not printable, so a surrogate pair
}

TEST(MODULE, ConsumerEnum) {
  enum Color : u8 { Red, Green, Blue };
  EXPECT_THAT(
    [] { encode(Blue); },
    ThrowsMessage<InvalidState>(matchesRegex(".*cannot format enum of type `.*Color`")));

  EXPECT_EQ(encode(log::LogLevel::info), "\"info\"");
  EXPECT_THAT(
    [&] { encode(static_cast<log::LogLevel>(-1)); }, // NOLINT
    ThrowsMessage<InvalidState>(EndsWith("invalid `rocket::log::LogLevel` value 255")));
}

TEST(MODULE, ConsumerIntegerI64) {
  EXPECT_EQ(encode(-42_i64), "-42");
  EXPECT_EQ(encode(+42_i64), "42");
}

TEST(MODULE, ConsumerFloatF64) {
  using type = f64;
  using limits = numeric_limits<type>;

  EXPECT_EQ(encode(-.1_f64), "-0.1");
  EXPECT_EQ(encode(1._f64), "1");
  EXPECT_EQ(encode(-123.456_f64), "-123.456");
  EXPECT_EQ(encode(-limits::infinity()), "-Infinity");
  EXPECT_EQ(encode(limits::infinity()), "Infinity");
}

TEST(MODULE, ConsumerPointer) {
  EXPECT_EQ(encode(reinterpret_cast<void*>(0)), "null");
  EXPECT_THAT(encode(reinterpret_cast<void*>(0x1234ABCD)), matchesRegex("\"0x[0-9A-F]+\""));
}

TEST(MODULE, ConsumerString) {
  EXPECT_EQ(encode("Hello"sv), "\"Hello\"");
  EXPECT_EQ(encode(U"Hello"sv), "\"Hello\"");
  EXPECT_EQ(encode("\x7f"sv), "\"\\u007F\"");
  EXPECT_EQ(encode(U"\u200B"sv), "\"\\u200B\"");
  EXPECT_EQ(encode(U"\U0001F971"sv), "\"\U0001F971\""); // Printable, so verbatim
  EXPECT_EQ(encode(U"\U0010FFFF"sv), "\"\\uDBFF\\uDFFF\""); // Not printable, so a surrogate pair
  EXPECT_EQ(encode(U"a\U000F0000b"sv), "\"a\\uDB80\\uDC00b\""); // U+F0000 is a private-use code point
}

TEST(MODULE, ConsumerStringShortEscapes) {
  EXPECT_EQ(encode("\b\t\n\f\r\"\\"sv), R"("\b\t\n\f\r\"\\")");
  EXPECT_EQ(encode("'/"sv), R"("'/")"); // Neither needs escaping
}

TEST(MODULE, ConsumerStringNoNonstandardEscapes) {
  // JSON has no `\a`, `\v`, or `\e`
  EXPECT_EQ(encode("\a"sv), R"("\u0007")");
  EXPECT_EQ(encode("\v"sv), R"("\u000B")");
  EXPECT_EQ(encode("\x1b"sv), R"("\u001B")");
  EXPECT_EQ(encode("a\a\vb"sv), R"("a\u0007\u000Bb")");
  EXPECT_EQ(encode('\a'), R"("\u0007")");
  EXPECT_EQ(encode(U'\v'), R"("\u000B")");

  EXPECT_EQ(encode("\x01"sv), R"("\u0001")");
}

TEST(MODULE, ConsumerOptional) {
  using type = optional<string>;

  const type val;
  EXPECT_EQ(encode(val), "null");
  EXPECT_EQ(encode<type>("Hello"), "\"Hello\"");
}

TEST(MODULE, ConsumerTuplePair) {
  EXPECT_EQ(encode(make_pair("answer"sv, 42)), "[\"answer\", 42]");
  EXPECT_EQ(encode(make_pair("answer"sv, 42), { .indent=true }),
    "[\n"
    "  \"answer\",\n"
    "  42\n"
    "]");
}

TEST(MODULE, ConsumerList) {
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

TEST(MODULE, ConsumerSet) {
  EXPECT_EQ(encode(set<i32> {}), "[]");
  EXPECT_EQ(encode(set<i32> { 1, 2, 2, 3 }), "[1, 2, 3]");
}

TEST(MODULE, ConsumerMap) {
  EXPECT_EQ(
    encode(map<i32, i32> { { 1, 2 }, { 2, 3 }, { 3, 4 } }),
    "{\"1\": 2, \"2\": 3, \"3\": 4}");
  EXPECT_EQ(
    encode(map<string, i32> { { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } }),
    "{\"alpha\": 1, \"beta\": 2, \"gamma\": 3}");
}

TEST(MODULE, ConsumerDuration) {
  using namespace std::chrono;

  EXPECT_EQ(encode(1ns), "\"1ns\"");
  EXPECT_EQ(encode(2us), "\"2µs\"");
  EXPECT_EQ(encode(3ms), "\"3ms\"");
  EXPECT_EQ(encode(4s), "\"4s\"");
  EXPECT_EQ(encode(5min), "\"5min\"");
  EXPECT_EQ(encode(-6h), "\"-6h\"");
  EXPECT_EQ(encode(days(7)), "\"7d\"");
  EXPECT_EQ(encode(weeks(8)), "\"8[604800]s\"");
  EXPECT_EQ(encode(months(9)), "\"9[2629746]s\"");
  EXPECT_EQ(encode(years(10)), "\"10[31556952]s\"");
}

TEST(MODULE, ConsumerHourMinuteSecond) {
  using namespace std::chrono;

  EXPECT_EQ(encode(hh_mm_ss { 1h + 2min + 3s }), "\"01:02:03\"");
  EXPECT_EQ(encode(hh_mm_ss { 1h + 2min + 3s + 5ms }), "\"01:02:03.005\"");
  EXPECT_EQ(encode(hh_mm_ss { 111h + 2min + 3s + 123ms }), "\"111:02:03.123\"");
  EXPECT_EQ(encode(hh_mm_ss { -(111h + 2min + 3s + 123456us) }), "\"-111:02:03.123456\"");
  EXPECT_EQ(encode(hh_mm_ss { -(111h + 2min + 3s + 123456789ns) }), "\"-111:02:03.123456789\"");
  EXPECT_EQ(encode(hh_mm_ss<minutes> { -2min }), "\"-00:02:00\"");
}

TEST(MODULE, ConsumerDate) {
  using namespace std::chrono;

  EXPECT_EQ(encode(year_month_day { 1970y, January, 2d }), "\"1970-01-02\"");
}

TEST(MODULE, ConsumerTimeZone) {
  using namespace std::chrono;

  EXPECT_EQ(encode(rocket::chrono::locate_zone("Europe/Berlin")), "\"Europe/Berlin\"");
  EXPECT_EQ(encode(rocket::chrono::locate_zone("UTC")), "\"Etc/UTC\"");
}

TEST(MODULE, ConsumerTime) {
  using namespace std::chrono;

  const auto* const regexS = R"("\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z")"; // Seconds
  const auto* const regexNs = R"("\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6,9}Z")"; // Nanoseconds

  const auto now = rocket::chrono::now<system_clock>();
  EXPECT_THAT(encode(time_point_cast<seconds>(now)), matchesRegex(regexS));
  EXPECT_THAT(encode(now), matchesRegex(regexNs));
}

TEST(MODULE, ConsumerZonedTime) {
  using namespace std::chrono;

  const auto* const regexSecond =
    R"(\"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(Z|[+-]\d{2}:\d{2}) \([^ ]+\)\")"; // Seconds
  const auto* const regexNanosecond =
    R"(\"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6,9}(Z|[+-]\d{2}:\d{2}) \([^ ]+\)\")"; // Nanoseconds

  {
    // Current time zone
    const auto* current = rocket::chrono::current_zone();
    const auto now = rocket::chrono::now<system_clock>();
    EXPECT_THAT(
      encode(rocket::chrono::zoned_time(current, time_point_cast<seconds>(now))),
      matchesRegex(regexSecond));
    EXPECT_THAT(
      encode(rocket::chrono::zoned_time(current, now)),
      matchesRegex(regexNanosecond));
  }

  {
    // Time zone UTC
    const auto* utc = rocket::chrono::locate_zone("UTC");
    const auto now = rocket::chrono::now<system_clock>();
    EXPECT_THAT(
      encode(rocket::chrono::zoned_time(utc, time_point_cast<seconds>(now))),
      matchesRegex(regexSecond));
    EXPECT_THAT(
      encode(rocket::chrono::zoned_time(utc, now)),
      matchesRegex(regexNanosecond));
  }
}

TEST(MODULE, ConsumerInterval) {
  using namespace rocket::math;
  EXPECT_EQ(encode(ClosedInterval<f32>(-4.2F, 4.2F)), "\"[-4.2,4.2]\"");
  EXPECT_EQ(encode(LeftOpenInterval<f32>(-4.2F, 4.2F)), "\"(-4.2,4.2]\"");
  EXPECT_EQ(encode(RightOpenInterval<f32>(-4.2F, 4.2F)), "\"[-4.2,4.2)\"");
  EXPECT_EQ(encode(RightOpenInterval<f32>(-4.2F, nullopt)), "\"[-4.2,∞)\"");
  EXPECT_EQ(encode(LeftOpenInterval<i32>(nullopt, -12)), "\"(-∞,-12]\"");
  EXPECT_EQ(encode(OpenInterval<f32>()), "\"∅\"");
  EXPECT_EQ(encode(OpenInterval<f32>(nullopt, nullopt)), "\"(-∞,∞)\"");
  EXPECT_EQ(encode(ClosedInterval<i32>(2, 1)), "\"∅\"");
}

TEST(MODULE, ConsumerDeclared) {
  const MyStruct val { 42, true, "hello", { 1, 2, 3 } };
  EXPECT_EQ(
    encode(val),
    "{\"ärger\": 42, \"ökonom\": true, \"übermut\": \"hello\", \"vec\": [1, 2, 3]}");
  EXPECT_EQ(decode<MyStruct>(encode(val)), val);
}

TEST(MODULE, ConsumerInstance) {
  using type = reflect::Instance<MyStruct, MyStruct::Three>;
  const type val(MyStruct { 42, true, "hello", { 1, 2, 3 } });
  EXPECT_EQ(encode(val), "{\"ärger\": 42, \"ökonom\": true, \"übermut\": \"hello\"}");
  EXPECT_EQ(decode<type>(encode(val)), val);
}

TEST(MODULE, ConsumerVarRef) {
  int a = 0, b = 1, c= 2;
  const auto& refs = ROCKET_REFLECT_VARS((a)(b)(c));
  EXPECT_EQ(encode(refs), "{\"a\": 0, \"b\": 1, \"c\": 2}");
  const auto& ref = std::get<1>(refs);
  EXPECT_EQ(encode(ref), "\"b\": 1");
}

TEST(MODULE, ConsumerCodePoint) {
  using type = unicode::CodePoint;

  EXPECT_EQ(encode(type('a')), "\"U+0061\"");
  EXPECT_EQ(encode(type(U'€')), "\"U+20AC\"");
  EXPECT_EQ(encode(type(U'\U0010FFFF')), "\"U+10FFFF\"");
}

// `JsonProducer` -------------------------------------------------------------------------------------------

TEST(MODULE, ProducerBool) {
  EXPECT_EQ(decode<bool>("// sup\ntrue"), true);
  EXPECT_EQ(decode<bool>("  /* comment\nanother line in the comment */\r\n\ttrue"), true);
  EXPECT_EQ(decode<bool>("\r\n  true"), true);
  EXPECT_EQ(decode<bool>("false"), false);
  EXPECT_EQ(decode<bool>("\r\nx").error().message, "expected a boolean value");
}

TEST(MODULE, ProducerChar) {
  EXPECT_EQ(decode<char>("\"a\""), 'a');
  EXPECT_EQ(decode<char>("\"'\""), '\'');
  EXPECT_EQ(decode<char>("\"\\t\""), '\t');
  EXPECT_EQ(decode<char>("  \"ä\"").error().message, "invalid character literal");

  EXPECT_EQ(decode<char>("  \"\\xFF\"").error().message, "invalid character literal");
}

TEST(MODULE, ProducerChar32) {
  EXPECT_EQ(decode<char32>("\"ä\""), U'ä');
  EXPECT_EQ(decode<char32>("\"\\u20ac\""), U'€');
  EXPECT_EQ(decode<char32>("\"\\U0010FFFF\""), U'\U0010FFFF');
  EXPECT_EQ(decode<char32>("\"€\""), U'€');

  EXPECT_EQ(decode<char32>("  \"\\x800D\"").error().message, "invalid character literal");
}

TEST(MODULE, ProducerCharSlash) {
  EXPECT_EQ(decode<char>(R"("\/")"), '/');
  EXPECT_EQ(decode<char32>(R"("\/")"), U'/');
}

TEST(MODULE, ProducerChar32SurrogatePair) {
  EXPECT_EQ(decode<char32>(R"("\uD83E\uDD71")"), U'\U0001F971'); // U+1F971 (Yawning Face)
  EXPECT_EQ(decode<char32>(R"("\uD83D")").error().message, "Unpaired surrogate in escape sequence");
}

TEST(MODULE, ProducerEnum) {
  enum Color : u8 { Red, Green, Blue };

  EXPECT_THAT(decode<Color>("2").error().message, matchesRegex("cannot scan enum of type `.*Color`"));

  EXPECT_EQ(decode<log::LogLevel>("  \"info\"  "), log::LogLevel::info);
  EXPECT_EQ(
    decode<log::LogLevel>("\"bogus\"").error().message,
    "invalid value for enum `rocket::log::LogLevel`");
}

TEST(MODULE, ProducerInteger) {
  EXPECT_EQ(decode<i32>("-42"), -42);
  EXPECT_EQ(decode<i32>("0xABcd"), 0xABCD);
  EXPECT_EQ(decode<i32>("077"), 077);
  EXPECT_EQ(decode<i32>("0o123"), 0123);
  EXPECT_EQ(decode<i32>("0b1010101"), 0b1010101);

  EXPECT_EQ(decode<i32>("  x").error().message, "expected an integer value");
}

TEST(MODULE, ProducerFloat) {
  using type = f64;
  using limits = numeric_limits<type>;

  EXPECT_EQ(decodeAndTell<type>("  -123.456  "), make_pair(-123.456_f64, 10_u64));
  EXPECT_EQ(decode<type>("  -1e3"), -1e3_f64);
  EXPECT_EQ(decode<type>("-Infinity"), -limits::infinity());
  EXPECT_EQ(decode<type>("Infinity"), limits::infinity());
  EXPECT_EQ(decode<type>("+Infinity"), limits::infinity());

  EXPECT_TRUE(isnan(decode<type>("NaN").value()));
}

TEST(MODULE, ProducerPointer) {
  EXPECT_EQ(decodeAndTell<void*>("  null  "), make_pair(static_cast<void*>(0), 6_u64)); // NOLINT
  EXPECT_EQ(
    decodeAndTell<void*>("  \"0x12345678\"  "),
    make_pair(reinterpret_cast<void*>(0x12345678), 14_u64));
}

TEST(MODULE, ProducerStringSlash) {
  EXPECT_EQ(decode<string>(R"("a\/b")"), "a/b");
  EXPECT_EQ(decode<string>(R"("a/b")"), "a/b");
}

TEST(MODULE, ProducerStringSurrogatePair) {
  EXPECT_EQ(decode<string>(R"("\uD83E\uDD71")"), "🥱"); // U+1F971 (Yawning Face)
  EXPECT_EQ(decode<string>(R"("\ud83e\udd71")"), "🥱"); // Lowercase digits
  EXPECT_EQ(decode<string>(R"("a\uD83E\uDD71b")"), "a🥱b");
  EXPECT_EQ(decode<string>(R"("\uD800\uDC00")"), "\U00010000"); // Lowest pair
  EXPECT_EQ(decode<string>(R"("\uDBFF\uDFFF")"), "\U0010FFFF"); // Highest pair
  EXPECT_EQ(decode<u32string>(R"("\uD83E\uDD71")"), U"\U0001F971");
}

TEST(MODULE, ProducerStringBmpEscapesAroundSurrogates) {
  EXPECT_EQ(decode<string>(R"("\uD7FF")"), "\uD7FF"); // Just below the surrogates
  EXPECT_EQ(decode<string>(R"("\uE000")"), "\uE000"); // Just above the surrogates
}

TEST(MODULE, ProducerInvalidUtf8) {
  constexpr string_view MESSAGE = "invalid UTF-8 byte sequence";

  EXPECT_EQ(decode<string>("\"a\xFF\"").error().message, MESSAGE);
  EXPECT_EQ(decode<string>("\"\xE2\x82\"").error().message, MESSAGE); // Truncated sequence
  EXPECT_EQ(decode<char>("\"\xFF\"").error().message, MESSAGE);
  EXPECT_EQ(decode<std::chrono::seconds>("\"\xFF\"").error().message, MESSAGE); // Through `readJsonString`
}

TEST(MODULE, ProducerStringUnpairedSurrogate) {
  constexpr string_view MESSAGE = "Unpaired surrogate in escape sequence";

  EXPECT_EQ(decode<string>(R"("\uD83E")").error().message, MESSAGE); // High surrogate at EOI
  EXPECT_EQ(decode<string>(R"("\uD83Ex")").error().message, MESSAGE); // High surrogate, then no backslash
  EXPECT_EQ(decode<string>(R"("\uD83E\n")").error().message, MESSAGE); // High surrogate, then another escape
  EXPECT_EQ(decode<string>(R"("\uD83EA")").error().message, MESSAGE); // High surrogate, then no surrogate
  EXPECT_EQ(decode<string>(R"("\uD83E\uD83E")").error().message, MESSAGE); // Two high surrogates
  EXPECT_EQ(decode<string>(R"("\uDD71")").error().message, MESSAGE); // Low surrogate on its own
  EXPECT_EQ(decode<string>(R"("\uDD71\uD83E")").error().message, MESSAGE); // Pair in the wrong order

  EXPECT_EQ(decode<string>(R"("\uD83E\uDD")").error().message, "expected 4 hexadecimal digits, got EOI");
}

TEST(MODULE, StringEscapesRoundtrip) {
  const string in = "\a\b\t\n\v\f\r\x1b\"\\/'🥱";
  const auto out = encode(in);
  EXPECT_EQ(out.find("\\a"), string::npos);
  EXPECT_EQ(out.find("\\v"), string::npos);
  EXPECT_EQ(out.find("\\e"), string::npos);
  EXPECT_EQ(decode<string>(out), in);
}

TEST(MODULE, ProducerOptionalString) {
  using type = optional<string>;
  EXPECT_EQ(decode<type>("  null  "), nullopt);
}

TEST(MODULE, ProducerOptionalStringView) {
  nio::StringSource in("\"Hello\""); // The source must remain valid for the string view
  EXPECT_EQ(jsonCodec.decode<optional<basic_string_view<char32>>>(in), U"Hello"sv);
}

TEST(MODULE, ProducerTuple) {
  EXPECT_EQ((decode<pair<string, i32>>("  [  \"answer\"   , 42   ]")), make_pair("answer"s, 42_i32));
  EXPECT_EQ(
    (decode<pair<string, i32>>("  [  // comment\n \"answer\"   , 42  /* comment */  , // comment\n  ]")),
    make_pair("answer"s, 42_i32));

  EXPECT_EQ((decode<tuple<i32, i32, bool>>("[1, 2, true]")), make_tuple(1_i32, 2_i32, true));
}

TEST(MODULE, ProducerList) {
  EXPECT_EQ((decode<array<i32, 3>>("  [ 1, 2, 3   ]")), (array<i32, 3> { 1, 2, 3 }));
  EXPECT_EQ((decode<array<i32, 3>>("  [ 1, 2, 3   , ]")), (array<i32, 3> { 1, 2, 3 }));
  EXPECT_EQ((decode<list<i32>>("  [ 1, 2, 3   , ]")), (list<i32> { 1, 2, 3 }));
  EXPECT_EQ((decode<vector<i32>>("[]")), (vector<i32> {}));
  EXPECT_EQ((decode<vector<i32>>("[1, 2, 3]")), (vector<i32> { 1, 2, 3 }));
  EXPECT_EQ((decode<vector<i32>>("[1, 2, 3,]")), (vector<i32> { 1, 2, 3 }));
  EXPECT_EQ((decode<vector<i32>>("  [ 1, 2, 3   , ]")), (vector<i32> { 1, 2, 3 }));
}

TEST(MODULE, ProducerSet) {
  EXPECT_EQ((decode<set<i32>>("  [ 3, 2, 1  ,  ]   ")), (set<i32> { 1, 2, 3 }));
}

TEST(MODULE, ProducerMap) {
  using type = std::map<i32, string>;

  // Test last one wins
  EXPECT_EQ(
    decode<type>("{ \"1\": \"alpha\", \"2\": \"beta\", \"3\": \"gämmä\", \"3\": \"gamma\" }"),
    (type { { 1, "alpha" }, { 2, "beta" }, { 3, "gamma" } }));
}

TEST(MODULE, ProducerBimap) {
  using type = bimap<string, i32>;
  // Test last one wins
  EXPECT_EQ(
    (decode<type>(
      "  { \"alpha\"\t: 1, \"beta\"  :/* comment */ 2, \"gamma\": 4, \"gamma\": 3  ,  }   ")),
    (make_bimap<string, i32>({ { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } })));
  // Also for a value that is already mapped from another key
  EXPECT_EQ(
    (decode<type>("{ \"alpha\": 1, \"beta\": 2, \"gamma\": 1 }")),
    (make_bimap<string, i32>({ { "beta", 2 }, { "gamma", 1 } })));
}

TEST(MODULE, ProducerBimapUnordered) {
  using type = unordered_bimap<string, i32>;
  // Test last one wins
  EXPECT_EQ(
    (decode<type>("  { \"alpha\"\t: 1, \"beta\"  :/* comment */ 2, \"gamma\": 4, \"gamma\": 3  ,  }   ")),
    (make_unordered_bimap<string, i32>({ { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } })));
  // Also for a value that is already mapped from another key
  EXPECT_EQ(
    (decode<type>("{ \"alpha\": 1, \"beta\": 2, \"gamma\": 1 }")),
    (make_unordered_bimap<string, i32>({ { "beta", 2 }, { "gamma", 1 } })));
}

TEST(MODULE, ProducerDuration) {
  using namespace std::chrono;

  EXPECT_EQ(decode<milliseconds>("\"1s\""), 1000ms);
  EXPECT_EQ(decode<milliseconds>("\"1s\""), 1s);
  EXPECT_EQ(decode<milliseconds>("\"-5s\""), -5s);
  EXPECT_EQ(decode<seconds>("\"1000ms\""), 1s);
  EXPECT_EQ(decode<microseconds>("\"7us\""), 7us);
  EXPECT_EQ(decode<microseconds>("\"7µs\""), 7us);
  EXPECT_EQ(decode<days>("\"2[604800]s\""), days(14));
  EXPECT_EQ(decode<seconds>("\"3[1/1]s\""), 3s);

  // Roundtrips, also of units without a suffix of their own
  EXPECT_EQ(decode<weeks>(encode(weeks(8))), weeks(8));
  EXPECT_EQ(decode<months>(encode(months(9))), months(9));
  EXPECT_EQ(decode<years>(encode(years(10))), years(10));

  // The value must be exactly representable in the target type, unless that has a floating-point count
  expectFailure<seconds>("  \"1500ms\"", "duration not representable in the target type", 2 + 1);
  EXPECT_EQ(decode<duration<f64>>("\"1500ms\""), duration<f64>(1.5));

  EXPECT_EQ(decode<seconds>("x").error().message, "expected a string");
  expectFailure<seconds>("  \"x\"", "expected a duration", 2 + 1);
  expectFailure<seconds>("  \"10x\"", "expected a duration", 2 + 1);
  expectFailure<seconds>("  \"10mix\"", "expected a duration", 2 + 1);
  expectFailure<seconds>("  \"10w\"", "expected a duration", 2 + 1);

  // Range checks: 292 years fit into `nanoseconds`, 300 years do not
  EXPECT_EQ(decode<nanoseconds>("\"292[31556952]s\""), duration_cast<nanoseconds>(years(292)));
  EXPECT_EQ(decode<nanoseconds>("\"9223372036854775807ns\""), nanoseconds::max());
  EXPECT_EQ(decode<nanoseconds>("\"-9223372036854775808ns\""), nanoseconds::min());
  expectFailure<nanoseconds>("  \"300[31556952]s\"", "duration out of range", 2 + 1);
  expectFailure<nanoseconds>("  \"-300[31556952]s\"", "duration out of range", 2 + 1);
  expectFailure<duration<i32, std::milli>>("  \"3000h\"", "duration out of range", 2 + 1);

  // A floating-point representation cannot overflow
  EXPECT_TRUE((decode<duration<f32, std::nano>>("\"300[31556952]s\"")));
}

TEST(MODULE, ProducerEscapeFailurePosition) {
  using Map = map<string, string>;
  constexpr string_view INVALID_ESCAPE = "invalid escape sequence";

  expectFailure<string>(R"(  "\q")", INVALID_ESCAPE, 3, { { 3, 5 } });
  expectFailure<char>(R"(  "\q")", INVALID_ESCAPE, 3, { { 3, 5 } });
  expectFailure<Map>("{\n  \"a\": \"\\q\"\n}", INVALID_ESCAPE, 10, { { 10, 12 } }); // Value on a later line
  expectFailure<Map>(R"({"\q": "x"})", INVALID_ESCAPE, 2, { { 2, 4 } }); // Key
  // Through `readJsonString`
  expectFailure<std::chrono::seconds>(R"( "\q")", INVALID_ESCAPE, 2, { { 2, 4 } });

  expectFailure<string>(R"("\uDD71")", "Unpaired surrogate in escape sequence", 1, { { 1, 7 } });
  expectFailure<string>("  \"a\xFF\"", "invalid UTF-8 byte sequence", 4);
}

TEST(MODULE, ProducerKeyFailureRanges) {
  using Map = map<pair<i32, i32>, i32>;
  constexpr string_view UNTERMINATED = "unterminated array";

  expectFailure<Map>("{\"[1, 2\": 3}", UNTERMINATED, 7, { { 2, 7 } });

  // An escape sequence is longer in the input than after unescaping, so the bounds must be mapped
  expectFailure<Map>(R"({"\u005B1, 2": 3})", UNTERMINATED, 12, { { 2, 12 } });
}

TEST(MODULE, ProducerUnexpectedInputAfterValue) {
  using namespace std::chrono;
  using TimePoint = time_point<system_clock, nanoseconds>;
  using ZonedTime = rocket::chrono::zoned_time<nanoseconds>;
  constexpr string_view UNEXPECTED = "unexpected input after value";

  expectFailure<seconds>("\"5s!\"", UNEXPECTED, 3, { { 3, 4 } });
  expectFailure<seconds>("  \"5s!\"", UNEXPECTED, 5, { { 5, 6 } });
  expectFailure<year_month_day>("\"1970-01-02x\"", UNEXPECTED, 11, { { 11, 12 } });
  expectFailure<hh_mm_ss<seconds>>("\"01:02:03x\"", UNEXPECTED, 9, { { 9, 10 } });
  expectFailure<TimePoint>("\"2026-10-07T09:05:26Zx\"", UNEXPECTED, 21, { { 21, 22 } });
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00+02:00 (Europe/Berlin)x\"", UNEXPECTED, 42, { { 42, 43 } });
  expectFailure<log::LogLevel>("\"info x\"", UNEXPECTED, 6, { { 6, 7 } });
  expectFailure<void*>("\"0x1234!\"", UNEXPECTED, 7, { { 7, 8 } });
  expectFailure<map<i32, i32>>("{\"1!\": 2}", UNEXPECTED, 3, { { 3, 4 } });

  // An escape sequence is longer in the input than after unescaping, so the bounds must be mapped
  expectFailure<seconds>(R"("5s\u0021")", UNEXPECTED, 3, { { 3, 9 } });

  // Whitespace may follow the value, just as it may precede it
  EXPECT_EQ(decode<seconds>("\"5s \""), 5s);
  EXPECT_EQ(decode<year_month_day>("\"1970-01-02\\t\""), (year_month_day { 1970y, January, 2d }));
  EXPECT_EQ(decode<log::LogLevel>("\"info \""), log::LogLevel::info);
}

TEST(MODULE, ProducerFailureInsideCluster) {
  using namespace std::chrono;

  // A digit or dot and U+0301 (COMBINING ACUTE ACCENT) form one cluster. A duration failure is reported at
  // the start of the string content, which is also the start of the cluster

  const auto simple = decode<seconds>("\"5\u0301\"");
  EXPECT_EQ(simple.error().message, "expected a duration");
  EXPECT_EQ(simple.error().position, 1U);

  const auto indented = decode<seconds>("  \"5\u0301\"");
  EXPECT_EQ(indented.error().message, "expected a duration");
  EXPECT_EQ(indented.error().position, 3U);

  const auto afterEscape = decode<seconds>("\"\\u00355\u0301\"");
  EXPECT_EQ(afterEscape.error().message, "expected a duration");
  EXPECT_EQ(afterEscape.error().position, 1U);

  // An `hh_mm_ss` failure is reported at the start of the string content, too

  const auto rethrown = decode<hh_mm_ss<milliseconds>>("\"01:02:03.\u0301\"");
  EXPECT_EQ(rethrown.error().message, "expected subseconds");
  EXPECT_EQ(rethrown.error().position, 1U);
}

TEST(MODULE, ProducerFailureInsideClusterOutsideString) {
  using Pair = pair<i32, i32>;

  // A digit and U+0301 (COMBINING ACUTE ACCENT) form one cluster, so these failures lie inside it. The
  // reported positions are the starts of the clusters
  expectFailure<Pair>("[1́, 2]", "expected ','", 1);
  expectFailure<Pair>("[1, 2́", "unterminated array", 4, { { 0, 4 } });

  // A failure at a cluster boundary stays where it is
  expectFailure<Pair>("[1; 2]", "expected ','", 2);

  // The source stays where the failure occurred
  EXPECT_EQ(decodeAndTell<Pair>("[1́, 2]").second, 2U);

  // The position is now a character boundary, so `locations` accepts it
  namespace loc = rocket::str::location;
  const string input = "[1́, 2]";
  const auto result = decode<Pair>(input);
  ASSERT_FALSE(result.has_value());
  const loc::Position pos {
    .type=loc::error, .position=result.error().position.value(), .message=result.error().message
  };
  EXPECT_EQ(loc::locations(input, { pos }).locations.at(0).column, 2U);
}

TEST(MODULE, ProducerCodePointInvalid) {
  using CodePoint = unicode::CodePoint;

  expectFailure<CodePoint>("  \"U+110000\"", "invalid code point", 2);
  expectFailure<CodePoint>("  \"U+D800\"", "invalid code point", 2);
  expectFailure<pair<i32, CodePoint>>("[1, \"U+110000\"]", "invalid code point", 4);
  expectFailure<CodePoint>("  \"U+\"", "expected hexadecimal digits", 2);
  expectFailure<CodePoint>("  \"U+G\"", "expected hexadecimal digits", 2);
  expectFailure<CodePoint>("  \"U+0041x\"", "unterminated code point", 9, { { 2, 9 } });
}

TEST(MODULE, ProducerCharacter) {
  const unicode::Character<char> character("x"s);
  const unicode::CharacterView<char> view("x"sv);

  EXPECT_EQ(static_cast<string_view>(decodeInto(character, "\"a\"").value()), "a");
  EXPECT_EQ(static_cast<string_view>(decodeInto(view, "\"a\"").value()), "a");

  const auto empty = decodeInto(character, "  \"\"");
  EXPECT_EQ(empty.error().message, "expected a character");
  EXPECT_EQ(empty.error().position, 2U);

  const auto emptyView = decodeInto(view, "\"\"");
  EXPECT_EQ(emptyView.error().message, "expected a character");
  EXPECT_EQ(emptyView.error().position, 0U);
}

TEST(MODULE, ProducerHourMinuteSecond) {
  using namespace std::chrono;

  EXPECT_EQ(
    decode<hh_mm_ss<seconds>>("\"01:02:03\"").value().to_duration(),
    (hh_mm_ss { 1h + 2min + 3s }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<seconds>>("\"01:02:03.123\"").value().to_duration(),
    (hh_mm_ss { 1h + 2min + 3s }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<milliseconds>>("\"01:02:03.123456\"").value().to_duration(),
    (hh_mm_ss { 1h + 2min + 3s + 123ms }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<microseconds>>("\"-111:02:03.123456789\"").value().to_duration(),
    (hh_mm_ss { -(111h + 2min + 3s + 123456us) }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<nanoseconds>>("\"-111:02:03.123456789\"").value().to_duration(),
    (hh_mm_ss { -(111h + 2min + 3s + 123456789ns) }).to_duration());
  EXPECT_EQ(decode<hh_mm_ss<seconds>>("\"-00:02:00\"").value().to_duration(), -2min);
  EXPECT_EQ(decode<hh_mm_ss<seconds>>("\" -00:02:00\"").value().to_duration(), -2min);
  EXPECT_EQ(decode<hh_mm_ss<seconds>>("\" -01:02:03\"").value().to_duration(), -(1h + 2min + 3s));
  EXPECT_EQ(decode<hh_mm_ss<seconds>>("\"\\t-01:02:03\"").value().to_duration(), -(1h + 2min + 3s));

  EXPECT_EQ(decode<hh_mm_ss<milliseconds>>("x").error().message, "expected a string");
  EXPECT_EQ(decode<hh_mm_ss<milliseconds>>("\"x\"").error().message, "expected an hour, minute, and second");
  EXPECT_EQ(
    decode<hh_mm_ss<milliseconds>>("\"01:02:\"").error().message,
    "expected an hour, minute, and second");
  EXPECT_EQ(decode<hh_mm_ss<milliseconds>>("\"01:02:03.\"").error().message, "expected subseconds");
}

TEST(MODULE, ProducerHourMinuteSecondInvalid) {
  using namespace std::chrono;
  constexpr string_view INVALID = "invalid hour, minute, or second";

  expectFailure<hh_mm_ss<seconds>>("\"01:60:00\"", INVALID, 1);
  expectFailure<hh_mm_ss<seconds>>("\"01:00:60\"", INVALID, 1);

  // Only the hours may have a sign, in front of them
  expectFailure<hh_mm_ss<seconds>>("\"01:-1:00\"", "expected an hour, minute, and second", 1);
  expectFailure<hh_mm_ss<seconds>>("\"01:00:-1\"", "expected an hour, minute, and second", 1);
  expectFailure<hh_mm_ss<seconds>>("  \"01:99:99\"", INVALID, 3);

  // The sum of all parts would not fit into nanoseconds
  expectFailure<hh_mm_ss<seconds>>("\"2562047:00:00\"", INVALID, 1);
  expectFailure<hh_mm_ss<seconds>>("\"-2562047:00:00\"", INVALID, 1);

  EXPECT_EQ(decode<hh_mm_ss<seconds>>("\"01:59:59\"").value().to_duration(), 1h + 59min + 59s);
  EXPECT_EQ(decode<hh_mm_ss<seconds>>("\"2562046:59:59\"").value().to_duration(), 2562046h + 59min + 59s);
}

TEST(MODULE, ProducerDateInvalid) {
  using namespace std::chrono;
  constexpr string_view INVALID = "invalid date";

  expectFailure<year_month_day>("\"2026-13-01\"", INVALID, 1);
  expectFailure<year_month_day>("\"2026-00-01\"", INVALID, 1);
  expectFailure<year_month_day>("\"2026-01-32\"", INVALID, 1);
  expectFailure<year_month_day>("\"2026-01-00\"", INVALID, 1);
  expectFailure<year_month_day>("\"2026-02-30\"", INVALID, 1);
  expectFailure<year_month_day>("\"2025-02-29\"", INVALID, 1); // No leap year
  expectFailure<year_month_day>("  \"2026-13-45\"", INVALID, 3);

  // These would be truncated to valid values
  expectFailure<year_month_day>("\"2026-257-01\"", INVALID, 1);
  expectFailure<year_month_day>("\"2026-01-257\"", INVALID, 1);

  // A nonnegative year has exactly four digits, so later years cannot be read
  expectFailure<year_month_day>("\"70000-01-01\"", "expected a date", 1);

  EXPECT_EQ(decode<year_month_day>("\"2024-02-29\""), (year_month_day { 2024y, February, 29d }));
}

TEST(MODULE, ProducerDate) {
  using namespace std::chrono;

  EXPECT_EQ(decode<year_month_day>("\"1970-01-02\""), (year_month_day { 1970y, January, 2d }));
  EXPECT_EQ(decode<year_month_day>("\"-100-01-02\""), (year_month_day { -100y, January, 2d }));

  EXPECT_EQ(decode<year_month_day>("x").error().message, "expected a string");
  EXPECT_EQ(decode<year_month_day>("\"x\"").error().message, "expected a date");

  // Negative years, as {fmt} writes them, roundtrip
  for (const int y : { -32767, -10000, -100, -5, 0, 5, 2026, 9999 }) {
    const year_month_day val { year(y), March, 4d };
    EXPECT_EQ(decode<year_month_day>(encode(val)), val) << encode(val);
  }
}

TEST(MODULE, ProducerTimeZone) { // NOLINT(*-complexity)
  using namespace std::chrono;

  EXPECT_EQ(decode<TimeZone>("\"Europe/Berlin\""), rocket::chrono::locate_zone("Europe/Berlin"));
  EXPECT_EQ(decode<TimeZone>(R"("Europe\/Berlin")"), rocket::chrono::locate_zone("Europe/Berlin"));
  EXPECT_EQ(decode<TimeZone>("\"UTC\""), rocket::chrono::locate_zone("UTC"));
  EXPECT_EQ(decode<TimeZone>("x").error().message, "expected a string");

  EXPECT_THAT(decode<TimeZone>("\"x\"").error().message, AnyOf(
    EndsWith("cannot locate zone: x"), // Linux, macOS
    StrEq("unable to locate time_zone with given name"))); // Windows

  // Legacy names that look like abbreviations are rejected, other names are accepted
  for (const auto* name : { "CET", "EST", "MST", "EST5EDT", "PST8PDT", "Japan" }) {
    EXPECT_EQ(
      decode<TimeZone>(fmt::format("\"{}\"", name)).error().message,
      fmt::format("ambiguous time-zone name `{}`", name));
  }
  for (const auto* name : { "UTC", "GMT", "Etc/GMT+5", "US/Eastern" }) {
    EXPECT_EQ(decode<TimeZone>(fmt::format("\"{}\"", name)), rocket::chrono::locate_zone(name)) << name;
  }
}

TEST(MODULE, ProducerTime) {
  using namespace std::chrono;

  using TimePoint = time_point<system_clock, nanoseconds>;

  // Test equality on roundtrip
  const TimePoint val1 = rocket::chrono::now<system_clock>();
  string encoded = encode(val1);
  nio::out.println("VAL1: {}", encoded);
  const TimePoint val2 = decode<TimePoint>(encoded).value(); // NOLINT
  nio::out.println("VAL2: {}", encode(val2));
  EXPECT_EQ(val2, val1);
  EXPECT_EQ(val2.time_since_epoch().count(), val1.time_since_epoch().count());
}

TEST(MODULE, ProducerZonedTime) {
  using namespace std::chrono;

  using ZonedTime = rocket::chrono::zoned_time<nanoseconds>;

  // Test equality on roundtrip
  const auto* current = rocket::chrono::current_zone();
  const auto now = rocket::chrono::now<system_clock>();
  const ZonedTime val1 = rocket::chrono::zoned_time(current, now);
  string encoded = encode(val1);
  nio::out.println("VAL1: {}", encoded);
  const ZonedTime val2 = decode<ZonedTime>(encoded).value(); // NOLINT
  nio::out.println("VAL2: {}", encode(val2));
  EXPECT_EQ(val2, val1);
}

TEST(MODULE, ProducerZonedTimeUtcOffset) {
  using namespace std::chrono;
  using ZonedTime = rocket::chrono::zoned_time<nanoseconds>;
  constexpr string_view INVALID = "invalid UTC offset";

  // The UTC offset must be one that the time zone has at that local time. In October, Europe/Berlin is at
  // +02:00
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00+01:00 (Europe/Berlin)\"", INVALID, 1);
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00Z (Europe/Berlin)\"", INVALID, 1);
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00+24:00 (Europe/Berlin)\"", INVALID, 1);
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00-24:00 (Europe/Berlin)\"", INVALID, 1);
  expectFailure<ZonedTime>("  \"2026-10-07T11:21:00+24:00 (Europe/Berlin)\"", INVALID, 3);
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00.5+24:00 (Europe/Berlin)\"", INVALID, 1);

  // Minutes above 59 are invalid, otherwise `+01:60` would be read as `+02:00`
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00+01:60 (Europe/Berlin)\"", INVALID, 1);
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00+99:99 (Europe/Berlin)\"", INVALID, 1);

  const auto expectSys = [](string_view input, sys_seconds expected) {
    const auto result = decode<ZonedTime>(input);
    ASSERT_TRUE(result.has_value()) << input;
    EXPECT_EQ(result->get_sys_time(), expected) << input;
  };
  const sys_days october = 2026y / October / 7;
  const sys_days january = 2026y / January / 7;
  expectSys("\"2026-10-07T11:21:00+02:00 (Europe/Berlin)\"", october + 9h + 21min);
  expectSys("\"2026-01-07T11:21:00+01:00 (Europe/Berlin)\"", january + 10h + 21min);
  expectSys("\"2026-01-07T11:21:00Z (Europe/London)\"", january + 11h + 21min);
  expectSys("\"2026-01-07T11:21:00+00:00 (UTC)\"", january + 11h + 21min);
}

TEST(MODULE, ProducerZonedTimeTimeZone) {
  using namespace std::chrono;
  using ZonedTime = rocket::chrono::zoned_time<nanoseconds>;
  constexpr string_view EXPECTED = "expected a time point with time zone";

  // Failures are reported at the start of the value
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00Z \"", EXPECTED, 1);
  expectFailure<ZonedTime>("  \"2026-10-07T11:21:00Z (\"", EXPECTED, 3);
  expectFailure<ZonedTime>("  \"2026-10-07T11:21:00Z (Europe/Berlin\"", EXPECTED, 3);
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00Z Europe/Berlin\"", EXPECTED, 1);
  expectFailure<ZonedTime>("\"2026-10-07T11:21:00+01:00 (Nowhere/Nothing)\"", "unknown time zone", 1);
  expectFailure<ZonedTime>("\"2026-01-07T11:21:00+01:00 (CET)\"", "ambiguous time-zone name", 1);

  EXPECT_EQ(
    decode<ZonedTime>("\"2026-01-07T11:21:00Z (Europe/London)\"").value().get_time_zone(),
    rocket::chrono::locate_zone("Europe/London"));
}

TEST(MODULE, ProducerTimeIndependentOfLocalZone) { // NOLINT(*-complexity)
  using namespace std::chrono;
  using TimePoint = time_point<system_clock, nanoseconds>;
  using ZonedTime = rocket::chrono::zoned_time<nanoseconds>;

  const auto expectTime = [](string_view input, TimePoint expected) {
    const auto result = decode<TimePoint>(input);
    ASSERT_TRUE(result.has_value()) << input;
    EXPECT_EQ(*result, expected) << input;
  };
  const auto expectZoned = [](string_view input, TimePoint expected) {
    const auto result = decode<ZonedTime>(input);
    ASSERT_TRUE(result.has_value()) << input;
    EXPECT_EQ(result->get_sys_time(), expected) << input;
  };

  const auto oldTz = system::env::get<string>("TZ");
  ROCKET_SCOPE_EXIT([&] {
    if (oldTz) {
      system::env::set("TZ", *oldTz);
    } else {
      system::env::unset("TZ");
    }
  });

  const sys_days summer = 2026y / July / 15;
  const sys_days spring = 2026y / March / 29; // DST starts in Europe/Berlin at 01:00 UTC
  const sys_days autumn = 2026y / October / 25; // DST ends in Europe/Berlin at 01:00 UTC

  for (const auto* tz : { "UTC", "Europe/Berlin", "America/New_York", "Asia/Tokyo" }) {
    SCOPED_TRACE(tz);
    system::env::set("TZ", tz);

    expectTime("\"2026-07-15T12:00:00Z\"", summer + 12h);
    expectTime("\"2026-07-15T12:00:00.25Z\"", summer + 12h + 250ms);

    // As local times in Europe/Berlin, these do not exist and exist twice, respectively
    expectTime("\"2026-03-29T02:30:00Z\"", spring + 2h + 30min);
    expectTime("\"2026-10-25T02:30:00Z\"", autumn + 2h + 30min);

    expectZoned("\"2026-07-15T14:00:00+02:00 (Europe/Berlin)\"", summer + 12h);
    expectZoned("\"2026-03-29T03:30:00+02:00 (Europe/Berlin)\"", spring + 1h + 30min);
    expectZoned("\"2026-10-25T02:30:00+01:00 (Europe/Berlin)\"", autumn + 1h + 30min);
    expectZoned("\"2026-07-15T12:00:00Z (UTC)\"", summer + 12h);
  }

  // `std::mktime` would have moved this to March 2
  expectFailure<TimePoint>("\"2026-02-30T12:00:00Z\"", "expected a time point", 1);
}

TEST(MODULE, ProducerInterval) { // NOLINT(*-complexity)
  using namespace rocket::math;
  EXPECT_EQ(decode<ClosedInterval<f64>>("\"[-4.2,4.2]\""), ClosedInterval<f64>(-4.2_f64, 4.2_f64));
  EXPECT_EQ(decode<ClosedInterval<f64>>("\" [ -4.2 , 4.2 ] \""), ClosedInterval<f64>(-4.2_f64, 4.2_f64));
  EXPECT_EQ(decode<OpenInterval<f64>>("\"∅\""), OpenInterval<f64>());
  EXPECT_EQ(
    decode<OpenInterval<i64>>("  \"(-∞,∞)\" /* Comment */  // Comment"),
    OpenInterval<i64>(nullopt, nullopt));
  EXPECT_EQ(decode<RightOpenInterval<i32>>("\"[5,∞)\""), RightOpenInterval<i32>(5, nullopt));
  EXPECT_EQ(decode<LeftOpenInterval<f64>>("\"(-∞,-12]\""), LeftOpenInterval<f64>(nullopt, -12.0));

  // JSON escapes are resolved before scanning
  EXPECT_EQ(decode<OpenInterval<i64>>("\"(-\\u221E,\\u221E)\""), OpenInterval<i64>(nullopt, nullopt));

  // Roundtrips
  for (const auto& val : { RightOpenInterval<f64>(-1.25, 3.5), RightOpenInterval<f64>(12, nullopt),
                           RightOpenInterval<f64>() }) {
    EXPECT_EQ(decode<RightOpenInterval<f64>>(encode(val)), val) << encode(val);
  }

  // Errors
  expectFailure<ClosedInterval<i32>>("[1, 2]", "expected a string", 0);
  expectFailure<ClosedInterval<i32>>("null", "expected a string", 0);
  expectFailure<ClosedInterval<i32>>("\"[1,2)\"", "expected an interval", 1);
  expectFailure<ClosedInterval<i32>>("\"x\"", "expected an interval", 1);
  expectFailure<ClosedInterval<i32>>("\"[1,2]x\"", "unexpected input after value", 6, { { 6, 7 } });
}

TEST(MODULE, ProducerDeclared) {
  // Test last one wins
  EXPECT_EQ(
    (decode<MyStruct>(
      "  { \"ärger\":  42, \"vec\": [7, 8], \"ökonom\": true, \"übermut\": \"hello\", \"vec\": [1, 2, 3] }   ")),
    (MyStruct { 42, true, "hello", { 1, 2, 3 } }));

  // Members in a different order
  EXPECT_EQ(
    (decode<MyStruct>("{\"vec\": [1, 2, 3], \"übermut\": \"hello\", \"ärger\": 42, \"ökonom\": true}")),
    (MyStruct { 42, true, "hello", { 1, 2, 3 } }));

  // Missing members keep their default values
  EXPECT_EQ(
    (decode<MyStruct>("{\"ökonom\": true, \"vec\": [1, 2, 3]}")),
    (MyStruct { 0, true, "", { 1, 2, 3 } }));
  EXPECT_EQ((decode<MyStruct>("{}")), (MyStruct {}));

  // Unknown members are an error
  EXPECT_EQ(decode<MyStruct>("{\"ärger\": 42, \"bogus\": 1}").error().message, "unknown member `bogus`");
}

TEST(MODULE, ProducerDeclaredFileSource) {
  const auto path = testSource("test-JsonCodec-MyStruct.json");

  string input;
  {
    nio::FileSource in(path);
    input = in.readString();
  }

  nio::FileSource in(path);

  const auto result = jsonCodec.decode<MyStruct>(in);
  if (not result) {
    const DecodeError& err = result.error();
    namespace loc = rocket::str::location;
    const loc::Position pos {
      .type=loc::error, .position=err.position.value(), .ranges=err.ranges, .message=err.message
    };
    const auto result = loc::locations(input, { pos }, { .setLineString=true, .source=path.string() });
    loc::printLocations(nio::out, input, result, { .styled=true });
    throw InvalidState("cannot decode `MyStruct`");
  }

  const MyStruct& val = result.value();
  EXPECT_EQ(val.ärger, 16);
  EXPECT_EQ(val.ökonom, true);
  EXPECT_EQ(val.übermut, "a test\nstring");
  EXPECT_EQ(val.vec, (vector<i32> { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }));
}

TEST(MODULE, ProducerInstance) {
  using type = reflect::Instance<MyStruct, MyStruct::Three>;
  const MyStruct val { 42, true, "hello" };
  EXPECT_EQ(
    (decode<type>("  { \"ärger\":  42, \"ökonom\": true, \"übermut\": \"hello\",  }   ")),
    (type(val)));
}

TEST(MODULE, ProducerCodePoint) {
  using type = unicode::CodePoint;
  EXPECT_EQ(decode<type>("\"a\""), type('a'));
  EXPECT_EQ(decode<type>("\"€\""), type(U'€'));
  EXPECT_EQ(decode<type>("\"U+0061\""), type('a'));
  EXPECT_EQ(decode<type>("\"U+20AC\""), type(U'€'));
  EXPECT_EQ(decode<type>("\"U+10FFF\""), type(U'\U00010FFF'));
  EXPECT_EQ(decode<type>("\"U+10FFFF\""), type(U'\U0010FFFF'));
}

// EOF
