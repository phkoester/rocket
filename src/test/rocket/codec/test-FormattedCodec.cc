/*
 * test-FormattedCodec.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/Bimap-codec.h"
#include "rocket/scope.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/chrono/chrono.h"
#include "rocket/log/log.h"
#include "rocket/reflect/reflect.h"
#include "rocket/str/location/location.h"
#include "rocket/system/system.h"

#include <sstream>

using namespace rocket::codec;

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

const FormattedCodec formattedCodec;

// Local functions ------------------------------------------------------------------------------------------

template<typename T>
string
encode(const T& val, const FormattedConsumerConfig& config = {}) {
  nio::StringSink out;
  formattedCodec.encode(val, out, config);
  return out.str();
}

template<typename T>
[[nodiscard]] DecodeResult<T>
decode(string_view str) {
  nio::StringSource in(str);
  return formattedCodec.decode<T>(in);
}

template<typename T>
[[nodiscard]] DecodeResult<T>
decodeFromStream(const string& str) {
  istringstream is(str);
  nio::StreamSource in(is);
  return formattedCodec.decode<T>(in);
}

template<typename T>
[[nodiscard]] pair<DecodeResult<T>, u64>
decodeAndTell(string_view str) {
  nio::StringSource in(str);
  return { formattedCodec.decode<T>(in), in.tell() };
}

// `Character` has no default constructor, so `FormattedCodec::decode` cannot create one
template<typename T>
[[nodiscard]] DecodeResult<T>
decodeInto(T val, string_view str) {
  nio::StringSource in(str);
  return formattedCodec.Base::decode<T>(val, in);
}

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

// `TEST` ---------------------------------------------------------------------------------------------------

// `FormattedConsumer` ......................................................................................

TEST(FormattedCodec, ConsumerBool) {
  EXPECT_EQ(encode(false), "false");
  EXPECT_EQ(encode(true), "true");
}

TEST(FormattedCodec, ConsumerChar) {
  EXPECT_EQ(encode('\x7F'), "'\\x7F'");
  EXPECT_EQ(encode('\t'), "'\\t'");
  EXPECT_EQ(encode(U'€'), "'€'");
  EXPECT_EQ(encode(U'\u200B'), "'\\u200B'");
  EXPECT_EQ(encode(U'\U0010FFFF'), "'\\U0010FFFF'");
}

TEST(FormattedCodec, ConsumerEnum) {
  enum Color : u8 { Red, Green, Blue };
  EXPECT_THAT(
    [] { encode(Blue); },
    ThrowsMessage<InvalidState>(matchesRegex(".*Cannot format enum of type `.*Color`")));

  EXPECT_EQ(encode(log::LogLevel::info), "info");
  EXPECT_THAT(
    [&] { encode(static_cast<log::LogLevel>(-1)); }, // NOLINT
    ThrowsMessage<InvalidState>(EndsWith("Invalid `rocket::log::LogLevel` value 255")));
}

TEST(FormattedCodec, ConsumerIntegerI64) {
  EXPECT_EQ(encode(-42_i64), "-42");
  EXPECT_EQ(encode(+42_i64), "42");
}

TEST(FormattedCodec, ConsumerFloatF64) {
  using type = f64;
  using limits = numeric_limits<type>;

  EXPECT_EQ(encode(-.1_f64), "-0.1");
  EXPECT_EQ(encode(1._f64), "1");
  EXPECT_EQ(encode(-123.456_f64), "-123.456");
  EXPECT_EQ(encode(-limits::infinity()), "-∞");
  EXPECT_EQ(encode(limits::infinity()), "∞");
}

TEST(FormattedCodec, ConsumerPointer) {
  EXPECT_EQ(encode(reinterpret_cast<void*>(0)), "null");
  EXPECT_THAT(encode(reinterpret_cast<void*>(0x1234ABCD)), matchesRegex("0x[0-9A-F]+"));
}

TEST(FormattedCodec, ConsumerString) {
  EXPECT_EQ(encode("Hello"sv), "\"Hello\"");
  EXPECT_EQ(encode(U"Hello"sv), "\"Hello\"");
  EXPECT_EQ(encode("\x7f"sv), "\"\\x7F\"");
  EXPECT_EQ(encode(U"\u200B"sv), "\"\\u200B\"");
  EXPECT_EQ(encode(U"\U00010FFF"sv), "\"\\U00010FFF\"");
}

TEST(FormattedCodec, ConsumerOptional) {
  using type = optional<string>;

  const type val;
  EXPECT_EQ(encode(val), "null");
  EXPECT_EQ(encode<type>("Hello"), "\"Hello\"");
}

TEST(FormattedCodec, ConsumerTuplePair) {
  EXPECT_EQ(encode(make_pair("answer"sv, 42)), "(\"answer\", 42)");
  EXPECT_EQ(encode(make_pair("answer"sv, 42), { .indent=true }),
    "(\n"
    "  \"answer\",\n"
    "  42\n"
    ")");
}

TEST(FormattedCodec, ConsumerList) {
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

TEST(FormattedCodec, ConsumerSet) {
  EXPECT_EQ(encode(set<i32> {}), "{}");
  EXPECT_EQ(encode(set<i32> { 1, 2, 2, 3 }), "{1, 2, 3}");
}

TEST(FormattedCodec, ConsumerMap) {
  EXPECT_EQ(
    encode(map<string, i32> { { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } }),
    "{\"alpha\": 1, \"beta\": 2, \"gamma\": 3}");
}

TEST(FormattedCodec, ConsumerDuration) {
  using namespace std::chrono;

  EXPECT_EQ(encode(1ns), "1ns");
  EXPECT_EQ(encode(2us), "2µs");
  EXPECT_EQ(encode(3ms), "3ms");
  EXPECT_EQ(encode(4s), "4s");
  EXPECT_EQ(encode(5min), "5min");
  EXPECT_EQ(encode(-6h), "-6h");
  EXPECT_EQ(encode(days(7)), "7d");
  EXPECT_EQ(encode(weeks(8)), "8w");
  EXPECT_EQ(encode(months(9)), "9m");
  EXPECT_EQ(encode(years(10)), "10y");
}

TEST(FormattedCodec, ConsumerHourMinuteSecond) {
  using namespace std::chrono;

  EXPECT_EQ(encode(hh_mm_ss { 1h + 2min + 3s }), "01:02:03");
  EXPECT_EQ(encode(hh_mm_ss { 1h + 2min + 3s + 5ms }), "01:02:03.005");
  EXPECT_EQ(encode(hh_mm_ss { 111h + 2min + 3s + 123ms }), "111:02:03.123");
  EXPECT_EQ(encode(hh_mm_ss { -(111h + 2min + 3s + 123456us) }), "-111:02:03.123456");
  EXPECT_EQ(encode(hh_mm_ss { -(111h + 2min + 3s + 123456789ns) }), "-111:02:03.123456789");
  EXPECT_EQ(encode(hh_mm_ss<minutes> { -2min }), "-00:02:00");
}

TEST(FormattedCodec, ConsumerYearDate) {
  using namespace std::chrono;

  EXPECT_EQ(encode(year_month_day { 1970y, January, 2d }), "1970-01-02");
}

TEST(FormattedCodec, ConsumerTimeZone) {
  using namespace std::chrono;

  EXPECT_EQ(encode(rocket::chrono::locate_zone("Europe/Berlin")), "(Europe/Berlin)");
  EXPECT_EQ(encode(rocket::chrono::locate_zone("UTC")), "(Etc/UTC)");
}

TEST(FormattedCodec, ConsumerTime) {
  using namespace std::chrono;

  const auto* const regexS = R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)"; // Seconds
  const auto* const regexNs = R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6,9}Z)"; // Nanoseconds

  const auto now = rocket::chrono::now<system_clock>();
  EXPECT_THAT(encode(time_point_cast<seconds>(now)), matchesRegex(regexS));
  EXPECT_THAT(encode(now), matchesRegex(regexNs));
}

TEST(FormattedCodec, ConsumerZonedTime) {
  using namespace std::chrono;

  const auto* const regexS =
    R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(Z|[+-]\d{2}:\d{2}) \([^ ]+\))"; // Seconds
  const auto* const regexNs =
    R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6,9}(Z|[+-]\d{2}:\d{2}) \([^ ]+\))"; // Nanoseconds

  {
    // Current time zone
    const auto* current = rocket::chrono::current_zone();
    const auto now = rocket::chrono::now<system_clock>();
    EXPECT_THAT(encode(rocket::chrono::zoned_time(current, time_point_cast<seconds>(now))), matchesRegex(regexS));
    EXPECT_THAT(encode(rocket::chrono::zoned_time(current, now)), matchesRegex(regexNs));
  }

  {
    // Time zone UTC
    const auto* utc = rocket::chrono::locate_zone("UTC");
    const auto now = rocket::chrono::now<system_clock>();
    EXPECT_THAT(encode(rocket::chrono::zoned_time(utc, time_point_cast<seconds>(now))), matchesRegex(regexS));
    EXPECT_THAT(encode(rocket::chrono::zoned_time(utc, now)), matchesRegex(regexNs));
  }
}

TEST(FormattedCodec, ConsumerInterval) {
  EXPECT_EQ(encode(math::ClosedInterval<f32>(-4.2F, 4.2F)), "[-4.2,4.2]");
  EXPECT_EQ(encode(math::LeftOpenInterval<f32>(-4.2F, 4.2F)), "(-4.2,4.2]");
  EXPECT_EQ(encode(math::RightOpenInterval<f32>(-4.2F, 4.2F)), "[-4.2,4.2)");
  EXPECT_EQ(encode(math::RightOpenInterval<f32>(-4.2, nullopt)), "[-4.2,∞)");
  EXPECT_EQ(encode(math::OpenInterval<f32>()), "∅");
  EXPECT_EQ(encode(math::OpenInterval<f32>(nullopt, nullopt)), "(-∞,∞)");
}

TEST(FormattedCodec, ConsumerDeclared) {
  EXPECT_EQ(
    encode(MyStruct { 42, true, "hello", { 1, 2, 3 } }),
    "(ärger=42, ökonom=true, übermut=\"hello\", vec=[1, 2, 3])");
}

TEST(FormattedCodec, ConsumerInstance) {
  using type = reflect::Instance<MyStruct, MyStruct::Three>;
  EXPECT_EQ(
    encode(type(MyStruct { 42, true, "hello", { 1, 2, 3 } })),
    "(ärger=42, ökonom=true, übermut=\"hello\")");
}

TEST(FormattedCodec, ConsumerVarRef) {
  int a = 0, b = 1, c= 2;
  const auto& refs = ROCKET_REFLECT_VARS((a)(b)(c));
  EXPECT_EQ(encode(refs), "(a=0, b=1, c=2)");
  const auto& ref = std::get<1>(refs);
  EXPECT_EQ(encode(ref), "b=1");
}

TEST(FormattedCodec, ConsumerCodePoint) {
  using type = unicode::CodePoint;

  EXPECT_EQ(encode(type('a')), "U+0061");
  EXPECT_EQ(encode(type(U'€')), "U+20AC");
  EXPECT_EQ(encode(type(U'\U0010FFFF')), "U+10FFFF");
}

// `FormattedProducer` ......................................................................................

TEST(FormattedCodec, ProducerBool) {
  EXPECT_EQ(decode<bool>("// sup\nTRue"), true);
  EXPECT_EQ(decode<bool>("  /* comment\nanother line in the comment */\r\n# comment\n\ttRUe"), true);
  EXPECT_EQ(decode<bool>("\r\n  1"), true);
  EXPECT_EQ(decode<bool>("false"), false);
  EXPECT_EQ(decode<bool>("\r\nx").error().message, "Expected a boolean value");
}

