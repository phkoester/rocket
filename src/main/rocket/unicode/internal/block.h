/*
 * block.h
 */

#pragma once

#include "rocket/math/Interval.h"

#include <unicodelib.h>

#include <vector>

namespace rocket::unicode::internal {

// Types ----------------------------------------------------------------------------------------------------

enum class EastAsianWidth {
  fullWidth, // F
  halfWidth, // H
  wide, // W
  narrow, // Na
  ambiguous, // A
  neutral // N
};

struct EastAsianWidthBlock {
  math::RightOpenInterval<uint32_t> interval;
  EastAsianWidth eastAsianWidth;
};

struct EmojiBlock {
  math::RightOpenInterval<uint32_t> interval;
};

// Data -----------------------------------------------------------------------------------------------------

extern const std::vector<EastAsianWidthBlock> eastAsianWidthBlocks;

extern const std::vector<EmojiBlock> emojiBlocksEmoji;
extern const std::vector<EmojiBlock> emojiBlocksEmoji_Presentation;
extern const std::vector<EmojiBlock> emojiBlocksEmoji_Modifier;
extern const std::vector<EmojiBlock> emojiBlocksEmoji_Modifier_Base;
extern const std::vector<EmojiBlock> emojiBlocksEmoji_Component;
extern const std::vector<EmojiBlock> emojiBlocksExtended_Pictographic;

// Functions ------------------------------------------------------------------------------------------------

/**
 * Performs something like a binary search in sorted disjunct intervals.
 */
template<typename Block>
const Block*
biFind(const std::vector<Block>& blocks, uint32_t cp) {
  size_t min = 0, max = blocks.size() - 1;

  if (cp < blocks[min].interval.lower || cp >= blocks[max].interval.upper)
    return nullptr;

  while (max > min) {
    size_t mid = (min + max) / 2;
    const auto& midInterval = blocks[mid].interval;
    if (cp >= midInterval.upper)
      min = mid + 1;
    else if (cp < midInterval.lower)
      max = mid - 1;
    else if (not blocks[min].interval.contains(cp))
      ++min;
    else
      return &blocks[min];
  }

  return blocks[min].interval.contains(cp) ? &blocks[min] : nullptr;
}

EastAsianWidth eastAsianWidth(uint32_t cp);

inline bool
emojiEmoji(uint32_t cp) {
  return biFind(emojiBlocksEmoji, cp) != nullptr;
}

inline bool
emojiEmoji_Presentation(uint32_t cp) {
  return biFind(emojiBlocksEmoji_Presentation, cp) != nullptr;
}

inline bool
emojiEmoji_Modifier(uint32_t cp) {
  return biFind(emojiBlocksEmoji_Modifier, cp) != nullptr;
}

inline bool
emojiEmoji_Modifier_Base(uint32_t cp) {
  return biFind(emojiBlocksEmoji_Modifier_Base, cp) != nullptr;
}

inline bool
emojiEmoji_Component(uint32_t cp) {
  return biFind(emojiBlocksEmoji_Component, cp) != nullptr;
}

inline bool
emojiExtended_Pictographic(uint32_t cp) {
  return biFind(emojiBlocksExtended_Pictographic, cp) != nullptr;
}

} // namespace rocket::unicode::internal

// EOF
