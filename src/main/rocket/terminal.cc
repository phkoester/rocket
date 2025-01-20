/*
 * terminal.cc
 */

#include "codec-std-decl.h"
#include "codec-std.h"

#include "terminal.h"

#include "system.h"

#include <format>
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

  string result = format("\e[{}", i);
  if (bold)
    result += ";1";
  if (underline)
    result += ";4";
  result.push_back('m');
  return result;
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
  return active_ ? format("\e[{}B", n) : string();
}

string
Ansi::left(int n) const {
  return active_ ? format("\e[{}D", n) : string();
}

string
Ansi::move(int line, int column) const {
  return active_ ? format("\e[{};{}H", line, column) : string();
}

string
Ansi::right(int n) const {
  return active_ ? format("\e[{}C", n) : string();
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
  return active_ ? format("\e[{}A", n) : string();
}

// Functions ------------------------------------------------------------------------------------------------

optional<pair<size_t, size_t>>
size(const basic_ios<char>& io) {
  int fd;
  if (&io == &cin)
    fd = STDIN_FILENO;
  else if (&io == &cout)
    fd = STDOUT_FILENO;
  else if (&io == &cerr)
    fd = STDERR_FILENO;
  else
    return nullopt;

  winsize ws;
  int res = ioctl(fd, TIOCGWINSZ, &ws);
  if (res != 0)
    return nullopt;
  return make_pair(ws.ws_col, ws.ws_row);
}

} // namespace rocket::terminal

// EOF