TEST(FormattedCodec, ProducerChar) {
  EXPECT_EQ(decode<char>("'a'"), 'a');
  EXPECT_EQ(decode<char>("'\\''"), '\'');
  EXPECT_EQ(decode<char>("'\\t'"), '\t');
  EXPECT_EQ(decode<char>("  'ä'").error().message, "Invalid character literal");

  EXPECT_EQ(decode<char>("  '\\xFF'").error().message, "Invalid character literal");
}

TEST(FormattedCodec, ProducerChar32) {
  EXPECT_EQ(decode<char32>("'ä'"), U'ä');
  EXPECT_EQ(decode<char32>("'\u20ac'"), U'€');
  EXPECT_EQ(decode<char32>("'\U00010FFF'"), U'\U00010FFF');
  EXPECT_EQ(decode<char32>("'€'"), U'€');

  EXPECT_EQ(decode<char32>("  '\\x800D'").error().message, "Invalid character literal");
}

TEST(FormattedCodec, ProducerEnum) {
  enum Color : u8 { Red, Green, Blue };

  EXPECT_THAT(decode<Color>("2").error().message, matchesRegex("Cannot scan enum of type `.*Color`"));

  EXPECT_EQ(decode<log::LogLevel>("  info  "), log::LogLevel::info);
  EXPECT_EQ(
    decode<log::LogLevel>("bogus").error().message,
    "Invalid value for enum `rocket::log::LogLevel`");
}

