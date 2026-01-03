/*
 * test-unicode.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/format/std.h"
#include "rocket/math/random.h"
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

// Local functions ------------------------------------------------------------------------------------------

namespace {

void
testGrapheme(const Grapheme& grapheme, u32string_view s) {
  if (not system::env::get<bool>("ROCKET_TEST_TERMINAL").value_or(false)) {
    cout << "Not testing grapheme because `ROCKET_TEST_TERMINAL` is not set\n";
    return;
  }

  auto& out = nio::stdout;

  string s8 = utf32To8(s);
  out.print("[{}]", s8);
  auto pos = system::terminal::position(out);
  EXPECT_TRUE(pos);
  EXPECT_EQ(pos->first, grapheme.width + 3);
  out.write('\n');
  out.println("[{:~<{}}]", "", grapheme.width);
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

// `rocket::unicode::internal` ..............................................................................

TEST(unicode, internalBlockBiFind) {
  using namespace rocket::unicode::internal;

  auto gen = math::gen();

  size_t hits = 0;
  for (size_t i = 0; i < 1'000'000; ++i) {
    uint32_t cp = math::random(gen, 0U, 0xffffU);
    const auto* p = biFind(eastAsianWidthBlocks, cp);
    if (p)
     ++hits;
  }
  EXPECT_GT(hits, 900'000);
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

  EXPECT_EQ(static_cast<type>(CodePoint(0x41U)), "A");
  EXPECT_EQ(static_cast<type>(CodePoint(0xe4U)), "ä");
  EXPECT_EQ(static_cast<type>(CodePoint(0x20acU)), "€");
}

TEST(unicode, CodePointOpCastU32String) {
  using type = u32string;

  EXPECT_EQ(static_cast<type>(CodePoint(0x41U)), U"A");
  EXPECT_EQ(static_cast<type>(CodePoint(0xe4U)), U"ä");
  EXPECT_EQ(static_cast<type>(CodePoint(0x20acU)), U"€");
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
  EXPECT_EQ(CodePoint(0x0000U).width(), 0); // NUL (0)
  EXPECT_EQ(CodePoint(0x0001U).width(), -1); // SOH (1)
  EXPECT_EQ(CodePoint(0x001fU).width(), -1); // US (31)
  EXPECT_EQ(CodePoint(0x0020U).width(), 1); // SP (32)
  EXPECT_EQ(CodePoint(0x007eU).width(), 1); // Tilde (126)
  EXPECT_EQ(CodePoint(0x007fU).width(), -1); // DEL (127)
  EXPECT_EQ(CodePoint(0x009fU).width(), -1); // APC (159)
  EXPECT_EQ(CodePoint(0x00a0U).width(), 1); // NBSP (160)
  EXPECT_EQ(CodePoint(0x00adU).width(), 1); // Soft Hyphen (173)
  EXPECT_EQ(CodePoint(0x0300U).width(), 0); // Combining Grave Accent, Category Mn (768)
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
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\u20ac')), "U+20AC");
  EXPECT_EQ(fmt::format("{}", CodePoint(0x00U)), "U+0000");
  EXPECT_EQ(fmt::format("{}", CodePoint(0xffU)), "U+00FF");
  EXPECT_EQ(fmt::format("{}", CodePoint(0xffU)), "U+00FF");
  EXPECT_EQ(fmt::format("{}", CodePoint(0x1abcdU)), "U+1ABCD");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\U0010ffff')), "U+10FFFF");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\U0010ffff')), "U+10FFFF");
  EXPECT_EQ(fmt::format("{}", CodePoint(0x110000U)), "U+110000");
  EXPECT_EQ(fmt::format("{}", CodePoint(0x1000000U)), "U+1000000");
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

  EXPECT_EQ(width(graphemes("\u0378")), 1);
  EXPECT_EQ(width(graphemes("\ue000")), 1);
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
    EXPECT_EQ(v.codePoints.size(), 3);
    EXPECT_EQ(static_cast<string>(v), "🧑‍🌾");
    EXPECT_EQ(in.tell(), 11);

    EXPECT_EQ(read(in, v), 1);
    EXPECT_EQ(v.codePoints.size(), 1);
    EXPECT_EQ(static_cast<string>(v), "a");
    EXPECT_EQ(in.tell(), 12);
  }
}

TEST(unicode, GraphemeFormat) {
  // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
  EXPECT_EQ(fmt::format("{}", Grapheme("🧑‍🌾")), "🧑‍🌾");
  EXPECT_EQ(fmt::format("{:?}", Grapheme("a")), "\"a\"");
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
}

// `rocket::unicode::utf8` ..................................................................................

TEST(unicode, utf8CodePointSize) {
  EXPECT_EQ(utf8::codePointSize(97), 1); // 'a'
  EXPECT_EQ(utf8::codePointSize(0xc3), 2); // First byte of 'ä'
  EXPECT_EQ(utf8::codePointSize(0x80), 0); // Continuation byte: 1000 0000
  EXPECT_EQ(utf8::codePointSize(0xbf), 0); // Continuation byte: 1011 1111
}

TEST(unicode, utf8CodePoints) {
  EXPECT_EQ(utf8::codePoints("a"), (CodePoints { 0x61U }));
  EXPECT_EQ(utf8::codePoints("ä€"), (CodePoints { 0xe4U, 0x20acU }));
}

TEST(unicode, utf8CountCodePoints) {
  EXPECT_EQ(utf8::countCodePoints("abcde"), 5);
  EXPECT_EQ(utf8::countCodePoints("äüöß€"), 5);
}

TEST(unicode, utf8Valid) {
  EXPECT_TRUE(utf8::valid("äöüß€"));
  EXPECT_FALSE(utf8::valid("\x80äöü€"));

  string out;

  EXPECT_TRUE(utf8::valid("äöüß€", &out));
  EXPECT_EQ(out, "äöüß€");

  EXPECT_FALSE(utf8::valid("\x61\x80\x62\x81\x63", &out));
  EXPECT_EQ(out, "a�b�c");

  EXPECT_FALSE(utf8::valid("\x80äöü€", &out));
  EXPECT_EQ(out, "�äöü€");

  EXPECT_FALSE(utf8::valid("\xc3", &out));
  EXPECT_EQ(out, "�"); // Incomplete 'ä', which is C3 A4

  EXPECT_FALSE(utf8::valid("\xe2\x82", &out));
  EXPECT_EQ(out, "��"); // Incomplete '€', which is E2 82 AC
}

// `rocket::unicode::utf32` ---------------------------------------------------------------------------------

TEST(unicode, utf32Graphemes) {
  {
    // ZWJ
    u32string s = U"\u200D";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].codePoints.size(), 1);
    EXPECT_EQ(graphemes[0].width, 0);
    testGrapheme(graphemes[0], s);
  }

  {
    // ề: Latin Small Letter E, Combining Circumflex Accent, Combining Grave Acccent
    u32string s = U"\u0065\u0302\u0300";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].codePoints.size(), 3);
    EXPECT_EQ(graphemes[0].width, 1);
    testGrapheme(graphemes[0], s);
  }

  {
    // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
    u32string s = U"🧑‍🌾";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].codePoints.size(), 3);
    EXPECT_EQ(graphemes[0].width, 2);
    testGrapheme(graphemes[0], s);
  }

  {
    // Man, ZWJ, Woman, ZWJ, Boy
    u32string s = U"👨‍👩‍👦";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].codePoints.size(), 5);
    EXPECT_EQ(graphemes[0].width, 2);
    testGrapheme(graphemes[0], s);
  }

  {
    u32string s = U"👩🏻\u200d🚀";
    auto graphemes = utf32::graphemes(s);
    EXPECT_EQ(graphemes.size(), 1);
    EXPECT_EQ(graphemes[0].codePoints.size(), 4);
    EXPECT_EQ(graphemes[0].width, 2);
    testGrapheme(graphemes[0], s);
  }
}

// EOF
