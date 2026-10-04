/*
 * test-rocket.cc
 */

#include "rocket-test/rocket-test.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(rocket, basicTypes) {
  static_assert(sizeof(bool) == 1);
  static_assert(sizeof(char) == 1);
  static_assert(sizeof(char32) == 4);
  static_assert(sizeof(i8) == 1);
  static_assert(sizeof(u8) == 1);
  static_assert(sizeof(i16) == 2);
  static_assert(sizeof(u16) == 2);
  static_assert(sizeof(i32) == 4);
  static_assert(sizeof(u32) == 4);
  static_assert(sizeof(i64) == 8);
  static_assert(sizeof(u64) == 8);
  static_assert(sizeof(i128) == 16);
  static_assert(sizeof(u128) == 16);
  static_assert(sizeof(f32) == 4);
  static_assert(sizeof(f64) == 8);
  static_assert(sizeof(void*) == 8);

  static_assert(is_signed_v<char>);
  static_assert(is_unsigned_v<char32>);
  static_assert(is_same_v<u64, std_size_t>);
  static_assert(is_signed_v<i128>);
  static_assert(is_unsigned_v<u128>);
  static_assert(is_same_v<decltype(1.0F), f32>);
  static_assert(is_same_v<decltype(1.0), f64>);
}

// EOF
