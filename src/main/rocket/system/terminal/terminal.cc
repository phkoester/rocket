/*
 * terminal.cc
 */

#include "terminal.h"

#include "rocket/assert.h"
#include "rocket/Guard.h"
#include "rocket/InputFailure.h"
#include "rocket/io/io.h"

#include <boost/safe_numerics/safe_integer.hpp>

#include <scn/scan.h>

#ifdef ROCKET_OS_WINDOWS
#include <Windows.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#endif

using namespace rocket;
using namespace rocket::system::terminal;
using namespace std;

using boost::safe_numerics::safe;

namespace {

// Local functions ------------------------------------------------------------------------------------------

#ifndef ROCKET_OS_WINDOWS

/**
 * Writes an ANSI escape sequence to a device and returns the response from `stdin`.
 *
 * @param out the sink to write to
 * @param sequence the ANSI escape sequence to write
 * @return the response from `stdin` if this instance is active, otherwise an empty string
 * @throw #rocket::InputFailure if the response from `stdin` cannot be read
 */
string
sendAnsiRequest(nio::Sink& out, std::string_view sequence) {
  // Save current `stdin` settings

  termios oldT, newT; // NOLINT
  tcgetattr(STDIN_FILENO, &oldT);
  newT = oldT;

  // Disable echo and canonical input on `stdin`

  newT.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSANOW, &newT);
  ROCKET_GUARD([&] { tcsetattr(STDIN_FILENO, TCSANOW, &oldT); });

  // Send the ANSI code requesting cursor position

  out.write(sequence);
  out.flush();

  // Read the response

  string ret;
  while (true) {
    char c = '\0';
    if (::read(STDIN_FILENO, &c, 1) != 1) {
      throw InputFailure(ret.size(), "Failed to read response");
    }
    ret.push_back(c);
    if (c == 'R') {
      break;
    }
  }
  return ret;
}

#else

HANDLE
getHandle(i32 fd) {
  switch (fd) {
  case STDIN_FILENO: return GetStdHandle(STD_INPUT_HANDLE);
  case STDOUT_FILENO: return GetStdHandle(STD_OUTPUT_HANDLE);
  case STDERR_FILENO: return GetStdHandle(STD_ERROR_HANDLE);
  default: return INVALID_HANDLE_VALUE;
  }
}

#endif // ROCKET_OS_WINDOWS

} // namespace

namespace rocket::system::terminal {

// #Ansi ----------------------------------------------------------------------------------------------------

string
Ansi::clear() const {
  return active_ ? "\x1b" "c" : string();
}

string
Ansi::down(i32 n) const {
  return active_ ? fmt::format("\x1b[{}B", n) : string();
}

string
Ansi::left(i32 n) const {
  return active_ ? fmt::format("\x1b[{}D", n) : string();
}

string
Ansi::move(i32 column, i32 line) const {
  return active_ ? fmt::format("\x1b[{};{}H", line, column) : string();
}

string
Ansi::right(i32 n) const {
  return active_ ? fmt::format("\x1b[{}C", n) : string();
}

string
Ansi::up(i32 n) const {
  return active_ ? fmt::format("\x1b[{}A", n) : string();
}

// Functions ------------------------------------------------------------------------------------------------

optional<pair<u64, u64>>
position(nio::Sink& out) { // NOLINT
  const i32 fd = out.handle();
  if (fd == -1 || ROCKET_ISATTY(fd) == 0) {
    return {};
  }

#ifdef ROCKET_OS_WINDOWS
  // Use CSBI

  auto handle = getHandle(fd);
  ROCKET_ASSERT(handle != INVALID_HANDLE_VALUE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (not GetConsoleScreenBufferInfo(handle, &csbi)) {
    return {};
  }
  return make_pair(safe<u64>(csbi.dwCursorPosition.X + 1), safe<u64>(csbi.dwCursorPosition.Y + 1));
#else
  // Send the ANSI code requesting cursor position

  const string response = sendAnsiRequest(out, "\x1b[6n");

  // Scan the response

  const auto result = scn::scan<u64, u64>(response, "\x1b[{};{}R");
  ROCKET_EXPECT(result, "Cannot scan response {:?}", response);
  const auto [y, x] = result->values();

  // Done

  return make_pair(x, y);
#endif
}

optional<pair<u64, u64>>
size(nio::Device& device) {
  const i32 fd = device.handle();
  if (fd == -1 || ROCKET_ISATTY(fd) == 0) {
    return {};
  }

#ifdef ROCKET_OS_WINDOWS
  // Use CSBI

  auto handle = getHandle(fd);
  ROCKET_ASSERT(handle != INVALID_HANDLE_VALUE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (not GetConsoleScreenBufferInfo(handle, &csbi)) {
    return {};
  }
  return make_pair(safe<u64>(csbi.dwSize.X), safe<u64>(csbi.dwSize.Y));
#else
  // Use #ioctl

  winsize ws; // NOLINT;
  const i32 res = ioctl(fd, TIOCGWINSZ, &ws); // NOLINT
  if (res != 0) {
    return {};
  }
  return make_pair(safe<u64>(ws.ws_col), safe<u64>(ws.ws_row));
#endif
}

} // namespace rocket::system::terminal

// EOF
