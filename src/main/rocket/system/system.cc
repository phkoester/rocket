/*
 * system.cc
 */

#include "system.h"

#include "rocket/InputFailure.h"
#include "rocket/assert.h"

#include <cstdlib>
#include <cstdio>
#include <memory>
#include <optional>

using namespace rocket;
using namespace std;

// Global variables -----------------------------------------------------------------------------------------

#ifndef ROCKET_OS_WINDOWS
extern char** environ; // NOLINT
#endif
namespace {

// Local functions ------------------------------------------------------------------------------------------

recursive_mutex&
envMutex() {
  static recursive_mutex mutex;
  return mutex;
}

#ifdef ROCKET_OS_WINDOWS

/**
 * Windows: Quotes an argument such that the C runtime of the called program reads it back unchanged.
 *
 * Backslashes are only special in front of a quotation mark: there, they must be doubled. This includes the
 * closing quotation mark. A quotation mark inside the argument is escaped with one more backslash.
 */
string
quoteArg(string_view arg) {
  string ret = "\"";
  u64 backslashes = 0;

  for (const char c : arg) {
    if (c == '\\') {
      ++backslashes; // Decide how to write them when we see what follows
      continue;
    }
    ret.append(c == '"' ? 2 * backslashes + 1 : backslashes, '\\');
    ret.push_back(c);
    backslashes = 0;
  }

  ret.append(2 * backslashes, '\\');
  ret.push_back('"');
  return ret;
}

/**
 * Windows: Escapes a string for `cmd.exe`, which `_popen` runs the command line with.
 *
 * Quotation marks are escaped as well, so `cmd.exe` never enters its quoted mode, in which the following
 * metacharacters would be interpreted differently. The carets are gone by the time the program is started.
 */
string
escapeCmd(string_view str) {
  constexpr string_view METACHARS = "()[]%!^\"`<>&|;, *?";

  string ret;
  ret.reserve(str.size());
  for (const char c : str) {
    if (METACHARS.contains(c)) {
      ret.push_back('^');
    }
    ret.push_back(c);
  }
  return ret;
}

/**
 * Windows: Converts arguments to a command-line string that passes each argument literally.
 *
 * The first argument names the program. `cmd.exe` passes the program token on to the program as it stands,
 * so a path with spaces has to be in real quotation marks for the C runtime to read it as one argument. An
 * escaped space would not do: the caret is gone by then, and the path would be split at the space.
 *
 * With a quotation mark at the start of the line, `cmd.exe /c` removes the first and the last quotation mark
 * of the line. The whole command line is therefore enclosed in one more pair for it to remove.
 *
 * Line breaks cannot be passed at all: `cmd.exe` stops reading the command line at the first one.
 */
string
makeCl(const vector<string_view>& args) {
  string ret = "\"";

  for (u64 i = 0; i < args.size(); ++i) {
    const string_view arg = args[i];
    ROCKET_CHECK(args, not arg.contains('\0'), "May not contain NUL characters");
    ROCKET_CHECK(
      args,
      not arg.contains('\n') && not arg.contains('\r'),
      "May not contain line breaks on Windows");

    if (i == 0) {
      // Inside real quotation marks, a further one would end the quoting and expose the rest to `cmd.exe`
      ROCKET_CHECK(args, not arg.contains('"'), "Program may not contain quotation marks on Windows");
      ret.append(quoteArg(arg));
    } else {
      ret.push_back(' ');
      ret.append(escapeCmd(quoteArg(arg)));
    }
  }

  ret.push_back('"');
  return ret;
}

#else

/**
 * Non-Windows: Converts arguments to a command-line string that passes each argument literally.
 *
 * `popen` hands the command line to `/bin/sh`. Enclosing each argument in single quotes turns off all
 * interpretation by the shell, including variable expansion, command substitution, operators, and globbing.
 * It also preserves empty arguments as `''`. Only a single quote itself cannot appear inside single quotes,
 * so it is written as `'\''`: close the quotes, add an escaped single quote, and reopen the quotes.
 */
string
makeCl(const vector<string_view>& args) {
  string ret;

  for (const auto& arg : args) {
    ROCKET_CHECK(args, not arg.contains('\0'), "May not contain NUL characters");

    if (not ret.empty()) {
      ret.push_back(' ');
    }

    ret.push_back('\'');
    for (const char c : arg) {
      if (c == '\'') {
        ret.append("'\\''");
      } else {
        ret.push_back(c);
      }
    }
    ret.push_back('\'');
  }

  return ret;
}

#endif

} // namespace

