/*
 * bench-nio.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/nio/nio.h"

#include "rocket-gtest/bench.h"

#include <fstream>

using namespace rocket;
using namespace rocket::gtest;
using namespace rocket::nio;
using namespace std;
using namespace std::filesystem;

// Constants ------------------------------------------------------------------------------------------------

constexpr size_t FILE_SIZE = 32 * 1'204 * 1'024; // 256 MiB
constexpr size_t CHUNK_SIZE = 1'024;
constexpr size_t N = 10;
constexpr size_t ITERATIONS = FILE_SIZE / CHUNK_SIZE;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(nio, FileSink) {
  string chunk(CHUNK_SIZE, ' '); // cppcheck-suppress variableScope
  auto tmp = ROCKET_GTEST_TEMP_PATH();

  ROCKET_BENCH(N, [&] {
    {
      FileSink out(tmp);
      for (size_t i = 0; i < ITERATIONS; ++i) {
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
      for (size_t i = 0; i < ITERATIONS; ++i) {
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
      for (size_t i = 0; i < ITERATIONS; ++i) {
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
      for (size_t i = 0; i < ITERATIONS; ++i) {
        buffered.write(chunk);
      }
    }
    EXPECT_EQ(file_size(tmp), FILE_SIZE);
  });
}

// EOF