TEST(FormattedCodec, ProducerInteger) {
  EXPECT_EQ(decode<i32>("-42"), -42);
  EXPECT_EQ(decode<i32>("0xABcd"), 0xABCD);
  EXPECT_EQ(decode<i32>("077"), 077);
  EXPECT_EQ(decode<i32>("0o123"), 0123);
  EXPECT_EQ(decode<i32>("0b1010101"), 0b1010101);

  EXPECT_EQ(decode<i32>("  x").error().message, "Expected an integer value");
}

TEST(FormattedCodec, ProducerFloat) {
  using type = f64;
  using limits = numeric_limits<type>;

  EXPECT_EQ(decodeAndTell<type>("  -123.456  "), make_pair(-123.456_f64, 10_u64));
  EXPECT_EQ(decode<type>("  -1e3"), -1e3_f64);
  EXPECT_EQ(decode<type>("-inf"), -limits::infinity());
  EXPECT_EQ(decode<type>("-∞"), -limits::infinity());
  EXPECT_EQ(decode<type>("inf"), limits::infinity());
  EXPECT_EQ(decode<type>("+inf"), limits::infinity());
  EXPECT_EQ(decode<type>("∞"), limits::infinity());

  EXPECT_TRUE(isnan(decode<type>("nan").value()));
}

