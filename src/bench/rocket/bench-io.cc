/*
 * bench-io.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/io.h"

#include "rocket-gtest/bench.h"

using namespace rocket;
using namespace rocket::io;
using namespace std;

// Constants ------------------------------------------------------------------------------------------------

constexpr size_t INPUT_SIZE = 16 * 1'204 * 1'024; // 16 MiB
constexpr size_t N = 10;

// `TEST` ---------------------------------------------------------------------------------------------------

// `Buffer` .................................................................................................

/**
 * Compare with `io.is`.
 */
TEST(io, Buffer) {
  ROCKET_BENCH(N, [&] {
    string s = string(INPUT_SIZE, ' ');
    EXPECT_EQ(s.size(), INPUT_SIZE);
    auto is = io::is(s);
    Buffer buf(is);

    while (true) {
      auto got = buf.get();
      if (not got)
        break;
      EXPECT_EQ(*got, byte(' ')); // Slow with build type 'debug'
    }
    EXPECT_EQ(buf.position(), INPUT_SIZE);
  });
}

// Functions ................................................................................................

/**
 * Compare with `io.Buffer`.
 */
TEST(io, is) {
  ROCKET_BENCH(N, [&] {
    string s = string(INPUT_SIZE, ' ');
    EXPECT_EQ(s.size(), INPUT_SIZE);
    auto is = io::is(s);
    size_t n = 0;
    while (true) {
      char c = io::getChar(is);
      if (is.eof())
        break;
      EXPECT_EQ(c, ' ');
      ++n;
    }
    EXPECT_EQ(n, INPUT_SIZE);
  });
}

// EOF
