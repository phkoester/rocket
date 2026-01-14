/*
 * test-Character.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/system/terminal/terminal.h"
#include "rocket/unicode/Character.h"
#include "rocket/unicode/ConvertTo.h"

using namespace rocket;
using namespace rocket::gtest;
using namespace rocket::unicode;
using namespace std;

// Functions ------------------------------------------------------------------------------------------------

template<typename C> requires IsChar<C>
void
testCharacter(const Character<C>& c) {
  auto& out = nio::stdout;

  if (not TEST_TERMINAL) {
    out.println("Not testing character because `" ROCKET_TEST_TERMINAL "` is not set");
    return;
  }

  out.print("[{}]", ConvertTo<char>().apply(c));
  auto pos = system::terminal::position(out);
  EXPECT_TRUE(pos);
  EXPECT_EQ(pos->first, c.width() + 3); // Check terminal's cursor position
  out.write('\n');
  out.println("[{:~<{}}]", "", c.width());
}

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(Character, charWidth) {
  EXPECT_EQ(Character("\t"sv).width(), 0);

  {
    Character c("\u200D"sv); // u+0200D (ZERO WIDTH JOINER)
    EXPECT_EQ(c.width(), 0);
  }

  {
    // ề: LATIN SMALL LETTER E, COMBINING CIRCUMFLEX ACCENT, COMBINING GRAVE ACCENT
    Character c("\u0065\u0302\u0300"sv);
    EXPECT_EQ(c.width(), 1);
    testCharacter(c);
  }

  {
    // U+1F9D1 (ADULT), U+200D (ZERO WIDTH JOINER), U+1F33E (EAR OF RICE)
    Character c("🧑‍🌾"sv);
    EXPECT_EQ(c.width(), 2);
    testCharacter(c);
  }

  {
    // MAN, ZERO WIDTH JOINER, WOMAN, ZERO WIDTH JOINER, BOY
    Character c("👨‍👩‍👦"sv);
    EXPECT_EQ(c.countCodePoints(), 5);
    EXPECT_EQ(c.width(), 2);
    testCharacter(c);
  }

  {
    // ?, ZERO WIDTH JOINER, ?
    Character c("👩🏻\u200D🚀"sv);
    EXPECT_EQ(c.countCodePoints(), 4);
    EXPECT_EQ(c.width(), 2);
    testCharacter(c);
  }
}

TEST(Character, char32Width) {
  {
    // MAN, ZERO WIDTH JOINER, WOMAN, ZERO WIDTH JOINER, BOY
    Character c(U"👨‍👩‍👦"sv);
    EXPECT_EQ(c.countCodePoints(), 5);
    EXPECT_EQ(c.width(), 2);
    testCharacter(c);
  }

  {
    // ?, ZERO WIDTH JOINER, ?
    Character c(U"👩🏻\u200D🚀"sv);
    EXPECT_EQ(c.countCodePoints(), 4);
    EXPECT_EQ(c.width(), 2);
    testCharacter(c);
  }
}

// EOF
