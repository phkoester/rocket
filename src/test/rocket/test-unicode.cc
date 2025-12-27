/*
 * test-unicode.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-rocket-decl.h"
#include "rocket/codec-rocket.h"

#include "rocket/io.h"
#include "rocket/system.h"
#include "rocket/terminal.h"
#include "rocket/unicode.h"
#include "rocket/internal/unicode-internal.h"

#include "rocket-gtest/matcher.h"

#include <random>

using namespace rocket;
using namespace rocket::gtest::matcher;
using namespace rocket::unicode;
using namespace rocket::unicode::internal;
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
  auto pos = terminal::position(out);
  EXPECT_TRUE(pos);
  EXPECT_EQ(pos->first, grapheme.width + 3);
  out.write('\n');
  out.println("[{:~<{}}]", "", grapheme.width);
}

} // namespace

// `TEST` ---------------------------------------------------------------------------------------------------

// `rocket::unicode::internal` ..............................................................................

TEST(unicode, biFind) {
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> distrib(0, 0xffffU);

  size_t hits = 0;
  for (size_t i = 0; i < 1'000'000; ++i) {
    uint32_t cp = distrib(gen);
    const auto* p = biFind(eastAsianWidthBlocks, cp);
    if (p)
     ++hits;
  }
  EXPECT_GT(hits, 900'000);
}

TEST(unicode, eastAsianWidth) {
  EXPECT_EQ(eastAsianWidth(0x0000U), EastAsianWidth::neutral);
  EXPECT_EQ(eastAsianWidth(0x0020U), EastAsianWidth::narrow);
  EXPECT_EQ(eastAsianWidth(0x00b8U), EastAsianWidth::ambiguous);
  EXPECT_EQ(eastAsianWidth(0xe0002U), EastAsianWidth::neutral);
  EXPECT_EQ(eastAsianWidth(0x10fffdU), EastAsianWidth::ambiguous);
  EXPECT_EQ(eastAsianWidth(0x10fffeU), EastAsianWidth::neutral);

  EXPECT_EQ(eastAsianWidth(0x01f468U), EastAsianWidth::wide); // MAN
}

TEST(unicode, emoji) {
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

TEST(unicode, CodePoint_opCast_string) {
  using type = string;

  EXPECT_EQ(static_cast<type>(CodePoint(0x41U)), "A");
  EXPECT_EQ(static_cast<type>(CodePoint(0xe4U)), "ä");
  EXPECT_EQ(static_cast<type>(CodePoint(0x20acU)), "€");
}

TEST(unicode, CodePoint_opCast_u32string) {
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

TEST(unicode, opInput_CodePoint_char) {
  using type = CodePoint;
  using charType = char;

  type v;

  {
    auto is = io::is<charType>();
    is >> v;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    string input = "x";
    auto is = io::is(input);
    is >> v;
    EXPECT_EQ(v, 'x');
    EXPECT_ISTREAM(is, false, false, input.size());
  }

  {
    string input = "€";
    auto is = io::is(input);
    is >> v;
    EXPECT_EQ(v, 0x20acU);
    EXPECT_ISTREAM(is, false, false, input.size());
  }
}

TEST(unicode, opInput_CodePoint_char32_t) {
  using type = CodePoint;
  using charType = char32_t;

  type v;

  {
    auto is = io::is<charType>();
    is >> v;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    u32string input = U"x";
    auto is = io::is(input);
    is >> v;
    EXPECT_EQ(v, 'x');
    EXPECT_ISTREAM(is, false, false, input.size());
  }

  {
    u32string input = U"€";
    auto is = io::is(input);
    is >> v;
    EXPECT_EQ(v, 0x20acU);
    EXPECT_ISTREAM(is, false, false, input.size());
  }
}

TEST(unicode, parseRon_CodePoint) {
  CodePoint v;

  {
    auto is = io::is("U+12345678abcd,");
    EXPECT_THAT(
        [&] { parseRon(io::resetg(is), v); },
        throwsParseFailure<char>(10, { 0, 14 }, HasSubstr("Expected at most 8 hexadecimal characters, got 12")));
    EXPECT_ISTREAM(is, true, false, 14);
  }

  {
    auto is = io::is("U+007f");
    parseRon(io::resetg(is), v);
    EXPECT_EQ(v, CodePoint(0x007fU));
    EXPECT_ISTREAM(is, false, false, 6);
  }

  {
    auto is = io::is("U+1abcD");
    parseRon(io::resetg(is), v);
    EXPECT_EQ(v, CodePoint(0x1abcdU));
    EXPECT_ISTREAM(is, false, false, 7);
  }
}

TEST(unicode, CodePointFormat) {
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\u20ac')), "€");
  EXPECT_EQ(fmt::format("{:?}", CodePoint(U'\u20ac')), "U+20AC");
  EXPECT_EQ(fmt::format("{:?}", CodePoint(0x00U)), "U+0000");
  EXPECT_EQ(fmt::format("{:?}", CodePoint(0xffU)), "U+00FF");
  EXPECT_EQ(fmt::format("{:?}", CodePoint(0xffU)), "U+00FF");
  EXPECT_EQ(fmt::format("{:?}", CodePoint(0x1abcdU)), "U+1ABCD");
  EXPECT_EQ(fmt::format("{:?}", CodePoint(U'\U0010ffff')), "U+10FFFF");
  EXPECT_EQ(fmt::format("{:?}", CodePoint(U'\U0010ffff')), "U+10FFFF");
  EXPECT_EQ(fmt::format("{:?}", CodePoint(0x110000U)), "U+110000");
  EXPECT_EQ(fmt::format("{:?}", CodePoint(0x1000000U)), "U+1000000");
}

TEST(unicode, CodePointsFormat) {
  CodePoints cps = { CodePoint(U'a'), CodePoint(U'b'), CodePoint(U'c') };
  EXPECT_EQ(fmt::format("{}", cps), "[a, b, c]");
  EXPECT_EQ(fmt::format("{::?}", cps), "[U+0061, U+0062, U+0063]");
  EXPECT_EQ(fmt::format("{::~>6?}", cps), "[~~U+0061, ~~U+0062, ~~U+0063]");
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

TEST(unicode, Grapheme_opCast_string) {
  using type = string;

  EXPECT_EQ(static_cast<type>(Grapheme("A")), "A");
  EXPECT_EQ(static_cast<type>(Grapheme("€")), "€");
  EXPECT_EQ(static_cast<type>(Grapheme("😁")), "😁");
}

TEST(unicode, Grapheme_opCast_u32string) {
  using type = u32string;

  EXPECT_EQ(static_cast<type>(Grapheme("A")), U"A");
  EXPECT_EQ(static_cast<type>(Grapheme("€")), U"€");
  EXPECT_EQ(static_cast<type>(Grapheme("😁")), U"😁");
}

TEST(unicode, opInput_Grapheme_char) {
  using type = Grapheme;
  using charType = char;

  type v;

  {
    auto is = io::is<charType>();
    is >> v;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    string input = "🧑‍🌾a";
    auto is = io::is(input);
    is >> v;
    EXPECT_EQ(v.codePoints.size(), 3);
    EXPECT_EQ(v.width, 2);
    EXPECT_ISTREAM(is, false, false, input.size() - 1);
    is >> v;
    EXPECT_EQ(v.codePoints.size(), 1);
    EXPECT_EQ(v.width, 1);
    EXPECT_ISTREAM(is, false, false, input.size());
  }
}

TEST(unicode, opInput_Grapheme_char32_t) {
  using type = Grapheme;
  using charType = char32_t;

  type v;

  {
    auto is = io::is<charType>();
    is >> v;
    EXPECT_ISTREAM(is, true, true, 0);
  }

  {
    u32string input = U"🧑‍🌾a";
    auto is = io::is(input);
    is >> v;
    EXPECT_EQ(v.codePoints.size(), 3);
    EXPECT_EQ(v.width, 2);
    EXPECT_ISTREAM(is, false, false, input.size() - 1);
    is >> v;
    EXPECT_EQ(v.codePoints.size(), 1);
    EXPECT_EQ(v.width, 1);
    EXPECT_ISTREAM(is, false, false, input.size());
  }
}

TEST(unicode, parseRon_Grapheme) {
  Grapheme v;

  {
    auto is = io::is("\"😁\"");
    parseRon(is, v);
    EXPECT_EQ(v, Grapheme("😁"));
    EXPECT_ISTREAM(is, false, false, 6);
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

TEST(unicode, CodePointIterator_char) {
  using type = char;

  string_view s = "ä€";

  auto it = CodePointIterator<type>(s);
  EXPECT_FALSE(it.decrement());
  EXPECT_TRUE(it.begin());
  EXPECT_FALSE(it.end());
  EXPECT_EQ(it.codePointSize(), 2);
  EXPECT_EQ(*it, CodePoint(U'ä'));

  ++it;
  EXPECT_FALSE(it.begin());
  EXPECT_FALSE(it.end());
  EXPECT_EQ(it.codePointSize(), 3);
  EXPECT_EQ(*it, CodePoint(U'€'));

  ++it;
  EXPECT_FALSE(it.begin());
  EXPECT_TRUE(it.end());

  auto it2(it);
  it2 -= 2;
  EXPECT_TRUE(it2.begin());
  EXPECT_FALSE(it2.end());

  auto end = CodePointIterator<type>(s, s.size());
  EXPECT_EQ(end.codePointPosition(), 2);

  auto beg = CodePointIterator<type>(s);
  EXPECT_EQ(distance(beg, end), 2);
}

TEST(unicode, CodePointIterator_char32_t) {
  using type = char32_t;

  u32string_view s = U"ä€";

  auto it = CodePointIterator<type>(s);
  EXPECT_FALSE(it.decrement());
  EXPECT_TRUE(it.begin());
  EXPECT_FALSE(it.end());
  EXPECT_EQ(*it, CodePoint(U'ä'));

  ++it;
  EXPECT_FALSE(it.begin());
  EXPECT_FALSE(it.end());
  EXPECT_EQ(*it, CodePoint(U'€'));

  ++it;
  EXPECT_FALSE(it.begin());
  EXPECT_TRUE(it.end());

  auto it2(it);
  it2 -= 2;
  EXPECT_TRUE(it2.begin());
  EXPECT_FALSE(it2.end());

  auto end = CodePointIterator<type>(s, s.size());
  EXPECT_EQ(end.codePointPosition(), 2);

  auto beg = CodePointIterator<type>(s);
  EXPECT_EQ(distance(beg, end), 2);
}

TEST(unicode, GraphemeIterator_char) {
  using type = char;

  // ☢️:  6 bytes, 2 code points
  // 🧑‍🌾: 11 bytes, 3 code points
  string_view s = "☢️🧑‍🌾";
  EXPECT_EQ(s.size(), 17);
  EXPECT_EQ(countCodePoints(s), 5);
  EXPECT_EQ(countGraphemes(s), 2);

  auto it = GraphemeIterator<type>(s);
  EXPECT_TRUE(it.begin());
  EXPECT_FALSE(it.end());
  EXPECT_EQ(it.position(), 0);
  EXPECT_EQ(it.graphemeSize(), 2);
  EXPECT_EQ(*it, Grapheme("☢️"));

  ++it;
  EXPECT_FALSE(it.begin());
  EXPECT_FALSE(it.end());
  EXPECT_EQ(it.position(), 6);
  EXPECT_EQ(it.graphemeSize(), 3);
  EXPECT_EQ(*it, Grapheme("🧑‍🌾"));

  ++it;
  EXPECT_FALSE(it.begin());
  EXPECT_TRUE(it.end());

  auto it2(it);
  EXPECT_EQ(it2.position(), 17);
  --it2;
  EXPECT_EQ(it2.position(), 6);
  it2 -= 1;
  EXPECT_TRUE(it2.begin());
  EXPECT_FALSE(it2.end());

  auto end = GraphemeIterator<type>(s, s.size());
  EXPECT_EQ(end.graphemePosition(), 2);

  auto beg = GraphemeIterator<type>(s);
  EXPECT_EQ(distance(beg, end), 2);
}

TEST(unicode, GraphemeIterator_char32_t) {
  using type = char32_t;

  // ☢️: 2 code points
  // 🧑‍🌾: 3 code points
  u32string_view s = U"☢️🧑‍🌾";
  EXPECT_EQ(s.size(), 5);
  EXPECT_EQ(countCodePoints(s), 5);
  EXPECT_EQ(countGraphemes(s), 2);

  auto it = GraphemeIterator<type>(s);
  EXPECT_TRUE(it.begin());
  EXPECT_FALSE(it.end());
  EXPECT_EQ(it.position(), 0);
  EXPECT_EQ(it.graphemeSize(), 2);
  EXPECT_EQ(*it, Grapheme("☢️"));

  ++it;
  EXPECT_FALSE(it.begin());
  EXPECT_FALSE(it.end());
  EXPECT_EQ(it.position(), 2);
  EXPECT_EQ(it.graphemeSize(), 3);
  EXPECT_EQ(*it, Grapheme("🧑‍🌾"));

  ++it;
  EXPECT_FALSE(it.begin());
  EXPECT_TRUE(it.end());

  auto it2(it);
  EXPECT_EQ(it2.position(), 5);
  --it2;
  EXPECT_EQ(it2.position(), 2);
  it2 -= 1;
  EXPECT_TRUE(it2.begin());
  EXPECT_FALSE(it2.end());

  auto end = GraphemeIterator<type>(s, s.size());
  EXPECT_EQ(end.graphemePosition(), 2);

  auto beg = GraphemeIterator<type>(s);
  EXPECT_EQ(distance(beg, end), 2);
}

// `rocket::unicode::utf8` ..................................................................................

TEST(unicode, utf8_codePointSize) {
  EXPECT_EQ(utf8::codePointSize(97), 1); // 'a'
  EXPECT_EQ(utf8::codePointSize(0xc3), 2); // First byte of 'ä'
  EXPECT_EQ(utf8::codePointSize(0x80), 0); // Continuation byte: 1000 0000
  EXPECT_EQ(utf8::codePointSize(0xbf), 0); // Continuation byte: 1011 1111
}

TEST(unicode, utf8_codePoints) {
  EXPECT_EQ(utf8::codePoints("a"), (CodePoints { 0x61U }));
  EXPECT_EQ(utf8::codePoints("ä€"), (CodePoints { 0xe4U, 0x20acU }));
}

TEST(unicode, utf8_countCodePoints) {
  EXPECT_EQ(utf8::countCodePoints("abcde"), 5);
  EXPECT_EQ(utf8::countCodePoints("äüöß€"), 5);
}

TEST(unicode, utf8_valid) {
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

TEST(unicode, utf32_graphemes) {
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
