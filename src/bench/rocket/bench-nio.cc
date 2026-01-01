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
constexpr size_t N = 32;
constexpr size_t ITERATIONS = FILE_SIZE / CHUNK_SIZE;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(bench_nio, FileSink) {
  string chunk(CHUNK_SIZE, ' ');
  auto tmp = tempPath();

  ROCKET_BENCH(N, [&] {
    {
      FileSink sink(tmp);
      for (size_t i = 0; i < ITERATIONS; ++i) {
        sink.write(chunk);
      }
    }
    EXPECT_EQ(file_size(tmp), FILE_SIZE);
  });
}

TEST(bench_nio, BufferedFileSink) {
  string chunk(CHUNK_SIZE, ' ');
  auto tmp = tempPath();

  ROCKET_BENCH(N, [&] {
    {
      FileSink sink(tmp);
      std::setbuf(sink.file_, nullptr); // Disable buffering
      BufferedSink buffered(sink);
      for (size_t i = 0; i < ITERATIONS; ++i) {
        buffered.write(chunk);
      }
    }
    EXPECT_EQ(file_size(tmp), FILE_SIZE);
  });
}

TEST(bench_nio, StreamSink) {
  string chunk(CHUNK_SIZE, ' ');
  auto tmp = tempPath();

  ROCKET_BENCH(N, [&] {
    {
      ofstream os(tmp.c_str());
      StreamSink sink(os);
      for (size_t i = 0; i < ITERATIONS; ++i) {
        sink.write(chunk);
      }
    }
    EXPECT_EQ(file_size(tmp), FILE_SIZE);
  });
}

TEST(bench_nio, BufferedStreamSink) {
  string chunk(CHUNK_SIZE, ' ');
  auto tmp = tempPath();

  ROCKET_BENCH(N, [&] {
    {
      ofstream os(tmp.c_str());
      os.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering
      StreamSink sink(os);
      BufferedSink buffered(sink);
      for (size_t i = 0; i < ITERATIONS; ++i) {
        buffered.write(chunk);
      }
    }
    EXPECT_EQ(file_size(tmp), FILE_SIZE);
  });
}

// EOF
