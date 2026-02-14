/*
 * test-hash.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/nio/nio.h"
#include "rocket/hash/hash.h"

using namespace rocket::hash;

// #TEST ----------------------------------------------------------------------------------------------------

TEST(hash, combineOrder) {
  u64 seed = 1;
  combine(seed, 2);
  combine(seed, 3);
  u64 hash1 = seed;

  seed = 3;
  combine(seed, 2);
  combine(seed, 1);
  u64 hash2 = seed;

  EXPECT_NE(hash1, hash2);
}

TEST(hash, printBoostHash) {
  using Hash = BoostHash;
  auto& out = nio::out;
  out.println("bool false: {}", Hash()(false));
  out.println("u8 42: {}", Hash()(42_u8));
  out.println("i32 43: {}", Hash()(43_i32));
  out.println("f64 45.67: {}", Hash()(45.67_f64));
  out.println("string \"hello\": {}", Hash()("hello"s));
  vector<i32> vec = { 1, 2, 3 };
  out.println("vector: {}", Hash()(vec));
}

TEST(hash, printStdHash) {
  using Hash = StdHash;
  auto& out = nio::out;
  out.println("bool false: {}", Hash()(false));
  out.println("u8 42: {}", Hash()(42_u8));
  out.println("i32 43: {}", Hash()(43_i32));
  out.println("f64 45.67: {}", Hash()(45.67_f64));
  out.println("string \"hello\": {}", Hash()("hello"s));
}

// EOF
