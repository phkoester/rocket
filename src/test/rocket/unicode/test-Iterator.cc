/*
 * test-Iterator.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/unicode/Character.h"
#include "rocket/unicode/Iterator.h"

using namespace rocket::unicode;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Iterator, char) {
  using type = char;

  string_view str = "ä€"; // 2 + 3 bytes

  auto it = Iterator<type>(IteratorType::Character, str);
  EXPECT_EQ(it.previous(), NPOS);
  EXPECT_EQ(it.previous(), NPOS); // Not a typo: testing twice
  EXPECT_EQ(it.first(), 0);
  EXPECT_EQ(it.last(), str.size());
  it.first();
  EXPECT_EQ(it.nextSegment(), "ä");
  EXPECT_EQ(it.current(), 2);

  EXPECT_EQ(it.nextSegment(), "€");
  EXPECT_EQ(it.current(), 5);
  EXPECT_EQ(it.next(), NPOS);
  EXPECT_EQ(it.next(), NPOS); // Not a typo: testing twice
}

TEST(Iterator, charMultiCodePoint) {
  using type = char;

  string_view str = "🧑‍🌾\r\n👨‍👩‍👦"sv;
  auto it = Iterator<type>(IteratorType::Character, str);
  auto segs = it.nextSegments();
  EXPECT_EQ(segs.size(), 3);
  EXPECT_EQ(segs[0], "🧑‍🌾");
  EXPECT_EQ(unicode::CharacterView<char>(segs[0]).countCodePoints(), 3);
  EXPECT_EQ(segs[1], "\r\n");
  EXPECT_TRUE(unicode::CharacterView<char>(segs[1]).crLf());
  EXPECT_TRUE(unicode::CharacterView<char>(segs[1]).eol());
  EXPECT_EQ(unicode::CharacterView<char>(segs[1]).countCodePoints(), 2);
  EXPECT_EQ(segs[2], "👨‍👩‍👦");
  EXPECT_EQ(unicode::CharacterView<char>(segs[2]).countCodePoints(), 5);
}

TEST(Iterator, char32) {
  using type = char32;

  u32string_view str = U"ä€";

  auto it = Iterator<type>(IteratorType::Character, str);
  EXPECT_EQ(it.previous(), NPOS);
  EXPECT_EQ(it.previous(), NPOS); // Not a typo: testing twice
  EXPECT_EQ(it.first(), 0);
  EXPECT_EQ(it.last(), str.size());
  it.first();
  EXPECT_EQ(it.nextSegment(), U"ä");
  EXPECT_EQ(it.current(), 1);

  EXPECT_EQ(it.nextSegment(), U"€");
  EXPECT_EQ(it.current(), 2);
  EXPECT_EQ(it.next(), NPOS);
  EXPECT_EQ(it.next(), NPOS); // Not a typo: testing twice
}

TEST(Iterator, char32MultiCodePoint) {
  using type = char32;

  u32string_view str = U"🧑‍🌾\r\n👨‍👩‍👦";
  auto it = Iterator<type>(IteratorType::Character, str);
  auto segs = it.nextSegments();
  EXPECT_EQ(segs.size(), 3);
  EXPECT_EQ(segs[0], U"🧑‍🌾");
  EXPECT_EQ(unicode::CharacterView<char32>(segs[0]).countCodePoints(), 3);
  EXPECT_EQ(segs[1], U"\r\n");
  EXPECT_TRUE(unicode::CharacterView<char32>(segs[1]).crLf());
  EXPECT_TRUE(unicode::CharacterView<char32>(segs[1]).eol());
  EXPECT_EQ(unicode::CharacterView<char32>(segs[1]).countCodePoints(), 2);
  EXPECT_EQ(segs[2], U"👨‍👩‍👦");
  EXPECT_EQ(unicode::CharacterView<char32>(segs[2]).countCodePoints(), 5);
}

// EOF
