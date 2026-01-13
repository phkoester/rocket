/*
 * test-Iterator.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/unicode/iterator.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(iterator, CodePointIteratorChar) {
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

TEST(iterator, CodePointIteratorChar32) {
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

TEST(iterator, GraphemeIteratorChar) {
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

TEST(iterator, GraphemeIteratorChar32) {
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

// EOF
