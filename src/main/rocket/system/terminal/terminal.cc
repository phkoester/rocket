/*
 * terminal.cc
 */

#include "terminal.h"

#include "rocket/Guard.h"
#include "rocket/InputFailure.h"
#include "rocket/assert.h"
#include "rocket/numeric.h"
#include "rocket/scan/scan.h"

#include <termios.h>
#include <sys/ioctl.h>

using namespace rocket::system::terminal;
using namespace std;

namespace rocket::system::terminal {

// `Ansi` ---------------------------------------------------------------------------------------------------

string
Ansi::clear() const {
  return active_ ? "\ec" : string();
}

string
Ansi::down(i32 n) const {
  return active_ ? fmt::format("\e[{}B", n) : string();
}

string
Ansi::left(i32 n) const {
  return active_ ? fmt::format("\e[{}D", n) : string();
}

string
Ansi::move(i32 column, i32 line) const {
  return active_ ? fmt::format("\e[{};{}H", line, column) : string();
}

string
Ansi::request(nio::Sink& out, string_view sequence) const {
  if (not active_)
    return string();

  // Save current `STDIN` settings

  termios oldT, newT;
  tcgetattr(STDIN_FILENO, &oldT);
  newT = oldT;

  // Disable echo and canonical input on `STDIN`

  newT.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSANOW, &newT);
  ROCKET_GUARD([&] { tcsetattr(STDIN_FILENO, TCSANOW, &oldT); });

  // Send the ANSI code requesting cursor position

  out.write(sequence);
  out.flush();

  // Read the response

  string ret;
  while (true) {
    char c;
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

string
Ansi::right(i32 n) const {
  return active_ ? fmt::format("\e[{}C", n) : string();
}

string
Ansi::up(i32 n) const {
  return active_ ? fmt::format("\e[{}A", n) : string();
}

// Functions ------------------------------------------------------------------------------------------------

optional<pair<u64, u64>>
position(nio::Sink& out) {
  i32 fd;
  if (not out.terminal(&fd)) {
    return nullopt;
  }

  // Send the ANSI code requesting cursor position

  Ansi ansi(true); // We know the sink is connected to a terminal
  string response = ansi.request(out, "\e[6n");

  // Scan the response

  auto result = scn::scan<u64, u64>(response, "\e[{};{}R");
  ROCKET_EXPECT(result, "Cannot scan response {:?}", response);
  auto[y, x] = result->values();

  // Done

  return make_pair(x, y);
}

optional<pair<u64, u64>>
size(nio::Io& io) {
  i32 fd;
  if (not io.terminal(&fd)) {
    return nullopt;
  }

  winsize ws;
  i32 res = ioctl(fd, TIOCGWINSZ, &ws);
  if (res != 0) {
    return nullopt;
  }
  return make_pair(to<u64>(ws.ws_col), to<u64>(ws.ws_row));
}

} // namespace rocket::system::terminal

// EOF
