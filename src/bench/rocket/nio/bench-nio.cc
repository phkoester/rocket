/*
 * bench-nio.cc
 */

// Early macros ---------------------------------------------------------------------------------------------

#undef ROCKET_TEST_PROTECTED
/// Use this macro instead of `protected` to allow access to protected members of a class when testing.
#define ROCKET_TEST_PROTECTED public
#undef ROCKET_TEST_PRIVATE
/// Use this macro instead of `private` to allow access to private members of a class when testing.
#define ROCKET_TEST_PRIVATE public

// Includes -------------------------------------------------------------------------------------------------

#include "rocket/rocket.h"
#include "rocket/math/random.h"
#include "rocket/nio/nio.h"
#include "rocket/system/system.h"

#include <benchmark/benchmark.h>

#include <filesystem>
#include <fstream>

using namespace rocket;
using namespace rocket::nio;
using namespace std;
using namespace std::filesystem;

// Constants ------------------------------------------------------------------------------------------------

constexpr u64 FILE_SIZE = 1'204 * 1'024; // 1 MiB
constexpr u64 CHUNK_SIZE = 1'024;
constexpr u64 ITERATIONS = FILE_SIZE / CHUNK_SIZE;

path
tempFile() {
  string file = ROCKET_SRC_FILE;
  std::replace(file.begin(), file.end(), system::fileSeparator(), '_');

  auto gen = math::gen();
  path name = fmt::format(
      "rocket-bench-{}-{}.tmp",
      file, math::randomHex(gen, 16));
  path ret = temp_directory_path() / name;

  // process.atExit([=] { remove(ret); }, true);

  return ret;
}

// #BENCHMARK -----------------------------------------------------------------------------------------------

static void
nio_FileSink(benchmark::State& state) {
  string chunk(CHUNK_SIZE, ' ');
  auto temp = tempFile();

  for (auto _ : state) {
    FileSink out(temp.string());
    for (u64 i = 0; i < ITERATIONS; ++i) {
      out.write(chunk);
    }
  }
  ROCKET_EXPECT(file_size(temp) == FILE_SIZE);
}

BENCHMARK(nio_FileSink);

static void
nio_BufferedFileSink(benchmark::State& state) {
  string chunk(CHUNK_SIZE, ' '); // cppcheck-suppress variableScope
  auto tmp = tempFile();

  for (auto _ : state) {
    FileSink out(tmp.string());
    std::setbuf(out.file_, nullptr); // Disable buffering
    BufferedSink buffered(out);
    for (u64 i = 0; i < ITERATIONS; ++i) {
      buffered.write(chunk);
    }
  }
  ROCKET_EXPECT(file_size(tmp) == FILE_SIZE);
}

BENCHMARK(nio_BufferedFileSink);

static void
nio_StreamSink(benchmark::State& state) {
  string chunk(CHUNK_SIZE, ' '); // cppcheck-suppress variableScope
  auto tmp = tempFile();

  for (auto _ : state) {
    ofstream os(tmp.c_str());
    StreamSink out(os);
    for (u64 i = 0; i < ITERATIONS; ++i) {
      out.write(chunk);
    }
  }
  ROCKET_EXPECT(file_size(tmp) == FILE_SIZE);
}

BENCHMARK(nio_StreamSink);

static void
nio_BufferedStreamSink(benchmark::State& state) {
  string chunk(CHUNK_SIZE, ' '); // cppcheck-suppress variableScope
  auto tmp = tempFile();

 for (auto _ : state) {
    ofstream os(tmp.c_str());
    os.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering
    StreamSink out(os);
    BufferedSink buffered(out);
    for (u64 i = 0; i < ITERATIONS; ++i) {
      buffered.write(chunk);
    }
  }
  ROCKET_EXPECT(file_size(tmp) == FILE_SIZE);
}

BENCHMARK(nio_BufferedStreamSink);

// EOF
