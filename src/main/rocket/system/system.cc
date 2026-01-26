/*
 * system.cc
 */

#include "system.h"

#include "rocket/assert.h"

#include <array>
#include <memory>
#ifdef ROCKET_OS_WINDOWS
#include <Windows.h>
#endif

using namespace rocket;
using namespace std;

// Macros ---------------------------------------------------------------------------------------------------

#ifdef ROCKET_OS_WINDOWS
  #define PCLOSE _pclose
  #define POPEN _popen
#else
  #define PCLOSE pclose
  #define POPEN popen
#endif

namespace {

// Local functions ------------------------------------------------------------------------------------------

#ifdef ROCKET_OS_WINDOWS

/**
 * Windows: Convert arguments to a command-line string. The rules are:
 *
 * - Replace a space by `" "`
 * - Replace a quotation mark by `\"`
 * - Replace a backslash followed by quotation mark by `"\\"`
 *
 * @todo Use a generalized escaping mechanism
 */
string
makeCl(const vector<string_view>& args) {
  string ret;

  for (const auto& arg : args) {
    if (not ret.empty())
      ret.push_back(' ');

    for (u64 i = 0, size = arg.size(); i < size; ++i) {
      char c = arg[i];
      optional<char> next;
      if (i < size - 1)
        next = arg[i + 1];

      if (c == ' ')
        ret.append("\" \"");
      else if (c == '"')
        ret.append("\\\"");
      else if (c == '\\' && next && *next == '"')
        ret.append("\"\\\\\"");
      else
        ret.push_back(c);
    }
  }

  return ret;
}

#else

/**
 * Non-Windows: Convert arguments to a command-line string. The rules are:
 *
 * - Escape a space with a backslash
 * - Escape a quotation mark with a backslash
 * - Escape an apostrophe with a backslash
 * - Escape a backslash with a backslash
 *
 * @todo Use a generalized escaping mechanism
 */
string
makeCl(const vector<string_view>& args) {
  string ret;

  for (const auto& arg : args) {
    if (not ret.empty())
      ret.push_back(' ');

    for (char c : arg) {
      if (c == ' ')
        ret.append("\\ ");
      else if (c == '"')
        ret.append("\\\"");
      else if (c == '\'')
        ret.append("\\'");
      else if (c == '\\')
        ret.append("\\\\");
      else
        ret.push_back(c);
    }
  }

  return ret;
}

#endif

} // namespace

namespace rocket::system {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

recursive_mutex envMutex;

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

vector<std::byte> // MSVC needs `std::` prefix
exec(const string& cl) {
  vector<std::byte> ret;
  array<std::byte, 128> buf;

  unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(cl.c_str(), "r"), PCLOSE);
  if (not pipe) {
    ROCKET_FAIL("Cannot open pipe for command `{}`", cl);
  }
  u64 n;
  while ((n = fread(buf.data(), 1, buf.size(), pipe.get())) > 0) {
    ret.insert(ret.end(), buf.begin(), buf.begin() + n);
  }
  if (ferror(pipe.get()) != 0) {
    ROCKET_FAIL("Cannot read from pipe for command `{}`", cl);
  }

  return ret;
}

vector<std::byte>
exec(const vector<string_view>& args) {
  return exec(makeCl(args));
}

string_view
executableSuffix() {
#ifdef ROCKET_OS_WINDOWS
  return ".exe";
#else
  return string_view();
#endif
}

char
fileSeparator() {
#ifdef ROCKET_OS_WINDOWS
  return '\\';
#else
  return '/';
#endif
}

char
pathSeparator() {
#ifdef ROCKET_OS_WINDOWS
  return ';';
#else
  return ':';
#endif
}

namespace env {

// Environment ----------------------------------------------------------------------------------------------

unordered_map<string, string>
get() {
  ROCKET_MUTEX_LOCK(internal::envMutex);

  unordered_map<string, string> ret;

#ifndef ROCKET_OS_WINDOWS
  // GNU C
  char** p = __environ;
  while(*p != nullptr) {
    string_view entry(*p);
    auto eq = entry.find('=');
    string_view name, value;
    if (eq == string_view::npos) {
      name = entry;
    } else {
      name = entry.substr(0, eq);
      value = entry.substr(eq + 1);
    }
    ret.emplace(name, value);
    ++p;
  }
#else
  // Windows
  unique_ptr<CHAR, function<void(LPCH)>> env(GetEnvironmentStrings(), [](LPCH p) {
    ROCKET_ASSERT(FreeEnvironmentStrings(p));
  });
  LPSTR p = reinterpret_cast<LPSTR>(env.get());
  while (p != nullptr && *p != '\0') {
    string_view entry(p);
    auto eq = entry.find('=');
    string_view name, value;
    if (eq == string_view::npos) {
      name = entry;
    } else {
      name = entry.substr(0, eq);
      value = entry.substr(eq + 1);
    }
    ret.emplace(name, value);
    p += entry.size() + 1;
  }
#endif
  return ret;
}

void
unset(const string& name) {
  ROCKET_MUTEX_LOCK(internal::envMutex);

  putenv(const_cast<char*>(name.c_str()));
}

} // namespace env

} // namespace rocket::system

// EOF
