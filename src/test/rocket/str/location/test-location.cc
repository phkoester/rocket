/*
 * test-location.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/nio/nio.h"
#include "rocket/str/location/location.h"

using namespace rocket::str::location;

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

// #TEST ----------------------------------------------------------------------------------------------------

TEST(location, locationsEmptyInput) {
  const Position pos { .type=note, .position=0, .message="Oops" };
  const auto result = locations("", { pos });
  EXPECT_EQ(result.config.source, "(input)");
  EXPECT_EQ(result.locations.size(), 1);
  const auto& loc = result.locations[0];
  EXPECT_LOCATION(loc, note, 0, ({}), 1, 1, ({ 0, 0 }), nullopt, "Oops", nullopt);
}

TEST(location, locationsEoi) {
  const string input = "a line without a line break at the end";
  const Position pos { .type=error, .position=10, .message="Oops" };
  const auto result = locations(input, { pos }, { .setLineString=true });
  EXPECT_EQ(result.config.source, "(input)");
  EXPECT_EQ(result.locations.size(), 1);
  const auto& loc = result.locations[0];
  EXPECT_LOCATION(loc, error, 10, ({}), 1, 11, ({ 0, input.size() }), input, "Oops", nullopt);
}

TEST(location, locationsLf) {
  const string input = "a line with an line break at the end\n";
  const Position pos { .type=error, .position=10, .message="Oops" };
  const auto result = locations(input, { pos }, { .setLineString=true });
  EXPECT_EQ(result.locations.size(), 1);
  const auto& loc = result.locations[0];
  EXPECT_LOCATION(
      loc,
      error,
      10, ({}),
      1, 11,
      ({ 0, input.size() - 1 }),
      input.substr(0, input.size() - 1),
      "Oops",
      nullopt);
}

TEST(location, locationsCrlf) {
  const string input = "a line with a line break at the end\r\n";
  const Position pos { .type=error, .position=10, .message="Oops" };
  const auto result = locations(input, { pos }, { .setLineString=true });
  EXPECT_EQ(result.locations.size(), 1);
  const auto& loc = result.locations[0];
  EXPECT_LOCATION(
      loc,
      error,
      10, ({}),
      1, 11,
      ({ 0, input.size() - 2 }),
      input.substr(0, input.size() - 2),
      "Oops",
      nullopt);
}

TEST(location, locationsNullByte) {
  const string input = "a\x00" "b"s;
  const Position pos { .type=error, .position=3, .message="Oops" };
  const auto result = locations(input, { pos }, { .setLineString=true });
  EXPECT_EQ(result.locations.size(), 1);
  const auto& loc = result.locations[0];
  EXPECT_LOCATION(loc, error, 3, ({}), 1, 3, ({ 0, 3 }), input, "Oops", nullopt);
}


TEST(location, locationsMultiByteCharactersAndLineBreak) {
  const string input = "ä€\n€";

  {
    const Position pos { .type=error, .position=2, .message="Oops" };
    const auto result = locations(input, { pos }, { .setLineString=true });
    EXPECT_EQ(result.locations.size(), 1);
    const auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, error, 2, ({}), 1, 2, ({ 0, 5 }), "ä€", "Oops", nullopt);
  }

  {
    const Position pos { .type=error, .position=6, .message="Oops" };
    const auto result = locations(input, { pos }, { .setLineString=true });
    EXPECT_EQ(result.locations.size(), 1);
    const auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, error, 6, ({}), 2, 1, ({ 6, 9 }), "€", "Oops", nullopt);
  }
}

TEST(location, locationsKafkaTxt) {
  const auto source = testSource("test-location-Kafka.txt").string();
  nio::FileSource in(source);
  const auto input = in.readString();

  const Position pos { .type=error, .position=3'000, .message="Oops" };
  const auto result = locations(input, { pos }, { .setLineString=true, .source=source });
  EXPECT_EQ(result.config.source, source);
  EXPECT_EQ(result.locations.size(), 1);
  const auto& loc = result.locations[0];
  EXPECT_LOCATION(
      loc,
      error,
      3'000, ({}),
      38, 77,
      ({ 2'922, 3'033 }),
      "blödsinnig. Der Mensch muß seinen Schlaf haben. Andere Reisende leben wie Haremsfrauen. Wenn ich zum "
      "Beispiel",
      "Oops",
      nullopt);

  const string_view line(input.begin() + loc.lineRange.a, input.begin() + *loc.lineRange.b); // NOLINT
  EXPECT_EQ(line, loc.lineString);
}

TEST(location, locationsMultiByteCharacter) {
  // 🧑‍🌾: U+1F9D1, U+200D, U+1F33E, 4 + 3 + 4 = 11 bytes
  const string input = "🧑‍🌾";

  // No character boundary at position 4
  EXPECT_THAT(
    ([&] {
      const Position pos { .type=error, .position=4, .message="Oops" };
      locations(input, { pos }, {});
    }),
    ThrowsMessage<InvalidState>(HasSubstr("Position 4 not found in source")));

  // No character boundary at position 7
  EXPECT_THAT(
    ([&] {
      const Position pos { .type=error, .position=7, .message="Oops" };
      locations(input, { pos }, {});
    }),
    ThrowsMessage<InvalidState>(HasSubstr("Position 7 not found in source")));

  // Position 11 is okay
  {
    const Position pos { .type=error, .position=11, .message="Oops" };
    const auto result = locations(input, { pos }, { .setLineString=true });
    EXPECT_EQ(result.locations.size(), 1);
    const auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, error, 11, ({}), 1, 3, ({ 0, 11 }), "🧑‍🌾", "Oops", nullopt);
  }
}

TEST(location, locationsTabSize0) {
  const string input = "a\nb\tc"; // 0: a, 1: \n, 2: b, 3: \t, 4: c
  // Position 4 points to `c` in the second line
  const Position pos { .type=note, .position=4, .message="Oops" };
  const auto result = locations(input, { pos }, { .setLineString=true, .tabSize=nullopt });
  EXPECT_EQ(result.locations.size(), 1);
  const auto& loc = result.locations[0];
  // Character width of '\t' is 0
  EXPECT_LOCATION(loc, note, 4, ({}), 2, 2, ({ 2, 5 }), "b\tc", "Oops", nullopt);
}


TEST(location, locationsTabSize8) {
  const string input = "a\nb\tc"; // 0: a, 1: \n, 2: b, 3: \t, 4: c
  // Position 4 points to `c` in the second line
  const Position pos { .type=note, .position=4, .message="Oops" };
  const auto result = locations(input, { pos }, { .setLineString=true });
  EXPECT_EQ(result.locations.size(), 1);
  const auto& loc = result.locations[0];
  EXPECT_LOCATION(loc, note, 4, ({}), 2, 9, ({ 2, 5 }), "b\tc", "Oops", nullopt);
}

TEST(location, printLocations) { // NOLINT(*-complexity)
  // Test multi-line input, more than one position per line
  {
    const string input = "a multi-line\ntext, where the second line is somewhat longer";
    const nio::StringSource in(input);
    const Position pos0 { .type=note, .position=2, .message="Oops1" };
    const Position pos1 { .type=warning, .position=13, .message="Oops2" };
    const Position pos2 { .type=error, .position=19, .message="Oops3", .caption="Watch out!" };
    const auto result = locations(input, { pos0, pos1, pos2 }, { .setLineString=true, .source="foo" });
    EXPECT_EQ(result.config.source, "foo");
    EXPECT_EQ(result.locations.size(), 3);
    const auto& loc0 = result.locations[0];
    EXPECT_LOCATION(
        loc0,
        note,
        2, ({}),
        1, 3,
        ({ 0, 12 }),
        input.substr(0, 12),
        "Oops1",
        nullopt);
    const auto& loc1 = result.locations[1];
    EXPECT_LOCATION(
        loc1,
        warning,
        13, ({}),
        2, 1,
        ({ 13, input.size() }),
        input.substr(13),
        "Oops2",
        nullopt);
    const auto& loc2 = result.locations[2];
    EXPECT_LOCATION(
        loc2,
        error,
        19, ({}),
        2, 7,
        ({ 13, input.size() }),
        input.substr(13),
        "Oops3",
        "Watch out!");
    nio::StringSink out;
    if (TEST_TERMINAL) {
      printLocations(nio::out, nullopt, result, { .styled=true });
    }
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

  // Test multi-code-point character
  {
    const string input = "🧑‍🌾\x00\tabc"s;
    const Position pos { .type=note, .position=13, .message="Oops" };
    const auto result = locations(input, { pos }, {});
    EXPECT_EQ(result.locations.size(), 1);
    const auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, note, 13, ({}), 1, 9, ({ 0, 16 }), nullopt, "Oops", nullopt);
    const string line = input.substr(loc.lineRange.a, *loc.lineRange.b - loc.lineRange.a);
    EXPECT_EQ(line, input);
    nio::StringSink out;
    if (TEST_TERMINAL) {
      printLocations(nio::out, input, result, { .styled=true });
    }
    printLocations(out, input, result, {});
    EXPECT_EQ(out.str(),
        "(input):1:9: note: Oops\n"
        "    1 | 🧑‍🌾\\x00  abc\n"
        "      |         ^\n");
  }

  // Test multi-code-point character, position at end of string
  {
    const string input = "🧑‍🌾\x00\tabc"s;
    const Position pos { .type=note, .position=16, .ranges={ { 0, 16 } }, .message="Oops" };
    const auto result = locations(input, { pos }, {});
    EXPECT_EQ(result.locations.size(), 1);
    const auto& loc = result.locations[0];
    // Test a range that spans the entire line
    EXPECT_LOCATION(loc, note, 16, ({ { 0, 16 } }), 1, 12, ({ 0, 16 }), nullopt, "Oops", nullopt);
    nio::StringSink out;
    if (TEST_TERMINAL) {
      printLocations(nio::out, input, result, { .styled=true });
    }
    printLocations(out, input, result, {});
    EXPECT_EQ(out.str(),
        "(input):1:12: note: Oops\n"
        "    1 | 🧑‍🌾\\x00  abc\n"
        "      | ~~~~~~~~~~~^\n");
  }
}

// EOF
