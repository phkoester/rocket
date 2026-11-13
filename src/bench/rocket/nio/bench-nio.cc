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

const string CHUNK(CHUNK_SIZE, ' ');

// `BENCH` --------------------------------------------------------------------------------------------------

void
nio_FileSink(benchmark::State& state) { // NOLINT(*-internal-linkage)
  const auto temp = rocket::filesystem::tempFile();
  for (auto _ : state) { // NOLINT
    FileSink out(temp);
    for (u64 i = 0; i < N; ++i) {
      out.Sink::write(CHUNK);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
}
BENCHMARK(nio_FileSink);

void
nio_StreamSink(benchmark::State& state) { // NOLINT(*-internal-linkage)
  const auto temp = rocket::filesystem::tempFile();
  for (auto _ : state) { // NOLINT
    ofstream os(temp.c_str());
    StreamSink out(os);
    for (u64 i = 0; i < N; ++i) {
      out.Sink::write(CHUNK);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
}
BENCHMARK(nio_StreamSink);

// EOF
