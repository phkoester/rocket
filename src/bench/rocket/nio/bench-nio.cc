/*
 * bench-nio.cc
 */

#include "rocket-bench/rocket-bench.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/nio/nio.h"

#include <filesystem>
#include <fstream>

using namespace rocket::nio;

// Constants ------------------------------------------------------------------------------------------------

constexpr u64 FILE_SIZE = 1'204 * 1'024; // 1 MiB
constexpr u64 CHUNK_SIZE = 1'024;
constexpr u64 N = FILE_SIZE / CHUNK_SIZE;

// #BENCH ---------------------------------------------------------------------------------------------------

BENCH(nio, FileSink, {
  const string chunk(CHUNK_SIZE, ' ');
  const auto temp = rocket::filesystem::tempFile();

  for (auto _ : state) {
    FileSink out(temp.string());
    for (u64 i = 0; i < N; ++i) {
      out.write(chunk);
    }
  }

  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
})

BENCH(nio, BufferedFileSink, {
  const string chunk(CHUNK_SIZE, ' ');
  const auto temp = rocket::filesystem::tempFile();

  for (auto _ : state) {
    FileSink out(temp.string());
    std::setbuf(out.file_, nullptr); // Disable buffering
    BufferedSink buffered(out);
    for (u64 i = 0; i < N; ++i) {
      buffered.write(chunk);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
})

BENCH(nio, StreamSink, {
  const string chunk(CHUNK_SIZE, ' ');
  const auto temp = rocket::filesystem::tempFile();

  for (auto _ : state) {
    ofstream os(temp.c_str());
    StreamSink out(os);
    for (u64 i = 0; i < N; ++i) {
      out.write(chunk);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
})

BENCH(nio, BufferedStreamSink, {
  const string chunk(CHUNK_SIZE, ' ');
  const auto temp = rocket::filesystem::tempFile();

  for (auto _ : state) {
    ofstream os(temp.c_str());
    os.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering
    StreamSink out(os);
    BufferedSink buffered(out);
    for (u64 i = 0; i < N; ++i) {
      buffered.write(chunk);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
})

// EOF