TEST(FormattedCodec, ProducerPointer) {
  EXPECT_EQ(decodeAndTell<void*>("  null  "), make_pair(static_cast<void*>(0), 6_u64)); // NOLINT
  EXPECT_EQ(decodeAndTell<void*>("  0x12345678  "), make_pair(reinterpret_cast<void*>(0x12345678), 12_u64));
}

TEST(FormattedCodec, ProducerInvalidUtf8) {
  constexpr string_view MESSAGE = "Invalid UTF-8 byte sequence";

  EXPECT_EQ(decode<string>("\"a\xFF\"").error().message, MESSAGE);
  EXPECT_EQ(decode<string>("\"\xE2\x82\"").error().message, MESSAGE); // Truncated sequence
  EXPECT_EQ(decode<char>("'\xFF'").error().message, MESSAGE);
}

TEST(FormattedCodec, ProducerEscapeFailurePosition) {
  using Map = map<string, string>;
  constexpr string_view INVALID_ESCAPE = "Invalid escape sequence";

  expectFailure<string>("  \"\\q\"", INVALID_ESCAPE, 3, { { 3, 5 } });
  expectFailure<char>("  '\\q'", INVALID_ESCAPE, 3, { { 3, 5 } });
  expectFailure<Map>("{\n  \"a\": \"\\q\"\n}", INVALID_ESCAPE, 10, { { 10, 12 } }); // Value on a later line
  expectFailure<Map>("{\"\\q\": \"x\"}", INVALID_ESCAPE, 2, { { 2, 4 } }); // Key

  expectFailure<string>("  \"a\xFF\"", "Invalid UTF-8 byte sequence", 4);
}

TEST(FormattedCodec, ProducerFailureInsideCluster) {
  using Pair = pair<i32, i32>;

  // A digit and U+0301 (COMBINING ACUTE ACCENT) form one cluster, so these failures lie inside it. The
  // reported positions are the starts of the clusters
  expectFailure<Pair>("(1́, 2)", "Expected ','", 1);
  expectFailure<Pair>("(1, 2́", "Unterminated tuple", 4, { { 0, 4 } });

  // A failure at a cluster boundary stays where it is
  expectFailure<Pair>("(1; 2)", "Expected ','", 2);

  // The same goes for a source that is not contiguous
  const auto fromStream = decodeFromStream<Pair>("(1́, 2)");
  ASSERT_FALSE(fromStream.has_value());
  EXPECT_EQ(fromStream.error().position, 1U);

  // The source stays where the failure occurred
  EXPECT_EQ(decodeAndTell<Pair>("(1́, 2)").second, 2U);

  // The position is now a character boundary, so `locations` accepts it
  namespace loc = rocket::str::location;
  const string input = "(1́, 2)";
  const auto result = decode<Pair>(input);
  ASSERT_FALSE(result.has_value());
  const loc::Position pos {
    .type=loc::error, .position=result.error().position, .message=result.error().message
  };
  EXPECT_EQ(loc::locations(input, { pos }).locations.at(0).column, 2U);
}

TEST(FormattedCodec, ProducerCodePointInvalid) {
  using CodePoint = unicode::CodePoint;

  expectFailure<CodePoint>("  U+110000", "Invalid code-point value 0x110000", 2);
  expectFailure<CodePoint>("  U+D800", "Invalid code-point value 0xD800", 2);
  expectFailure<pair<i32, CodePoint>>("(1, U+110000)", "Invalid code-point value 0x110000", 4);
}

