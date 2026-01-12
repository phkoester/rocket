/*
 * test-unicode.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/format/std.h"
#include "rocket/nio/nio.h"
#include "rocket/system/system.h"
#include "rocket/system/terminal/terminal.h"
#include "rocket/unicode/unicode.h"
#include "rocket/unicode/internal/block.h"

#include "rocket-gtest/matcher/matcher.h"

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace rocket::unicode;
using namespace std;
using namespace testing;

// Constants ------------------------------------------------------------------------------------------------

constexpr char TWO_BYTES    = 0b1101'1111;
constexpr char THREE_BYTES  = 0b1110'1111;
// constexpr char FOUR_BYTES   = 0b1111'0111;
constexpr char CONT         = 0b1011'1111;

constexpr char32_t D800 = static_cast<char32_t>(0xD800U);
constexpr char32_t MAX_PLUS_1 = static_cast<char32_t>(0x10FFFFU + 1);

// Functions ------------------------------------------------------------------------------------------------

// Functions ------------------------------------------------------------------------------------------------

auto
positions(initializer_list<pair<size_t, size_t>> list) {
  return makeUnorderedBimap(list);
}

void
testGrapheme(const Grapheme& grapheme, u32string_view s) {
  auto& out = nio::stdout;

  if (not system::env::get<bool>("ROCKET_TEST_TERMINAL").value_or(false)) {
    out.println("Not testing grapheme because `ROCKET_TEST_TERMINAL` is not set");
    return;
  }

  string s8 = utf32To8(s);
  out.print("[{}]", s8);
  auto pos = system::terminal::position(out);
  EXPECT_TRUE(pos);
  EXPECT_EQ(pos->first, grapheme.width() + 3);
  out.write('\n');
  out.println("[{:~<{}}]", "", grapheme.width());
}

// `TEST` ---------------------------------------------------------------------------------------------------

// `rocket::unicode::internal` ..............................................................................

TEST(unicode, internalBiFind) {
  using namespace rocket::unicode::internal;

  EXPECT_NE(biFind(eastAsianWidthBlocks, 0x100U), nullptr);
  EXPECT_NE(biFind(eastAsianWidthBlocks, 0x3fffdU), nullptr);
  EXPECT_EQ(biFind(eastAsianWidthBlocks, 0x3fffeU), nullptr);
  EXPECT_NE(biFind(eastAsianWidthBlocks, 0x10fffdU), nullptr);
  EXPECT_EQ(biFind(eastAsianWidthBlocks, 0x10fffeU), nullptr);
}

TEST(unicode, internalBlockEastAsianWidth) {
  using namespace rocket::unicode::internal;

  EXPECT_EQ(eastAsianWidth(0x0000U), EastAsianWidth::neutral);
  EXPECT_EQ(eastAsianWidth(0x0020U), EastAsianWidth::narrow);
  EXPECT_EQ(eastAsianWidth(0x00b8U), EastAsianWidth::ambiguous);
  EXPECT_EQ(eastAsianWidth(0xe0002U), EastAsianWidth::neutral);
  EXPECT_EQ(eastAsianWidth(0x10fffdU), EastAsianWidth::ambiguous);
  EXPECT_EQ(eastAsianWidth(0x10fffeU), EastAsianWidth::neutral);

  EXPECT_EQ(eastAsianWidth(0x01f468U), EastAsianWidth::wide); // MAN
}

TEST(unicode, internalBlockEmoji) {
  using namespace rocket::unicode::internal;

  EXPECT_FALSE(emojiEmoji(0x0000U));

  EXPECT_TRUE(emojiEmoji(0x0023U)); // HASH SIGN
  EXPECT_FALSE(emojiEmoji_Presentation(0x0023U));
  EXPECT_TRUE(emojiEmoji_Component(0x0023U));
  EXPECT_FALSE(emojiExtended_Pictographic(0x0023U));

  EXPECT_TRUE(emojiEmoji(0x2622U)); // RADIOACTIVE
  EXPECT_FALSE(emojiEmoji_Presentation(0x2622U));
  EXPECT_FALSE(emojiEmoji_Component(0x2622U));
  EXPECT_TRUE(emojiExtended_Pictographic(0x2622U));

  EXPECT_TRUE(emojiEmoji_Presentation(0x01f468U)); // MAN
}

// `rocket::unicode` ........................................................................................

TEST(unicode, CodePoint) {
  EXPECT_EQ(static_cast<uint32_t>(CodePoint('\x7f')), 127);

  EXPECT_THAT(
      [&] { CodePoint('\x80'); },
      ThrowsMessage<InvalidArgument>(HasSubstr("Parameter `v`: ")));
}

TEST(unicode, CodePointOpCasString) {
  using type = string;

  EXPECT_EQ(static_cast<type>(CodePoint(U'\x41')), "A");
  EXPECT_EQ(static_cast<type>(CodePoint(U'\xE4')), "ä");
  EXPECT_EQ(static_cast<type>(CodePoint(U'\x20AC')), "€");
}

TEST(unicode, CodePointOpCastU32String) {
  using type = u32string;

  EXPECT_EQ(static_cast<type>(CodePoint(U'\x41')), U"A");
  EXPECT_EQ(static_cast<type>(CodePoint(U'\xE4')), U"ä");
  EXPECT_EQ(static_cast<type>(CodePoint(U'\x20AC')), U"€");
}

TEST(unicode, CodePointLower) {
  using type = char32_t;

  EXPECT_EQ(static_cast<type>(CodePoint(U'a').lower()), U'a');
  EXPECT_EQ(static_cast<type>(CodePoint(U'Ä').lower()), U'ä');
  EXPECT_EQ(static_cast<type>(CodePoint(U'É').lower()), U'é');
}

TEST(unicode, CodePointUpper) {
  using type = char32_t;

  EXPECT_EQ(static_cast<type>(CodePoint(U'A').upper()), U'A');
  EXPECT_EQ(static_cast<type>(CodePoint(U'ä').upper()), U'Ä');
  EXPECT_EQ(static_cast<type>(CodePoint(U'é').upper()), U'É');
}

TEST(unicode, CodePointWidth) {
  EXPECT_EQ(CodePoint(U'\u0000').width(), 0); // NULL (0)
  EXPECT_EQ(CodePoint(U'\u0001').width(), 0); // START OF HEADING (1)
  EXPECT_EQ(CodePoint(U'\u001F').width(), 0); // INFORMATION SEPARATOR ONE (31)
  EXPECT_EQ(CodePoint(U'\u0020').width(), 1); // SPACE (32)
  EXPECT_EQ(CodePoint(U'\u007E').width(), 1); // TILDE (126)
  EXPECT_EQ(CodePoint(U'\u007F').width(), 0); // DELETE (127)
  EXPECT_EQ(CodePoint(U'\u0080').width(), 0); // PADDING CHARACTER (128)
  EXPECT_EQ(CodePoint(U'\u009F').width(), 0); // APPLICATION PROGRAM COMMAND (159)
  EXPECT_EQ(CodePoint(U'\u00A0').width(), 1); // NO-BREAK SPACE (160)
  EXPECT_EQ(CodePoint(U'\u00AD').width(), 0); // SOFT HYPHEN (173)
  EXPECT_EQ(CodePoint(U'\u0300').width(), 0); // COMBINING GRAVE ACCENT, Category Mn (768)
}

TEST(unicode, CodePointRead) {
  CodePoint v;

  {
    nio::StringSource in;
    EXPECT_EQ(read(in, v), 0);
    EXPECT_EQ(in.tell(), 0);
  }

  {
    nio::StringSource in("x");
    EXPECT_EQ(read(in, v), 1);
    EXPECT_EQ(v, 'x');
    EXPECT_EQ(in.tell(), 1);
  }

  {
    string input = "€";
    nio::StringSource in(input);
    EXPECT_EQ(read(in, v), input.size());
    EXPECT_EQ(v, U'€');
    EXPECT_EQ(in.tell(), input.size());
  }

  {
    string s = "€";
    string_view input(&s[1]); // Invalid UTF-8 byte sequence
    nio::StringSource in(input);
    EXPECT_EQ(read(in, v), 0);
    EXPECT_EQ(in.tell(), 0);
  }
}

TEST(unicode, CodePointFormat) {
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\u0000')), "U+0000");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\u20AC')), "U+20AC");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\u00FF')), "U+00FF");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\U0001ABCD')), "U+1ABCD");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\U0010FFFF')), "U+10FFFF");

  EXPECT_EQ(fmt::format(U"{}", CodePoint(U'\u20AC')), U"U+20AC");
}

TEST(unicode, CodePointsFormat) {
  CodePoints cps = { CodePoint(U'a'), CodePoint(U'b'), CodePoint(U'c') };
  EXPECT_EQ(fmt::format("{::}", cps), "[U+0061, U+0062, U+0063]");
  EXPECT_EQ(fmt::format("{::~>8}", cps), "[~~U+0061, ~~U+0062, ~~U+0063]");
  EXPECT_EQ(fmt::format("{:n:~>8}", cps), "~~U+0061, ~~U+0062, ~~U+0063");
}

TEST(unicode, Grapheme) {
  EXPECT_EQ(width(graphemes(U"a")), 1);
  EXPECT_EQ(width(graphemes(U"😁")), 2);

  // The following tests are taken from the Rust crate `unicode-display-width`

  EXPECT_EQ(width(graphemes("🔥🗡🍩👩🏻‍🚀⏰💃🏼🔦👍🏻")), 15);
  EXPECT_EQ(width(graphemes("🦀")), 2);
  EXPECT_EQ(width(graphemes("👨‍👩‍👧‍👧")), 2);
  EXPECT_EQ(width(graphemes("👩‍🔬")), 2);
  EXPECT_EQ(width(graphemes("sane text")), 9);
  EXPECT_EQ(width(graphemes("Ẓ̌á̲l͔̝̞̄̑͌g̖̘̘̔̔͢͞͝o̪̔T̢̙̫̈̍͞e̬͈͕͌̏͑x̺̍ṭ̓̓ͅ")), 9);
  EXPECT_EQ(width(graphemes("슬라바 우크라이나")), 17);
}

TEST(unicode, GraphemeOpCastString) {
  using type = string;

  EXPECT_EQ(static_cast<type>(Grapheme("A")), "A");
  EXPECT_EQ(static_cast<type>(Grapheme("€")), "€");
  EXPECT_EQ(static_cast<type>(Grapheme("😁")), "😁");
}

TEST(unicode, GraphemeOpCastU32string) {
  using type = u32string;

  EXPECT_EQ(static_cast<type>(Grapheme("A")), U"A");
  EXPECT_EQ(static_cast<type>(Grapheme("€")), U"€");
  EXPECT_EQ(static_cast<type>(Grapheme("😁")), U"😁");
}

TEST(unicode, GraphemeRead) {
  Grapheme v;

  {
    nio::StringSource in;
    EXPECT_EQ(read(in, v), 0);
    EXPECT_EQ(in.tell(), 0);
  }

  {
    nio::StringSource in("🧑‍🌾a");

    EXPECT_EQ(read(in, v), 11);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(static_cast<string>(v), "🧑‍🌾");
    EXPECT_EQ(in.tell(), 11);

    EXPECT_EQ(read(in, v), 1);
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(static_cast<string>(v), "a");
    EXPECT_EQ(in.tell(), 12);
  }
}

TEST(unicode, GraphemeFormat) {
  // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
  EXPECT_EQ(fmt::format("{}", Grapheme("🧑‍🌾")), "🧑‍🌾");
  EXPECT_EQ(fmt::format("{:?}", Grapheme("a")), "\"a\"");

  EXPECT_EQ(fmt::format(U"{}", Grapheme("🧑‍🌾")), U"🧑‍🌾");
}

TEST(unicode, conversions) {
  EXPECT_EQ(utf8To32("äöü€"), U"äöü€");
  EXPECT_EQ(utf32To8(U"äöü€"), "äöü€");

  const char* s1 = "a€b";

  vector<char32_t> v;
  u32string s = utf8To32(s1);
  copy(s.begin(), s.end(), back_inserter(v));
  EXPECT_EQ(v, (vector<char32_t>{ 97, 0x20ac, 98 }));

  u32string s2 = utf8To32(s1);
  EXPECT_EQ(s2, U"a€b");
  EXPECT_EQ(s2.size(), 3);

  string s3 = utf32To8(s2);
  EXPECT_EQ(s3, s1);

  // U+1F9D1 (ADULT), U+200D (ZERO WIDTH JOINER), U+1F33E (EAR OF RICE)
  EXPECT_EQ(utf8To32("a🧑‍🌾b"), U"a🧑‍🌾b");

  auto s32 = utf8To32("🧑‍🌾");
  ASSERT_EQ(s32.size(), 3);
  EXPECT_EQ(s32[0], 0x1F9D1);
  EXPECT_EQ(s32[1], 0x200D);
  EXPECT_EQ(s32[2], 0x1F33E);
}

// `rocket::unicode::utf8` ..................................................................................

TEST(unicode, utf8CodePointSize) {
  EXPECT_EQ(utf8::codePointSize(97), 1); // 'a'
  EXPECT_EQ(utf8::codePointSize(0xc3), 2); // First byte of 'ä'
  EXPECT_EQ(utf8::codePointSize(0x80), 0); // Continuation byte: 1000 0000
  EXPECT_EQ(utf8::codePointSize(0xbf), 0); // Continuation byte: 1011 1111
}

TEST(unicode, utf8CodePoints) {
  EXPECT_EQ(utf8::codePoints("a"), (CodePoints { U'\x61' }));
  EXPECT_EQ(utf8::codePoints("ä€"), (CodePoints { U'\xE4', U'\u20AC' }));
}

TEST(unicode, utf8CountCodePoints) {
  EXPECT_EQ(utf8::countCodePoints("abcde"), 5);
  EXPECT_EQ(utf8::countCodePoints("äüöß€"), 5);
}

TEST(unicode, utf8Validate) {
  UnorderedBimap<size_t, size_t> pos;

  {
    string_view sv = "äöüß€";
    auto cow = utf8::validate(sv, &pos);
    EXPECT_FALSE(cow.modified());
    EXPECT_EQ(cow.get(), "äöüß€");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 2, 2 }, { 4, 4 }, { 6, 6 }, { 8, 8 }, { 11, 11 }}));
  }

  static_assert("�"sv.size() == 3);

  {
    string s = { 'a', CONT, 'b' };
    string_view sv = s;
    auto cow = utf8::validate(sv, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�b");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 5 } }));
  }

  {
    string s = { 'a', TWO_BYTES, 'b', 'c' };
    string_view sv = s;
    auto cow = utf8::validate(sv, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�c");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 3, 4 }, { 4, 5 } }));
  }

  {
    string s = { 'a', THREE_BYTES, CONT };
    string_view sv = s;
    auto cow = utf8::validate(sv, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 3, 4 } }));
  }
}

// `rocket::unicode::utf32` ---------------------------------------------------------------------------------

TEST(unicode, utf32Graphemes) {
  {
    // ZWJ
    u32string s = U"\u200D";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].size(), 1);
    EXPECT_EQ(graphemes[0].width(), 0);
    testGrapheme(graphemes[0], s);
  }

  {
    // ề: Latin Small Letter E, Combining Circumflex Accent, Combining Grave Acccent
    u32string s = U"\u0065\u0302\u0300";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].size(), 3);
    EXPECT_EQ(graphemes[0].width(), 1);
    testGrapheme(graphemes[0], s);
  }

  {
    // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
    u32string s = U"🧑‍🌾";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].size(), 3);
    EXPECT_EQ(graphemes[0].width(), 2);
    testGrapheme(graphemes[0], s);
  }

  {
    // Man, ZWJ, Woman, ZWJ, Boy
    u32string s = U"👨‍👩‍👦";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].size(), 5);
    EXPECT_EQ(graphemes[0].width(), 2);
    testGrapheme(graphemes[0], s);
  }

  {
    u32string s = U"👩🏻\u200d🚀";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].size(), 4);
    EXPECT_EQ(graphemes[0].width(), 2);
    testGrapheme(graphemes[0], s);
  }
}

TEST(unicode, utf32Validate) {
  {
    u32string_view sv = U"abc";
    auto cow = utf32::validate(sv);
    EXPECT_FALSE(cow.modified());
    EXPECT_EQ(cow.get(), U"abc");
  }

  {
    u32string s = { 'a', D800, 'b', MAX_PLUS_1 };
    u32string_view sv = s;
    auto cow = utf32::validate(sv);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), U"a�b�");
  }
}

// EOF
