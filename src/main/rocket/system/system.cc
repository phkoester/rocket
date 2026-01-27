/*
 * system.cc
 */

#include "system.h"

#include "rocket/assert.h"

#include <array>
#include <cstdlib>
#include <cstdio>
#include <memory>

#include <iostream> // XXX

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

// Local variables ------------------------------------------------------------------------------------------

recursive_mutex envMutex;

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
    if (not ret.empty()) {
      ret.push_back(' ');
    }

    for (u64 i = 0, size = arg.size(); i < size; ++i) {
      char c = arg[i];
      optional<char> next;
      if (i < size - 1) {
        next = arg[i + 1];
      }

      if (c == ' ') {
        ret.append("\" \"");
      } else if (c == '"') {
        ret.append("\\\"");
      } else if (c == '\\' && next && *next == '"') {
        ret.append("\"\\\\\"");
      } else {
        ret.push_back(c);
      }
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
    if (not ret.empty()) {
      ret.push_back(' ');
    }

    for (char c : arg) {
      if (c == ' ') {
        ret.append("\\ ");
      } else if (c == '"') {
        ret.append("\\\"");
      } else if (c == '\'') {
        ret.append("\\'");
      } else if (c == '\\') {
        ret.append("\\\\");
      } else {
        ret.push_back(c);
      }
    }
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
  ROCKET_MUTEX_LOCK(envMutex);

  string nameStr(name);

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
  const char* p = getenv(nameStr.c_str());
  if (not p) {
    return nullopt;
  }
  return p;
#endif
}

void
setImpl(std::string_view name, const optional<string>& value, bool replace) {
  ROCKET_CHECK(value, value || replace);

  ROCKET_MUTEX_LOCK(envMutex);

  string nameStr(name);

#ifdef ROCKET_OS_WINDOWS
  if (value && not replace && env::get<string>(name)) {
    return;
  }
  // Set/unset
  _putenv_s(nameStr.c_str(), value ? value->c_str() : "");
#else
  if (value) {
    // Set
    setenv(nameStr.c_str(), value->c_str(), replace ? 1 : 0);
  } else {
    // Unset
    unsetenv(const_cast<char*>(nameStr.c_str()));
  }
#endif
}

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

vector<std::byte> // MSVC needs `std::byte`
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

vector<std::byte> // MSVC needs `std::byte`
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
  ROCKET_MUTEX_LOCK(envMutex);

  unordered_map<string, string> ret;

#if 0
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

#ifdef ROCKET_OS_WINDOWS
  char** p = _environ;
#else
  char** p = __environ;
#endif
  while(*p != nullptr) {
    string_view entry(*p);
    cout << "entry=[" << entry << "]" << endl; // XXX
    if (entry.empty()) {
      break;
    }
    auto eq = entry.find('=');
    string_view name, value;
    if (eq == string_view::npos) {
      name = entry;
    } else {
      name = entry.substr(0, eq);
      value = entry.substr(eq + 1);
    }
    cout << "name=[" << name << "], value=[" << value << "]" << endl; // XXX
    ret.emplace(name, value);
    ++p;
  }

  return ret;
}

} // namespace env

} // namespace rocket::system

// EOF