TEST(FormattedCodec, ProducerCharacter) {
  const unicode::Character<char> character("x"s);
  const unicode::CharacterView<char> view("x"sv);

  EXPECT_EQ(static_cast<string_view>(decodeInto(character, "\"a\"").value()), "a");
  EXPECT_EQ(static_cast<string_view>(decodeInto(view, "\"a\"").value()), "a");

  const auto empty = decodeInto(character, "  \"\"");
  EXPECT_EQ(empty.error().message, "Expected a character");
  EXPECT_EQ(empty.error().position, 2U);

  const auto emptyView = decodeInto(view, "\"\"");
  EXPECT_EQ(emptyView.error().message, "Expected a character");
  EXPECT_EQ(emptyView.error().position, 0U);
}

TEST(FormattedCodec, ProducerOptionalString) {
  using type = optional<string>;
  EXPECT_EQ(decode<type>("  null  "), nullopt);
}

TEST(FormattedCodec, ProducerOptionalStringView) {
  nio::StringSource in("\"Hello\""); // The source must remain valid for the string view
  EXPECT_EQ(formattedCodec.decode<optional<basic_string_view<char32>>>(in), U"Hello"sv);
}

TEST(FormattedCodec, ProducerTuple) {
  EXPECT_EQ((decode<pair<string, i32>>("  (  \"answer\"   , 42   )")), make_pair("answer"s, 42_i32));
  EXPECT_EQ(
    (decode<pair<string, i32>>("  (  # comment\n \"answer\"   , 42  /* comment */  , // comment\n  )")),
    make_pair("answer"s, 42_i32));

  EXPECT_EQ((decode<tuple<i32, i32, bool>>("(1, 2, true)")), make_tuple(1_i32, 2_i32, true));
}

TEST(FormattedCodec, ProducerList) {
  EXPECT_EQ((decode<array<i32, 3>>("  [ 1, 2, 3   ]")), (array<i32, 3> { 1, 2, 3 }));
  EXPECT_EQ((decode<array<i32, 3>>("  [ 1, 2, 3   , ]")), (array<i32, 3> { 1, 2, 3 }));
  EXPECT_EQ((decode<list<i32>>("  [ 1, 2, 3   , ]")), (list<i32> { 1, 2, 3 }));
  EXPECT_EQ((decode<vector<i32>>("[]")), (vector<i32> {}));
  EXPECT_EQ((decode<vector<i32>>("[1, 2, 3]")), (vector<i32> { 1, 2, 3 }));
  EXPECT_EQ((decode<vector<i32>>("[1, 2, 3,]")), (vector<i32> { 1, 2, 3 }));
  EXPECT_EQ((decode<vector<i32>>("  [ 1, 2, 3   , ]")), (vector<i32> { 1, 2, 3 }));
}

TEST(FormattedCodec, ProducerSet) {
  EXPECT_EQ((decode<set<i32>>("  { 3, 2, 1  ,  }   ")), (set<i32> { 1, 2, 3 }));
}

TEST(FormattedCodec, ProducerMap) {
  using type = std::map<i32, string>;

  // Test last one wins
  EXPECT_EQ(
    decode<type>("{ 1: \"alpha\", 2: \"beta\", 3: \"gämmä\", 3: \"gamma\" }"),
    (type { { 1, "alpha" }, { 2, "beta" }, { 3, "gamma" } }));
}

TEST(FormattedCodec, ProducerBimap) {
  using type = Bimap<string, i32>;
  // Test last one wins
  EXPECT_EQ(
    (decode<type>("  { \"alpha\"\t: 1, \"beta\"  :/* comment */ 2, \"gamma\": 4, \"gamma\": 3  ,  }   ")),
    (makeBimap<string, i32>({ { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } })));
}

TEST(FormattedCodec, ProducerBimapUnordered) {
  using type = UnorderedBimap<string, i32>;
  // Test last one wins
  EXPECT_EQ(
    (decode<type>("  { \"alpha\"\t: 1, \"beta\"  :/* comment */ 2, \"gamma\": 4, \"gamma\": 3  ,  }   ")),
    (makeUnorderedBimap<string, i32>({ { "alpha", 1 }, { "beta", 2 }, { "gamma", 3 } })));
}

