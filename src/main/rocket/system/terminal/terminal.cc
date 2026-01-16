/*
 * terminal.cc
 */

#include "terminal.h"

#include "rocket/Guard.h"
#include "rocket/InputFailure.h"
#include "rocket/assert.h"

#include <termios.h>
#include <sys/ioctl.h>

using namespace rocket::system::terminal;
using namespace std;

// Local functions ------------------------------------------------------------------------------------------

namespace {

string
styleCode(i32 i, bool fg) {
  bool bold = (i & Style::bold) != 0;
  bool high = (i & Style::high) != 0;
  bool underline = (i & Style::underline) != 0;
  i &= ~(Style::bold | Style::high | Style::underline);

  if (not fg) {
    i += 10;
  }
  if (high) {
    i += 60;
  }

  string ret = fmt::format("\e[{}", i);
  if (bold) {
    ret += ";1";
  }
  if (underline) {
    ret += ";4";
  }
  ret.push_back('m');
  return ret;
}

} // namespace

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
  ROCKET_CHECK(sink, out.fd() == STDOUT_FILENO || out.fd() == STDERR_FILENO);

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
Ansi::style(i32 fg) const {
  return active_ ? styleCode(fg, true) : string();
}

string
Ansi::style(i32 fg, i32 bg) const {
  return active_ ? styleCode(fg, true) + styleCode(bg, false) : string();
}

string
Ansi::up(i32 n) const {
  return active_ ? fmt::format("\e[{}A", n) : string();
}

// Functions ------------------------------------------------------------------------------------------------

optional<pair<u64, u64>>
position(nio::Sink& out) {
  if (not isatty(out.fd())) {
    return nullopt;
  }

  // Send the ANSI code requesting cursor position

  Ansi ansi(true); // We know the sink is connected to a terminal
  string response = ansi.request(out, "\e[6n");

  // Parse the response

  i32 x, y;
  auto sscanfResult = sscanf(response.c_str(), "\e[%d;%dR", &y, &x);
  ROCKET_EXPECT(sscanfResult == 2);

  // Done

  return make_pair(x, y);
}

optional<pair<u64, u64>>
size(nio::Sink& out) {
  i32 fd = out.fd();
  if (not isatty(fd)) {
    return nullopt;
  }

  winsize ws;
  i32 res = ioctl(fd, TIOCGWINSZ, &ws);
  if (res != 0) {
    return nullopt;
  }
  return make_pair(ws.ws_col, ws.ws_row);
}

} // namespace rocket::system::terminal

// EOF
