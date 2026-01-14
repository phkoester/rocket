/*
 * test-unicode.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/format/std.h"
#include "rocket/unicode/unicode.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;
using namespace testing;

// Constants ------------------------------------------------------------------------------------------------

constexpr char TWO_BYTES    = 0b1101'1111;
constexpr char THREE_BYTES  = 0b1110'1111;
constexpr char FOUR_BYTES   = 0b1111'0111;
constexpr char CONT         = 0b1011'1111;

constexpr char32_t D800       = static_cast<char32_t>(0xD800U);
constexpr char32_t MAX_PLUS_1 = static_cast<char32_t>(0x10FFFFU + 1);

// Functions ------------------------------------------------------------------------------------------------

auto
positions(initializer_list<pair<size_t, size_t>> list) {
  return makeUnorderedBimap(list);
}

// `TEST` ---------------------------------------------------------------------------------------------------

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

TEST(unicode, CodePointIsPrint) {
  EXPECT_TRUE(CodePoint('a').isPrint());
  EXPECT_FALSE(CodePoint(U'\uFFF0').isPrint());
  EXPECT_FALSE(CodePoint(U'\uFFFF').isPrint());
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

TEST(unicode, CodePointFormat) {
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\u0000')), "U+0000");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\u20AC')), "U+20AC");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\u00FF')), "U+00FF");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\U0001ABCD')), "U+1ABCD");
  EXPECT_EQ(fmt::format("{}", CodePoint(U'\U0010FFFF')), "U+10FFFF");

  EXPECT_EQ(fmt::format(U"{}", CodePoint(U'\u20AC')), U"U+20AC");
}

TEST(unicode, CodePointVectorFormat) {
  auto cps = vector<CodePoint> { CodePoint(U'a'), CodePoint(U'b'), CodePoint(U'c') };
  EXPECT_EQ(fmt::format("{::}", cps), "[U+0061, U+0062, U+0063]");
  EXPECT_EQ(fmt::format("{::~>8}", cps), "[~~U+0061, ~~U+0062, ~~U+0063]");
  EXPECT_EQ(fmt::format("{:n:~>8}", cps), "~~U+0061, ~~U+0062, ~~U+0063");
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

TEST(unicode, utf8Validate) {
  UnorderedBimap<size_t, size_t> pos;

  {
    auto cow = utf8::validate("äöüß€", &pos);
    EXPECT_FALSE(cow.modified());
    EXPECT_EQ(cow.get(), "äöüß€");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 2, 2 }, { 4, 4 }, { 6, 6 }, { 8, 8 }, { 11, 11 }}));
  }

  static_assert("�"sv.size() == 3);

  {
    string s = { 'a', CONT, 'b' };
    auto cow = utf8::validate(s, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�b");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 5 } }));
  }

  {
    string s = { 'a', TWO_BYTES, 'b', 'c' };
    auto cow = utf8::validate(s, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�bc");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 5 }, { 4, 6 } }));
  }

  {
    string s = { 'a', THREE_BYTES, CONT };
    auto cow = utf8::validate(s, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a�");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 3, 4 } }));
  }

  {
    string s = { 'a', FOUR_BYTES, CONT, 'b' };
    auto cow = utf8::validate(s, &pos);
    EXPECT_TRUE(cow.modified());
    EXPECT_EQ(cow.get(), "a��b");
    EXPECT_EQ(pos, positions({ { 0, 0 }, { 1, 1 }, { 2, 4 }, { 3, 7 }, { 4, 8 } }));
  }
}

// `rocket::unicode::utf32` ---------------------------------------------------------------------------------

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
