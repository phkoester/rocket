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
  for (auto _ : state) {
    FileSink out(temp);
    for (u64 i = 0; i < N; ++i) {
      out.Sink::write(CHUNK);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
}
BENCHMARK(nio_FileSink);

void
nio_BufferedFileSink(benchmark::State& state) {
  const auto temp = rocket::filesystem::tempFile();
  for (auto _ : state) {
    FileSink out(temp);
    std::setvbuf(out.file_, nullptr, _IONBF, 0); // Disable buffering
    BufferedSink buffered(out);
    for (u64 i = 0; i < N; ++i) {
      buffered.Sink::write(CHUNK);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
}
BENCHMARK(nio_BufferedFileSink);

void
nio_StreamSink(benchmark::State& state) {
  const auto temp = rocket::filesystem::tempFile();
  for (auto _ : state) {
    ofstream os(temp.c_str());
    StreamSink out(os);
    for (u64 i = 0; i < N; ++i) {
      out.Sink::write(CHUNK);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
}
BENCHMARK(nio_StreamSink);

void
nio_BufferedStreamSink(benchmark::State& state) {
  const auto temp = rocket::filesystem::tempFile();
  for (auto _ : state) {
    ofstream os(temp.c_str());
    os.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering
    StreamSink out(os);
    BufferedSink buffered(out);
    for (u64 i = 0; i < N; ++i) {
      buffered.Sink::write(CHUNK);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
}
BENCHMARK(nio_BufferedStreamSink);

// EOF
