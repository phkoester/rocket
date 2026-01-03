/*
 * test-libunicode.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include <libunicode/capi.h>
#include <libunicode/grapheme_segmenter.h>
#include <libunicode/utf8_grapheme_segmenter.h>

using namespace std;
using namespace unicode;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(libunicode, graphemeSegmenter) {
  auto s = U"abc";
  auto segmenter = grapheme_segmenter(s);
  int i = 0;
  // XXX Strange loop
  while (true) {
    auto segment = *segmenter;
    if (segment.empty()) {
      break;
    }
    ++i;
    ++segmenter;
  }
  EXPECT_EQ(i, 3);
}

TEST(libunicode, u32GcWidth) {
  u32string s = U"abc";
  int w = u32_gc_width(reinterpret_cast<const u32_char_t*>(s.data()), s.size(), GC_WIDTH_MODE_MODIFIABLE);
  EXPECT_EQ(w, 2); // XXX 3

  // U+01F9D1 (Adult), U+200D (ZWJ), U+01F33E (Ear of rice)
  s = U"🧑‍🌾";
  w = u32_gc_width(reinterpret_cast<const u32_char_t*>(s.data()), s.size(), GC_WIDTH_MODE_MODIFIABLE);
  EXPECT_EQ(w, 0); // XXX 2
}

// EOF
