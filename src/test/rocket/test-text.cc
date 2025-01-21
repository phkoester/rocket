/*
 * test-text.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-boost-decl.h"
#include "rocket/codec-rocket-decl.h"
#include "rocket/codec-std-decl.h"
#include "rocket/codec-boost.h"
#include "rocket/codec-rocket.h"
#include "rocket/codec-std.h"

#include "rocket/except.h"
#include "rocket/text.h"
#include "rocket/unicode.h"
#include "rocket/reflect.h"

#include "rocket-gtest/match.h"
#include "rocket-gtest/print.h"

#include <fstream>

using namespace rocket;
using namespace rocket::gtest;
using namespace rocket::gtest::match;
using namespace rocket::text;
using namespace std;
using namespace testing;

// Macros ---------------------------------------------------------------------------------------------------

#define EXPECT_LOCATION( \
    loc, type__, position__, ranges__, line__, column__, lineRange__, lineString__, message__, caption__) \
    EXPECT_EQ(loc.type, type__); \
    EXPECT_EQ(loc.position, position__); \
    EXPECT_EQ(loc.ranges, (text::Ranges ranges__)); \
    EXPECT_EQ(loc.line, line__); \
    EXPECT_EQ(loc.column, column__); \
    EXPECT_EQ(loc.lineRange, (text::Range lineRange__)); \
    EXPECT_EQ(loc.lineString, lineString__); \
    EXPECT_EQ(loc.message, message__); \
    EXPECT_EQ(loc.caption, caption__)

// `Config` -------------------------------------------------------------------------------------------------

struct Config {
  vector<string> modules;
  unordered_map<int, string> ports;
  string userName;

  ROCKET_REFLECT_MEMBERS(Config, index, (modules)(ports)(userName));
};

ROCKET_REFLECT_MEMBERS_DEFINE_FN_PARSE_RON(Config, index);

// Local functions ------------------------------------------------------------------------------------------

namespace {

vector<vector<string>>
pars(const vector<vector<string>>& v) {
  return v;
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(text, internalParagraphs) {
  string nbsp = "\u00a0";

  EXPECT_EQ(text::internal::paragraphs(""), (pars({{}})));
  EXPECT_EQ(text::internal::paragraphs("a b"), (pars({{ "a", "b" }})));
  EXPECT_EQ(
      text::internal::paragraphs(nbsp + "a" + nbsp + nbsp + "b cd e" + nbsp + "f"),
      (pars({{ "a b", "cd", "e f" }})));
  EXPECT_EQ(text::internal::paragraphs("a \t\t b"), (pars({{ "a", "b" }})));
  EXPECT_EQ(text::internal::paragraphs("a\nb c"), (pars({{"a"}, { "b", "c" }})));
  EXPECT_EQ(text::internal::paragraphs("a\r\nb c"), (pars({{"a"}, { "b", "c" }})));
  EXPECT_EQ(text::internal::paragraphs("a\n\nb"), (pars({{"a"}, {}, { "b" }})));
  EXPECT_EQ(text::internal::paragraphs("\na"), (pars({{}, {"a"} })));
}

TEST(text, Range) {
  using R = text::Range;

  EXPECT_EQ((R { 1, 2 } & R { 0, 3 }), (R { 1, 2 }));
  EXPECT_EQ((R { 0, 6 } & R { 3, 9 }), (R { 3, 6 }));
  EXPECT_EQ((R { 3, 9 } & R { 0, 6 }), (R { 3, 6 }));
}

TEST(text, locations) {
  // Test empty input stream
  {
    auto is = io::is();
    Position pos { .type=Position::note, .position=0, .message="Oops" }; 
    auto result = locations(is, { pos });
    EXPECT_EQ(result.params.source, "(input)");
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::note, 0, ({}), 1, 1, ({ 0, 0 }), nullopt, "Oops", nullopt);
  }

  // Test EOF
  {
    string s = "a line without a line break at the end";
    auto is = io::is(s);
    Position pos { .type=Position::error, .position=10, .message="Oops" };
    auto result = locations(is, { pos }, { .setLineString=true });
    EXPECT_EQ(result.params.source, "(input)");
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::error, 10, ({}), 1, 11, ({ 0, s.size() }), s, "Oops", nullopt);
  }

  // Test LF line break
  {
    string s = "a line with an line break at the end\n";
    auto is = io::is(s);
    Position pos { .type=Position::error, .position=10, .message="Oops" };
    auto result = locations(is, { pos }, { .setLineString=true });
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
    auto is = io::is(s);
    Position pos { .type=Position::error, .position=10, .message="Oops" };
    auto result = locations(is, { pos }, { .setLineString=true });
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
    auto is = io::is(s);
    Position pos { .type=Position::error, .position=3, .message="Oops" };
    auto result = locations(is, { pos }, { .setLineString=true });
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::error, 3, ({}), 1, 3, ({ 0, 3 }), s, "Oops", nullopt);
  }

  // Test multi-byte code points and line break
  {
    string s = "ä€\n€";
    auto is = io::is(s);
    
    {
      Position pos { .type=Position::error, .position=2, .message="Oops" };
      auto result = locations(io::resetg(is), { pos }, { .setLineString=true });
      EXPECT_EQ(result.locations.size(), 1);
      auto& loc = result.locations[0];
      EXPECT_LOCATION(loc, Position::error, 2, ({}), 1, 2, ({ 0, 5 }), "ä€", "Oops", nullopt);
    }

    {
      Position pos { .type=Position::error, .position=6, .message="Oops" };
      auto result = locations(io::resetg(is), { pos }, { .setLineString=true });
      EXPECT_EQ(result.locations.size(), 1);
      auto& loc = result.locations[0];
      EXPECT_LOCATION(loc, Position::error, 6, ({}), 2, 1, ({ 6, 9 }), "€", "Oops", nullopt);
    }
  }

  // Test a somewhat larger file with a small buffer size
  {
    string source = "src/test/rocket/test-text-Kafka.txt";
    string content;
    content << ifstream(source);
    auto is = io::is(content);

    Position pos { .type=Position::error, .position=3'000, .message="Oops" };
    auto result = locations(
        is, { pos }, { .bufferSize=io::MIN_BUFFER_SIZE, .setLineString=true, .source=source });
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
    auto is = io::is("🧑‍🌾");
    
    // No grapheme boundary at position 4
    EXPECT_THAT(
      ([&] {
        Position pos { .type=Position::error, .position=4, .message="Oops" };
        locations(io::resetg(is), { pos }, {});
      }),
      ThrowsMessage<except::InvalidState>(HasSubstr("Position 4 not found in input stream")));
    
    // No grapheme boundary at position 7
    EXPECT_THAT(
      ([&] {
        Position pos { .type=Position::error, .position=7, .message="Oops" };
        locations(io::resetg(is), { pos }, {});
      }),
      ThrowsMessage<except::InvalidState>(HasSubstr("Position 7 not found in input stream")));

    // Position 11 is okay
    {
      Position pos { .type=Position::error, .position=11, .message="Oops" };
      auto result = locations(io::resetg(is), { pos }, { .setLineString=true });
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
    auto is = io::is(s);

    EXPECT_THAT(
      ([&] {
        Position pos { .type=Position::error, .position=1'000, .message="Oops" };
        locations(io::resetg(is), { pos }, {});
      }),
      throwsInputFailure<char>(297, HasSubstr("Incomplete UTF-8 byte sequence")));
  }

  // Test tab size null
  {
    string s = "a\nb\tc"; // 0: a, 1: \n, 2: b, 3: \t, 4: c
    auto is = io::is(s);
    // Position 4 points to `c` in the second line
    Position pos { .type=Position::note, .position=4, .message="Oops" };
    auto result = locations(is, { pos }, { .setLineString=true, .tabSize=nullopt });
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    // Grapheme width of '\t' is 0
    EXPECT_LOCATION(loc, Position::note, 4, ({}), 2, 2, ({ 2, 5 }), "b\tc", "Oops", nullopt);
  }

  // Test tab size 8
  {
    string s = "a\nb\tc"; // 0: a, 1: \n, 2: b, 3: \t, 4: c
    auto is = io::is(s);
    // Position 4 points to `c` in the second line
    Position pos { .type=Position::note, .position=4, .message="Oops" };
    auto result = locations(is, { pos }, { .setLineString=true });
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::note, 4, ({}), 2, 9, ({ 2, 5 }), "b\tc", "Oops", nullopt);
  }
}

TEST(text, printLocations) {
  // Test failing `parseRon`
  {
    string source = "src/test/rocket/test-text-Config.ron";
    string content;
    content << ifstream(source);
    auto is = io::is(content);

    Config config;
    try {
      parseRon(is, config);
      FAIL();
    } catch (except::ParseFailure<char>& ex) {
      // Test tab size null
      {
        Position pos {
          .type=Position::error,
          .position=ex.position(),
          .ranges=ex.ranges(),
          .message=ex.message(),
          .caption="Look out!"
        };
        auto result = locations(
            io::resetg(is), { pos }, { .setLineString=true, .source=source, .tabSize=nullopt });
        EXPECT_EQ(result.locations.size(), 1);
        ostringstream os;
        printLocations(os, nullopt, result);
        EXPECT_EQ(
            os.str(),
            "src/test/rocket/test-text-Config.ron:14:1: error: Invalid name: \"pörts\"\n"
            "   14 | \\tpörts={80=\"http\", 443=\"https\"} # Typo: 'pörts', not 'ports'; ASCII 1: \\x01\n"
            "      |   ^~~~~\n"
            "      |   Look out!\n");
      }

      // Test tab size 8
      {
        Position pos {
          .type=Position::error,
          .position=ex.position(),
          .ranges=ex.ranges(),
          .message=ex.message(),
          .caption="Look out!"
        };
        auto result = locations(io::resetg(is), { pos }, { .setLineString=true, .source=source });
        EXPECT_EQ(result.locations.size(), 1);
        ostringstream os;
        printLocations(os, nullopt, result);
        EXPECT_EQ(os.str(),
            "src/test/rocket/test-text-Config.ron:14:9: error: Invalid name: \"pörts\"\n"
            "   14 |         pörts={80=\"http\", 443=\"https\"} # Typo: 'pörts', not 'ports'; ASCII 1: \\x01\n"
            "      |         ^~~~~\n"
            "      |         Look out!\n");
      }
    }
  }

  // Test multi-line input, more than one position per line
  {
    string s = "a multi-line\ntext, where the second line is somewhat longer";
    auto is = io::is(s);
    Position pos0 { .type=Position::note, .position=2, .message="Oops1" };
    Position pos1 { .type=Position::warning, .position=13, .message="Oops2" };
    Position pos2 { .type=Position::error, .position=19, .message="Oops3", .caption="Watch out!" };
    auto result = locations(is, { pos0, pos1, pos2 }, { .setLineString=true, .source="foo" });
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
    ostringstream os;
    printLocations(os, nullopt, result, {});
    EXPECT_EQ(os.str(),
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
    Positions grsp;
    auto grs = unicode::graphemes(s, &grsp);
    EXPECT_EQ(
        grsp,
        positions({ { 0, 0 }, { 1, 11 }, { 2, 12 }, { 3, 13 }, { 4, 14 }, { 5, 15 }, { 6, 16 } }));
    auto is = io::is(s);
    Position pos { .type=Position::note, .position=13, .message="Oops" };
    auto result = locations(is, { pos }, {});
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    EXPECT_LOCATION(loc, Position::note, 13, ({}), 1, 9, ({ 0, 16 }), nullopt, "Oops", nullopt);
    string line = s.substr(loc.lineRange.lower, *loc.lineRange.upper - loc.lineRange.lower);
    EXPECT_EQ(line, s);
    ostringstream os;
    printLocations(os, s, result, {});
    EXPECT_EQ(os.str(),
        "(input):1:9: note: Oops\n"
        "    1 | 🧑‍🌾\\x00  abc\n"
        "      |         ^\n");
  }

  // Test multi-code-point grapheme, position at end of string
  {
    string s = "🧑‍🌾\x00\tabc"s;
    auto is = io::is(s);
    Position pos { .type=Position::note, .position=16, .ranges={ { 0, 16 } }, .message="Oops" };
    auto result = locations(is, { pos }, {});
    EXPECT_EQ(result.locations.size(), 1);
    auto& loc = result.locations[0];
    // Test a range that spans the entire line
    EXPECT_LOCATION(loc, Position::note, 16, ({ { 0, 16 } }), 1, 12, ({ 0, 16 }), nullopt, "Oops", nullopt);
    ostringstream os;
    printLocations(os, s, result, {});
    EXPECT_EQ(os.str(),
        "(input):1:12: note: Oops\n"
        "    1 | 🧑‍🌾\\x00  abc\n"
        "      | ~~~~~~~~~~~^\n");
  }
}

// EOF
