/*
 * terminal.cc
 */

#include "terminal.h"

#include "Guard.h"
#include "assert.h"
#include "io.h"

#include <termios.h>
#include <sys/ioctl.h>

using namespace rocket::terminal;
using namespace std;

// Local functions ------------------------------------------------------------------------------------------

namespace {

string
styleCode(int i, bool fg) {
  bool bold = (i & Style::bold) != 0;
  bool high = (i & Style::high) != 0;
  bool underline = (i & Style::underline) != 0;
  i &= ~(Style::bold | Style::high | Style::underline);

  if (not fg)
    i += 10;
  if (high)
    i += 60;

  string ret = fmt::format("\e[{}", i);
  if (bold)
    ret += ";1";
  if (underline)
    ret += ";4";
  ret.push_back('m');
  return ret;
}

} // namespace

namespace rocket::terminal {

// `Ansi` ---------------------------------------------------------------------------------------------------

string
Ansi::clear() const {
  return active_ ? "\ec" : string();
}

string
Ansi::down(int n) const {
  return active_ ? fmt::format("\e[{}B", n) : string();
}

string
Ansi::left(int n) const {
  return active_ ? fmt::format("\e[{}D", n) : string();
}

string
Ansi::move(int column, int line) const {
  return active_ ? fmt::format("\e[{};{}H", line, column) : string();
}

string
Ansi::request(nio::Sink& sink, string_view sequence) const {
  // XXX
  nio::FileSink* fileSink = dynamic_cast<nio::FileSink*>(&sink);
  ROCKET_CHECK(sink, fileSink && (fileSink->fd() == STDOUT_FILENO || fileSink->fd() == STDERR_FILENO));

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

  sink.write(sequence);
  sink.flush();

  // Read the response

  string ret;
  while (true) {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
      throw io::InputFailure(cin, ret.size(), "Failed to read response");
    }
    ret.push_back(c);
    if (c == 'R') {
      break;
    }
  }
  return ret;
}

string
Ansi::right(int n) const {
  return active_ ? fmt::format("\e[{}C", n) : string();
}

string
Ansi::style(int fg) const {
  return active_ ? styleCode(fg, true) : string();
}

string
Ansi::style(int fg, int bg) const {
  return active_ ? styleCode(fg, true) + styleCode(bg, false) : string();
}

string
Ansi::up(int n) const {
  return active_ ? fmt::format("\e[{}A", n) : string();
}

// Functions ------------------------------------------------------------------------------------------------

optional<pair<size_t, size_t>>
position(nio::Sink& sink) {
  if (not isatty(sink.fd())) {
    return nullopt;
  }

  // Send the ANSI code requesting cursor position

  Ansi ansi(true); // We know that the sink is connected to a terminal
  string response = ansi.request(sink, "\e[6n");

  // Parse the response

  int x, y;
  auto sscanfResult = sscanf(response.c_str(), "\e[%d;%dR", &y, &x);
  ROCKET_EXPECT(sscanfResult == 2);

  // Done

  return make_pair(x, y);
}

optional<pair<size_t, size_t>>
size(nio::Sink& sink) {
  int fd = sink.fd();
  if (not isatty(fd)) {
    return nullopt;
  }

  winsize ws;
  int res = ioctl(fd, TIOCGWINSZ, &ws);
  if (res != 0) {
    return nullopt;
  }
  return make_pair(ws.ws_col, ws.ws_row);
}

} // namespace rocket::terminal

// EOF
