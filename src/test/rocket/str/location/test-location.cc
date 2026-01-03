/*
 * test-lopcation.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/str/location/location.h"
#include "rocket/unicode/unicode.h"

#include "rocket-gtest/matcher/matcher.h"

#include <fstream>

using namespace rocket;
using namespace rocket::gtest;
using namespace rocket::gtest::matcher;
using namespace rocket::str::location;
using namespace std;
using namespace testing;

// Macros ---------------------------------------------------------------------------------------------------

#define EXPECT_LOCATION( \
    loc, type__, position__, ranges__, line__, column__, lineRange__, lineString__, message__, caption__) \
    EXPECT_EQ(loc.type, type__); \
    EXPECT_EQ(loc.position, position__); \
    EXPECT_EQ(loc.ranges, (str::Ranges ranges__)); \
    EXPECT_EQ(loc.line, line__); \
    EXPECT_EQ(loc.column, column__); \
    EXPECT_EQ(loc.lineRange, (str::Range lineRange__)); \
    EXPECT_EQ(loc.lineString, lineString__); \
    EXPECT_EQ(loc.message, message__); \
    EXPECT_EQ(loc.caption, caption__)

// Local functions ------------------------------------------------------------------------------------------

namespace {

auto
positions(initializer_list<pair<size_t, size_t>> list) {
  return makeUnorderedBimap(list);
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(location, locations) {
  // Test empty input stream
  {
    nio::StringSource in("");
    Position pos { .type=Position::note, .position=0, .message="Oops" };
    auto result = locations(in, { pos });
    EXPECT_EQ(result.params.source, "(input)");
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::note, 0, ({}), 1, 1, ({ 0, 0 }), nullopt, "Oops", nullopt);
  }

  // Test EOF
  {
    string s = "a line without a line break at the end";
    nio::StringSource in(s);
    Position pos { .type=Position::error, .position=10, .message="Oops" };
    auto result = locations(in, { pos }, { .setLineString=true });
    EXPECT_EQ(result.params.source, "(input)");
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::error, 10, ({}), 1, 11, ({ 0, s.size() }), s, "Oops", nullopt);
  }

  // Test LF line break
  {
    string s = "a line with an line break at the end\n";
    nio::StringSource in(s);
    Position pos { .type=Position::error, .position=10, .message="Oops" };
    auto result = locations(in, { pos }, { .setLineString=true });
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(
        loc,
        Position::error,
        10, ({}),
        1, 11,
        ({ 0, s.size() - 1 }),
        s.substr(0, s.size() - 1),
        "Oops",
        nullopt);
  }

  // Test CRLF line break
  {
    string s = "a line with a line break at the end\r\n";
    nio::StringSource in(s);
    Position pos { .type=Position::error, .position=10, .message="Oops" };
    auto result = locations(in, { pos }, { .setLineString=true });
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(
        loc,
        Position::error,
        10, ({}),
        1, 11,
        ({ 0, s.size() - 2 }),
        s.substr(0, s.size() - 2),
        "Oops",
        nullopt);
  }

  // Test null byte
  {
    string s = "a\x00" "b"s;
    nio::StringSource in(s);
    Position pos { .type=Position::error, .position=3, .message="Oops" };
    auto result = locations(in, { pos }, { .setLineString=true });
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::error, 3, ({}), 1, 3, ({ 0, 3 }), s, "Oops", nullopt);
  }

  // Test multi-byte code points and line break
  {
    string s = "ä€\n€";
    nio::StringSource in(s);

    {
      Position pos { .type=Position::error, .position=2, .message="Oops" };
      in.seek(0);
      auto result = locations(in, { pos }, { .setLineString=true });
      EXPECT_EQ(result.locations.size(), 1);
      auto& loc = result.locations[0];
      EXPECT_LOCATION(loc, Position::error, 2, ({}), 1, 2, ({ 0, 5 }), "ä€", "Oops", nullopt);
    }

    {
      Position pos { .type=Position::error, .position=6, .message="Oops" };
      in.seek(0);
      auto result = locations(in, { pos }, { .setLineString=true });
      EXPECT_EQ(result.locations.size(), 1);
      auto& loc = result.locations[0];
      EXPECT_LOCATION(loc, Position::error, 6, ({}), 2, 1, ({ 6, 9 }), "€", "Oops", nullopt);
    }
  }

  // Test a somewhat larger file
  {
    string source = "src/test/rocket/str/location/test-location-Kafka.txt";
    nio::FileSource in(source);
    string content = in.Source::read();
    in.seek(0);

    Position pos { .type=Position::error, .position=3'000, .message="Oops" };
    auto result = locations(in, { pos }, { .setLineString=true, .source=source });
    EXPECT_EQ(result.params.source, source);
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(
        loc,
        Position::error,
        3'000, ({}),
        38, 77,
        ({ 2'922, 3'033 }),
        "blödsinnig. Der Mensch muß seinen Schlaf haben. Andere Reisende leben wie Haremsfrauen. Wenn ich zum Beispiel",
        "Oops",
        nullopt);

    string_view line(content.begin() + loc.lineRange.lower, content.begin() + *loc.lineRange.upper);
    EXPECT_EQ(line, loc.lineString);
  }

  // Test multi-code-point grapheme
  {
    // 🧑‍🌾: U+1F9D1, U+200D, U+1F33E, 4 + 3 + 4 = 11 bytes
    nio::StringSource in("🧑‍🌾");

    // No grapheme boundary at position 4
    EXPECT_THAT(
      ([&] {
        Position pos { .type=Position::error, .position=4, .message="Oops" };
        in.seek(0);
        locations(in, { pos }, {});
      }),
      ThrowsMessage<InvalidState>(HasSubstr("Position 4 not found in source")));

    // No grapheme boundary at position 7
    EXPECT_THAT(
      ([&] {
        Position pos { .type=Position::error, .position=7, .message="Oops" };
        in.seek(0);
        locations(in, { pos }, {});
      }),
      ThrowsMessage<InvalidState>(HasSubstr("Position 7 not found in source")));

    // Position 11 is okay
    {
      Position pos { .type=Position::error, .position=11, .message="Oops" };
      in.seek(0);
      auto result = locations(in, { pos }, { .setLineString=true });
      EXPECT_EQ(result.locations.size(), 1);
      auto& loc = result.locations[0];
      EXPECT_LOCATION(loc, Position::error, 11, ({}), 1, 3, ({ 0, 11 }), "🧑‍🌾", "Oops", nullopt);
    }
  }

  // Test incomplete UTF-8 byte sequence
  {
    u32string s32(100, U'€'); // A hundred times "€"
    string s8 = unicode::utf32To8(s32); // 300 bytes + 1 zero byte
    string_view s(s8.begin(), s8.end() - 1);
    nio::StringSource in(s);

    EXPECT_THAT(
      ([&] {
        Position pos { .type=Position::error, .position=1'000, .message="Oops" };
        in.seek(0);
        locations(in, { pos }, {});
      }),
      ThrowsMessage<InvalidState>(HasSubstr("Position 1000 not found in source")));
  }

  // Test tab size null
  {
    string s = "a\nb\tc"; // 0: a, 1: \n, 2: b, 3: \t, 4: c
    nio::StringSource in(s);
    // Position 4 points to `c` in the second line
    Position pos { .type=Position::note, .position=4, .message="Oops" };
    auto result = locations(in, { pos }, { .setLineString=true, .tabSize=nullopt });
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    // Grapheme width of '\t' is 0
    EXPECT_LOCATION(loc, Position::note, 4, ({}), 2, 2, ({ 2, 5 }), "b\tc", "Oops", nullopt);
  }

  // Test tab size 8
  {
    string s = "a\nb\tc"; // 0: a, 1: \n, 2: b, 3: \t, 4: c
    nio::StringSource in(s);
    // Position 4 points to `c` in the second line
    Position pos { .type=Position::note, .position=4, .message="Oops" };
    auto result = locations(in, { pos }, { .setLineString=true });
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::note, 4, ({}), 2, 9, ({ 2, 5 }), "b\tc", "Oops", nullopt);
  }
}

TEST(location, printLocations) {
  // Test multi-line input, more than one position per line
  {
    string s = "a multi-line\ntext, where the second line is somewhat longer";
    nio::StringSource in(s);
    Position pos0 { .type=Position::note, .position=2, .message="Oops1" };
    Position pos1 { .type=Position::warning, .position=13, .message="Oops2" };
    Position pos2 { .type=Position::error, .position=19, .message="Oops3", .caption="Watch out!" };
    auto result = locations(in, { pos0, pos1, pos2 }, { .setLineString=true, .source="foo" });
    EXPECT_EQ(result.params.source, "foo");
    EXPECT_EQ(result.locations.size(), 3);
    auto& loc0 = result.locations[0];
    EXPECT_LOCATION(
        loc0,
        Position::note,
        2, ({}),
        1, 3,
        ({ 0, 12 }),
        s.substr(0, 12),
        "Oops1",
        nullopt);
    auto& loc1 = result.locations[1];
    EXPECT_LOCATION(
        loc1,
        Position::warning,
        13, ({}),
        2, 1,
        ({ 13, s.size() }),
        s.substr(13),
        "Oops2",
        nullopt);
    auto& loc2 = result.locations[2];
    EXPECT_LOCATION(
        loc2,
        Position::error,
        19, ({}),
        2, 7,
        ({ 13, s.size() }),
        s.substr(13),
        "Oops3",
        "Watch out!");
    nio::StringSink out;
    printLocations(out, nullopt, result, {});
    EXPECT_EQ(out.str(),
        "foo:1:3: note: Oops1\n"
        "    1 | a multi-line\n"
        "      |   ^\n"
        "foo:2:1: warning: Oops2\n"
        "    2 | text, where the second line is somewhat longer\n"
        "      | ^\n"
        "foo:2:7: error: Oops3\n"
        "    2 | text, where the second line is somewhat longer\n"
        "      |       ^\n"
        "      |       Watch out!\n");
  }

  // Test multi-code-point grapheme
  {
    string s = "🧑‍🌾\x00\tabc"s;
    UnorderedBimap<size_t, size_t> grsp;
    auto grs = unicode::graphemes(s, &grsp);
    EXPECT_EQ(
        grsp,
        positions({ { 0, 0 }, { 1, 11 }, { 2, 12 }, { 3, 13 }, { 4, 14 }, { 5, 15 }, { 6, 16 } }));
    nio::StringSource in(s);
    Position pos { .type=Position::note, .position=13, .message="Oops" };
    auto result = locations(in, { pos }, {});
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::note, 13, ({}), 1, 9, ({ 0, 16 }), nullopt, "Oops", nullopt);
    string line = s.substr(loc.lineRange.lower, *loc.lineRange.upper - loc.lineRange.lower);
    EXPECT_EQ(line, s);
    nio::StringSink out;
    printLocations(out, s, result, {});
    EXPECT_EQ(out.str(),
        "(input):1:9: note: Oops\n"
        "    1 | 🧑‍🌾\\x00  abc\n"
        "      |         ^\n");
  }

  // Test multi-code-point grapheme, position at end of string
  {
    string s = "🧑‍🌾\x00\tabc"s;
    nio::StringSource in(s);
    Position pos { .type=Position::note, .position=16, .ranges={ { 0, 16 } }, .message="Oops" };
    auto result = locations(in, { pos }, {});
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    // Test a range that spans the entire line
    EXPECT_LOCATION(loc, Position::note, 16, ({ { 0, 16 } }), 1, 12, ({ 0, 16 }), nullopt, "Oops", nullopt);
    nio::StringSink out;
    printLocations(out, s, result, {});
    EXPECT_EQ(out.str(),
        "(input):1:12: note: Oops\n"
        "    1 | 🧑‍🌾\\x00  abc\n"
        "      | ~~~~~~~~~~~^\n");
  }
}

// EOF