TEST(FormattedCodec, ProducerDuration) {
  using namespace std::chrono;

  EXPECT_EQ(decode<milliseconds>("1s"), 1000ms);
  EXPECT_EQ(decode<milliseconds>("1s"), 1s);
  EXPECT_EQ(decode<milliseconds>("-5s"), -5s);

  EXPECT_EQ(decode<seconds>("1000ms"), 1s);

  EXPECT_EQ(decode<seconds>("x").error().message, "Expected a duration");
  EXPECT_EQ(decode<seconds>("10x").error().message, "Expected a time unit");

  // Range checks: 292 years fit into `nanoseconds`, 300 years do not
  EXPECT_EQ(decode<nanoseconds>("292y"), duration_cast<nanoseconds>(years(292)));
  EXPECT_EQ(decode<nanoseconds>("9223372036854775807ns"), nanoseconds::max());
  EXPECT_EQ(decode<nanoseconds>("-9223372036854775808ns"), nanoseconds::min());
  expectFailure<nanoseconds>("  300y", "Duration out of range", 2);
  expectFailure<nanoseconds>("  -300y", "Duration out of range", 2);
  expectFailure<duration<i32, std::milli>>("  3000h", "Duration out of range", 2);

  // A floating-point representation cannot overflow
  EXPECT_TRUE((decode<duration<f32, std::nano>>("300y")));
}