namespace rocket::system {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

optional<string>
getImpl(std::string_view name) {
  ROCKET_MUTEX_LOCK(envMutex());

  const string nameStr(name);

#ifdef ROCKET_OS_WINDOWS
  size_t size = 0;
  getenv_s(&size, nullptr, 0, nameStr.c_str());
  if (size == 0) {
    return nullopt;
  }
  string ret(size - 1, '\0');
  getenv_s(&size, ret.data(), size, nameStr.c_str());
  return ret;
#else
  const char* p = getenv(nameStr.c_str()); // NOLINT(concurrency-*)
  if (p == nullptr) {
    return nullopt;
  }
  return p;
#endif
}

void
setImpl(std::string_view name, const optional<string>& val, bool replace) {
  ROCKET_CHECK(val, val || replace);

  ROCKET_MUTEX_LOCK(envMutex());

  const string nameStr(name);

#ifdef ROCKET_OS_WINDOWS
  if (val && not replace && env::get<string>(name)) {
    return;
  }
  // Set/unset
  _putenv_s(nameStr.c_str(), val ? val->c_str() : "");
#else
  if (val) {
    // Set
    setenv(nameStr.c_str(), val->c_str(), replace ? 1 : 0); // NOLINT(concurrency-*)
  } else {
    // Unset
    unsetenv(const_cast<char*>(nameStr.c_str())); // NOLINT(concurrency-*)
  }
#endif
}

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

#ifndef ROCKET_OS_WINDOWS

string
exec(const string& cl) {
  string ret;
  vector<char> buf(1'024);

  const auto closePipe = [](FILE* file) { pclose(file); };
  const unique_ptr<FILE, decltype(closePipe)> pipe(popen(cl.c_str(), "r"), closePipe);
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

#endif

string
exec(const vector<string_view>& args) {
  return exec(makeCl(args));
}

vector<string>
makeArgs(string_view cl) { // NOLINT(*-complexity)
  vector<string> ret;

  enum State : u8 { NORMAL, SINGLE, DOUBLE };
  State state = NORMAL;
  string arg;

  for (size_t i = 0, size = cl.size(); i < size; ++i) {
    const char c = cl[i];
    optional<char> next;
    if (i < size - 1) {
      next = cl[i + 1];
    }

    switch (state) {
      case NORMAL:
        if (isspace(c) != 0) {
          if (not arg.empty()) {
            ret.push_back(arg);
            arg.clear();
          }
        } else if (c == '\'') {
          state = SINGLE;
        } else if (c == '"') {
          state = DOUBLE;
        } else if (c == '\\') {
          if (not next) {
            throw InputFailure(i, "Invalid escape sequence");
          }
          arg.push_back(*next);
          ++i;
        } else {
          arg.push_back(c);
        }
        break;
      case SINGLE:
        if (c == '\'') {
          state = NORMAL;
        } else {
          arg.push_back(c);
        }
        break;
      case DOUBLE:
        if (c == '"') {
          state = NORMAL;
        } else if (c == '\\') {
          if (not next) {
            throw InputFailure(i, "Invalid escape sequence");
          }
          arg.push_back(*next);
          ++i;
        } else {
          arg.push_back(c);
        }
        break;
    }
  }

  if (state == SINGLE) {
    throw InputFailure(cl.size(), "Unterminated single quote");
  }
  if (state == DOUBLE) {
    throw InputFailure(cl.size(), "Unterminated double quote");
  }

  if (not arg.empty()) {
    ret.push_back(arg);
  }

  return ret;
}

namespace env {

// Environment ----------------------------------------------------------------------------------------------

// Functions ................................................................................................

unordered_map<string, string>
get() {
  ROCKET_MUTEX_LOCK(envMutex());

  unordered_map<string, string> ret;

#ifdef ROCKET_OS_WINDOWS
  char** p = _environ; // NOLINT
#else
  char** p = environ; // NOLINT
#endif

  while (*p != nullptr) {
    const string_view entry(*p);
    string_view name, val;
    auto eq = entry.find('=');
    if (eq == string_view::npos) {
      name = entry;
    } else {
      name = entry.substr(0, eq);
      val = entry.substr(eq + 1);
    }
    ret.emplace(name, val);
    ++p;
  }

  return ret;
}

} // namespace env

} // namespace rocket::system

// EOF
