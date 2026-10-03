/*
 * system-windows.cc
 */

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include <Windows.h>

using namespace std;

namespace {

struct ConsoleModeGuard {
  ConsoleModeGuard() {
    for (auto& entry : entries_) {
      entry.handle = CreateFileA(entry.name, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
      if (entry.handle == INVALID_HANDLE_VALUE) {
        continue; // no console attached
      }
      DWORD mode = 0;
      if (GetConsoleMode(entry.handle, &mode) != 0) {
        entry.mode = mode;
      }
    }
  }

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

  ConsoleModeGuard(const ConsoleModeGuard&) = delete;

  ConsoleModeGuard& operator=(const ConsoleModeGuard&) = delete;

private:

  struct Entry {
    const char* name;
    HANDLE handle = INVALID_HANDLE_VALUE;
    std::optional<DWORD> mode;
  };

  std::array<Entry, 2> entries_ { {
    { .name="CONIN$" },  // stdin
    { .name="CONOUT$" }, // stdout and stderr (same screen buffer)
  } };
};

} // namespace

namespace rocket::system {

vector<char>
exec(const string& cl) {
  vector<char> ret;
  vector<char> buf(1'024);

  ConsoleModeGuard cmg;

  const unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cl.c_str(), "r"), _pclose);
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

  return ret;
}

} // namespace rocket::system

// EOF