TEST(FormattedCodec, ProducerHourMinuteSecond) {
  using namespace std::chrono;

  EXPECT_EQ(
    decode<hh_mm_ss<seconds>>("01:02:03").value().to_duration(),
    (hh_mm_ss { 1h + 2min + 3s }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<seconds>>("01:02:03.123").value().to_duration(),
    (hh_mm_ss { 1h + 2min + 3s }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<milliseconds>>("01:02:03.123456").value().to_duration(),
    (hh_mm_ss { 1h + 2min + 3s + 123ms }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<microseconds>>("-111:02:03.123456789").value().to_duration(),
    (hh_mm_ss { -(111h + 2min + 3s + 123456us) }).to_duration());
  EXPECT_EQ(
    decode<hh_mm_ss<nanoseconds>>("-111:02:03.123456789").value().to_duration(),
    (hh_mm_ss { -(111h + 2min + 3s + 123456789ns) }).to_duration());
  EXPECT_EQ(decode<hh_mm_ss<seconds>>("-00:02:00").value().to_duration(), -2min);
  EXPECT_EQ(decode<hh_mm_ss<seconds>>(" -00:02:00").value().to_duration(), -2min);
  EXPECT_EQ(decode<hh_mm_ss<seconds>>(" -01:02:03").value().to_duration(), -(1h + 2min + 3s));

  EXPECT_EQ(decode<hh_mm_ss<milliseconds>>("x").error().message, "Expected an hour, minute, and second");
  EXPECT_EQ(decode<hh_mm_ss<milliseconds>>("01:02:").error().message, "Expected an hour, minute, and second");
  EXPECT_EQ(decode<hh_mm_ss<milliseconds>>("01:02:03.").error().message, "Expected subseconds");
}

TEST(FormattedCodec, ProducerHourMinuteSecondInvalid) {
  using namespace std::chrono;
  constexpr string_view INVALID = "Invalid hour, minute, or second";

  expectFailure<hh_mm_ss<seconds>>("01:60:00", INVALID, 0);
  expectFailure<hh_mm_ss<seconds>>("01:00:60", INVALID, 0);
  expectFailure<hh_mm_ss<seconds>>("01:-1:00", INVALID, 0);
  expectFailure<hh_mm_ss<seconds>>("01:00:-1", INVALID, 0);
  expectFailure<hh_mm_ss<seconds>>("  01:99:99", INVALID, 2);

  // The sum of all parts would not fit into nanoseconds
  expectFailure<hh_mm_ss<seconds>>("2562047:00:00", INVALID, 0);
  expectFailure<hh_mm_ss<seconds>>("-2562047:00:00", INVALID, 0);

  EXPECT_EQ(decode<hh_mm_ss<seconds>>("01:59:59").value().to_duration(), 1h + 59min + 59s);
  EXPECT_EQ(decode<hh_mm_ss<seconds>>("2562046:59:59").value().to_duration(), 2562046h + 59min + 59s);
}

TEST(FormattedCodec, ProducerDateInvalid) {
  using namespace std::chrono;
  constexpr string_view INVALID = "Invalid date";

  expectFailure<year_month_day>("2026-13-01", INVALID, 0);
  expectFailure<year_month_day>("2026-00-01", INVALID, 0);
  expectFailure<year_month_day>("2026-01-32", INVALID, 0);
  expectFailure<year_month_day>("2026-01-00", INVALID, 0);
  expectFailure<year_month_day>("2026-02-30", INVALID, 0);
  expectFailure<year_month_day>("2025-02-29", INVALID, 0); // No leap year
  expectFailure<year_month_day>("  2026-13-45", INVALID, 2);

  // These would be truncated to valid values
  expectFailure<year_month_day>("2026-257-01", INVALID, 0);
  expectFailure<year_month_day>("2026-01-257", INVALID, 0);
  expectFailure<year_month_day>("70000-01-01", INVALID, 0);

  EXPECT_EQ(decode<year_month_day>("2024-02-29"), (year_month_day { 2024y, February, 29d }));
}

TEST(FormattedCodec, ProducerDate) {
  using namespace std::chrono;

  EXPECT_EQ(decode<year_month_day>("1970-01-02"), (year_month_day { 1970y, January, 2d }));
  EXPECT_EQ(decode<year_month_day>("-100-01-02"), (year_month_day { -100y, January, 2d }));

  EXPECT_EQ(decode<year_month_day>("x").error().message, "Expected a date");
}

TEST(FormattedCodec, ProducerTimeZone) {
  using namespace std::chrono;

  EXPECT_EQ(decode<TimeZone>("(Europe/Berlin)"), rocket::chrono::locate_zone("Europe/Berlin"));
  EXPECT_EQ(decode<TimeZone>("(UTC)"), rocket::chrono::locate_zone("UTC"));
  EXPECT_EQ(decode<TimeZone>("x").error().message, "Expected a time zone");

  EXPECT_THAT(decode<TimeZone>("(x)").error().message, AnyOf(
    EndsWith("cannot locate zone: x"), // Linux, macOS
    StrEq("unable to locate time_zone with given name"))); // Windows
}

TEST(FormattedCodec, ProducerTime) {
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

TEST(FormattedCodec, ProducerZonedTime) {
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

TEST(FormattedCodec, ProducerZonedTimeUtcOffset) {
  using namespace std::chrono;
  using ZonedTime = rocket::chrono::zoned_time<nanoseconds>;
  constexpr string_view INVALID = "Invalid UTC offset";

  expectFailure<ZonedTime>("2026-10-07T11:21:00+24:00 (Europe/Berlin)", INVALID, 19);
  expectFailure<ZonedTime>("2026-10-07T11:21:00-24:00 (Europe/Berlin)", INVALID, 19);
  expectFailure<ZonedTime>("2026-10-07T11:21:00+01:60 (Europe/Berlin)", INVALID, 19);
  expectFailure<ZonedTime>("2026-10-07T11:21:00+99:99 (Europe/Berlin)", INVALID, 19);
  expectFailure<ZonedTime>("  2026-10-07T11:21:00+24:00 (Europe/Berlin)", INVALID, 21);
  expectFailure<ZonedTime>("2026-10-07T11:21:00.5+24:00 (Europe/Berlin)", INVALID, 21);

  EXPECT_TRUE(decode<ZonedTime>("2026-10-07T11:21:00+23:59 (Europe/Berlin)").has_value());
  EXPECT_TRUE(decode<ZonedTime>("2026-10-07T11:21:00-23:59 (Europe/Berlin)").has_value());
  EXPECT_TRUE(decode<ZonedTime>("2026-10-07T11:21:00+00:00 (Europe/Berlin)").has_value());
}

TEST(FormattedCodec, ProducerTimeIndependentOfLocalZone) {
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

    expectTime("2026-07-15T12:00:00Z", summer + 12h);
    expectTime("2026-07-15T12:00:00.25Z", summer + 12h + 250ms);

    // As local times in Europe/Berlin, these do not exist and exist twice, respectively
    expectTime("2026-03-29T02:30:00Z", spring + 2h + 30min);
    expectTime("2026-10-25T02:30:00Z", autumn + 2h + 30min);

    expectZoned("2026-07-15T14:00:00+02:00 (Europe/Berlin)", summer + 12h);
    expectZoned("2026-03-29T03:30:00+02:00 (Europe/Berlin)", spring + 1h + 30min);
    expectZoned("2026-10-25T02:30:00+01:00 (Europe/Berlin)", autumn + 1h + 30min);
    expectZoned("2026-07-15T12:00:00Z (UTC)", summer + 12h);
  }

  // `std::mktime` would have moved this to March 2
  expectFailure<TimePoint>("2026-02-30T12:00:00Z", "Expected a time point", 0);
}

TEST(FormattedCodec, ProducerInterval) {
  using namespace rocket::math;
  EXPECT_EQ(decode<ClosedInterval<f64>>("[-4.2,4.2]"), ClosedInterval<f64>(-4.2_f64, 4.2_f64));
  EXPECT_EQ(decode<OpenInterval<f64>>("∅"), OpenInterval<f64>());
  EXPECT_EQ(
    decode<OpenInterval<i64>>("  (  -∞ /* Comment */, ∞)  // Comment"),
    OpenInterval<i64>(nullopt, nullopt));
}

TEST(FormattedCodec, ProducerDeclared) {
  // Test last one wins
  EXPECT_EQ(
    (decode<MyStruct>("  ( ärger  =  42, vec=[7, 8], ökonom=true, übermut=\"hello\", vec=[1, 2, 3] )   ")),
    (MyStruct { 42, true, "hello", { 1, 2, 3 } }));

  // Members in a different order
  EXPECT_EQ(
    (decode<MyStruct>("(vec=[1, 2, 3], übermut=\"hello\", ärger=42, ökonom=true)")),
    (MyStruct { 42, true, "hello", { 1, 2, 3 } }));

  // Missing members keep their default values
  EXPECT_EQ((decode<MyStruct>("(ökonom=true, vec=[1, 2, 3])")), (MyStruct { 0, true, "", { 1, 2, 3 } }));
  EXPECT_EQ((decode<MyStruct>("()")), (MyStruct {}));

  // Whitespace and comments may separate the name from '='
  EXPECT_EQ((decode<MyStruct>("(ärger /* c */ = 42, ökonom=true)")), (MyStruct { 42, true, "", {} }));

  // Unknown members are an error
  EXPECT_EQ(decode<MyStruct>("(ärger=42, bogus=1)").error().message, "Unknown member `bogus`");
  EXPECT_EQ(decode<MyStruct>("(1=2)").error().message, "Expected a member reference");
  EXPECT_EQ(decode<MyStruct>("(_bogus=1)").error().message, "Unknown member `_bogus`");
  EXPECT_EQ(decode<MyStruct>("($bogus=1)").error().message, "Unknown member `$bogus`");
  EXPECT_EQ(decode<MyStruct>("(foo$bar=1)").error().message, "Unknown member `foo$bar`");
}

TEST(FormattedCodec, ProducerDeclaredFileSource) {
  const auto path = testSource("test-FormattedCodec-MyStruct.ron");

  string input;
  {
    nio::FileSource in(path);
    input = in.readString();
  }

  nio::FileSource in(path);

  const auto result = formattedCodec.decode<MyStruct>(in);
  if (not result) {
    const DecodeError& err = result.error();
    namespace loc = rocket::str::location;
    const loc::Position pos {
      .type=loc::error, .position=err.position, .ranges=err.ranges, .message=err.message
    };
    const auto result = loc::locations(input, { pos }, { .setLineString=true, .source=path.string() });
    loc::printLocations(nio::out, input, result, { .styled=true });
    throw InvalidState("Cannot decode `MyStruct`");
  }

  const MyStruct& val = result.value();
  EXPECT_EQ(val.ärger, 16);
  EXPECT_EQ(val.ökonom, true);
  EXPECT_EQ(val.übermut, "a test\nstring");
  EXPECT_EQ(val.vec, (vector<i32> { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }));
}

TEST(FormattedCodec, ProducerInstance) {
  using type = reflect::Instance<MyStruct, MyStruct::Three>;
  const MyStruct val { 42, true, "hello" };
  EXPECT_EQ(
    (decode<type>("  ( ärger  =  42, ökonom=true, übermut=\"hello\",  )   ")),
    (type(val)));
}

TEST(FormattedCodec, ProducerCodePoint) {
  using type = unicode::CodePoint;
  EXPECT_EQ(decode<type>("'a',"), type('a'));
  EXPECT_EQ(decode<type>("'€'"), type(U'€'));
  EXPECT_EQ(decode<type>("U+0061,"), type('a'));
  EXPECT_EQ(decode<type>("U+20AC"), type(U'€'));
  EXPECT_EQ(decode<type>("U+10FFF"), type(U'\U00010FFF'));
  EXPECT_EQ(decode<type>("U+10FFFF"), type(U'\U0010FFFF'));
}

// EOF
