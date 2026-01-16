/*
 * bench-nio.cc
 */

#include "rocket-gtest/bench.h"

#include "rocket/nio/nio.h"

#include <fstream>

using namespace rocket::nio;

// Constants ------------------------------------------------------------------------------------------------

constexpr u64 FILE_SIZE = 32 * 1'204 * 1'024; // 256 MiB
constexpr u64 CHUNK_SIZE = 1'024;
constexpr u64 N = 10;
constexpr u64 ITERATIONS = FILE_SIZE / CHUNK_SIZE;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(nio, FileSink) {
  string chunk(CHUNK_SIZE, ' '); // cppcheck-suppress variableScope
  auto tmp = ROCKET_GTEST_TEMP_PATH();

  ROCKET_BENCH(N, [&] {
    {
      FileSink out(tmp);
      for (u64 i = 0; i < ITERATIONS; ++i) {
        out.write(chunk);
      }
    }
    EXPECT_EQ(file_size(tmp), FILE_SIZE);
  });
}

TEST(nio, BufferedFileSink) {
  string chunk(CHUNK_SIZE, ' '); // cppcheck-suppress variableScope
  auto tmp = ROCKET_GTEST_TEMP_PATH();

  ROCKET_BENCH(N, [&] {
    {
      FileSink out(tmp);
      std::setbuf(out.file_, nullptr); // Disable buffering
      BufferedSink buffered(out);
      for (u64 i = 0; i < ITERATIONS; ++i) {
        buffered.write(chunk);
      }
    }
    EXPECT_EQ(file_size(tmp), FILE_SIZE);
  });
}

TEST(nio, StreamSink) {
  string chunk(CHUNK_SIZE, ' '); // cppcheck-suppress variableScope
  auto tmp = ROCKET_GTEST_TEMP_PATH();

  ROCKET_BENCH(N, [&] {
    {
      ofstream os(tmp.c_str());
      StreamSink out(os);
      for (u64 i = 0; i < ITERATIONS; ++i) {
        out.write(chunk);
      }
    }
    EXPECT_EQ(file_size(tmp), FILE_SIZE);
  });
}

TEST(nio, BufferedStreamSink) {
  string chunk(CHUNK_SIZE, ' '); // cppcheck-suppress variableScope
  auto tmp = ROCKET_GTEST_TEMP_PATH();

  ROCKET_BENCH(N, [&] {
    {
      ofstream os(tmp.c_str());
      os.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering
      StreamSink out(os);
      BufferedSink buffered(out);
      for (u64 i = 0; i < ITERATIONS; ++i) {
        buffered.write(chunk);
      }
    }
    EXPECT_EQ(file_size(tmp), FILE_SIZE);
  });
}

// EOF
