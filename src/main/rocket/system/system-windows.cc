/*
 * system-windows.cc
 */

#include "rocket/assert.h"
#include "rocket/nio/nio.h"

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include <Windows.h>

using namespace rocket;
using namespace std;

namespace {

// `ConsoleModeGuard` ---------------------------------------------------------------------------------------

/**
 * A guard that restores the console mode of `stdin` and `stdout`.
 */
struct ConsoleModeGuard {
  ConsoleModeGuard() {
    for (auto& entry : entries_) {
      entry.handle = CreateFileA(
        entry.name,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);
      if (entry.handle == INVALID_HANDLE_VALUE) {
        continue; // No console attached
      }
      DWORD mode = 0;
      if (GetConsoleMode(entry.handle, &mode) != 0) {
        entry.mode = mode;
      }
    }
  }

  ConsoleModeGuard(const ConsoleModeGuard&) = delete;

  ~ConsoleModeGuard() {
    for (const auto& entry : entries_) {
      if (entry.handle == INVALID_HANDLE_VALUE) {
        continue;
      }
      if (entry.mode) {
        SetConsoleMode(entry.handle, *entry.mode);
      }
      CloseHandle(entry.handle);
    }
  }

  ConsoleModeGuard& operator=(const ConsoleModeGuard&) = delete;

private:

  struct Entry {
    const char* name;
    HANDLE handle = INVALID_HANDLE_VALUE;
    optional<DWORD> mode = {};
  };

  array<Entry, 2> entries_ { {
    { .name="CONIN$" },  // `stdin`
    { .name="CONOUT$" }, // `stdout` and `stderr` (same screen buffer)
  } };
};

} // namespace

namespace rocket::system {

// Functions ------------------------------------------------------------------------------------------------

string
exec(const string& cl) {
  string ret;
  vector<char> buf(1'024);

  // Auto-restore console modes that the pipe might compromise
  ConsoleModeGuard cmg;

  const auto closePipe = [](FILE* pipe) { _pclose(pipe); };
  unique_ptr<FILE, decltype(closePipe)> pipe(_popen(cl.c_str(), "r"), closePipe);
  if (not pipe) {
    ROCKET_FAIL("Cannot open pipe for command `{}`", cl);
  }
  u64 n; // NOLINT
  while ((n = fread(buf.data(), 1, buf.size(), pipe.get())) > 0) {
    ret.insert(ret.end(), buf.begin(), buf.begin() + n); // NOLINT
  }
  if (ferror(pipe.get()) != 0) {
    ROCKET_FAIL("Cannot read from pipe for command `{}`", cl);
  }

  // Close the pipe here rather than in the deleter, because only `_pclose` tells how the command ended
  const i32 status = _pclose(pipe.release());
  if (status != 0) {
    ROCKET_FAIL("Command `{}` failed with status {}", cl, status);
  }

  return ret;
}

} // namespace rocket::system

// EOF
