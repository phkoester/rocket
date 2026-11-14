/*
 * bench-nio.cc
 */

#include "rocket-bench/rocket-bench.h"

#include "rocket/filesystem/filesystem.h"
#include "rocket/nio/nio-utils.h"
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
      out.write(CHUNK);
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
      out.write(CHUNK);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
}
BENCHMARK(nio_StreamSink);

// `FileSource` .............................................................................................

namespace {

/// Returns a temporary file with @p n copies of @p line.
std::filesystem::path
tempFileWithLines(string_view line, u64 n) {
  auto ret = rocket::filesystem::tempFile();
  FileSink out(ret);
  for (u64 i = 0; i < n; ++i) {
    out.write(line);
  }
  return ret;
}

} // namespace

void
nio_FileSourceReadln(benchmark::State& state) { // NOLINT(*-internal-linkage)
  // Locks the `FILE` once per line, not once per character
  const auto temp = tempFileWithLines("The quick brown fox jumps over the lazy dog, again and again.\n", 16'384);
  for (auto _ : state) { // NOLINT
    FileSource in(temp);
    u64 n = 0;
    while (in.readln()) {
      ++n;
    }
    ROCKET_EXPECT(n == 16'384);
  }
}
BENCHMARK(nio_FileSourceReadln);

void
nio_FileSourceReadCodePoint(benchmark::State& state) { // NOLINT(*-internal-linkage)
  // Asks for the position once per code point
  const auto temp = tempFileWithLines("Grüße aus Köln, €42 \U0001F600\n", 4'096);
  for (auto _ : state) { // NOLINT
    FileSource in(temp);
    u64 n = 0;
    while (readCodePoint(in)) {
      ++n;
    }
    benchmark::DoNotOptimize(n);
  }
}
BENCHMARK(nio_FileSourceReadCodePoint);

void
nio_FileSourceSkip(benchmark::State& state) { // NOLINT(*-internal-linkage)
  // Asks for the position once per whitespace character
  const auto temp = tempFileWithLines("        // comment\n\t\t\n", 8'192);
  for (auto _ : state) { // NOLINT
    FileSource in(temp);
    skip(in, true, false);
    ROCKET_EXPECT(in.eof());
  }
}
BENCHMARK(nio_FileSourceSkip);

// EOF
