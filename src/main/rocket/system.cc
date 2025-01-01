/*
 * system.cc
 */

#include "codec-std-decl.h"
#include "codec-std.h"

#include "system.h"

#include "S.h"
#include "except.h"

#include <array>
#include <memory>

using namespace rocket;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

#ifdef GAIA_TARGET_OS_FAMILY_WIN

/**
 * Windows: Convert arguments to a command-line string. The rules are:
 *
 * - Replace a space by @c " "
 * - Replace a quotation mark by @c \"
 * - Replace a backslash followed by quotation markby @c "\\"
 *
 * @todo Use a generalized escaping mechanism
 */
string
makeCl(const vector<string_view>& args) {
  string result;

  for (const auto& arg : args) {
    if (not result.empty())
      result.push_back(' ');

    for (size_t i = 0, size = arg.size(); i < size; ++i) {
      char c = arg[i];
      optional<char> next;
      if (i < size - 1)
        next = arg[i + 1];
      
      if (c == ' ')
        result.append("\" \"");
      else if (c == '"')
        result.append("\\\"");
      else if (c == '\\' && next && *next == '"')
        result.append("\"\\\\\"");
      else
        result.push_back(c);
    }
  }

  return result;
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
  string result;

  for (const auto& arg : args) {
    if (not result.empty())
      result.push_back(' ');

    for (char c : arg) {
      if (c == ' ')
        result.append("\\ ");
      else if (c == '"')
        result.append("\\\"");
      else if (c == '\'')
        result.append("\\'");
      else if (c == '\\')
        result.append("\\\\");
      else
        result.push_back(c);
    }
  }

  return result;
}

#endif

} // namespace

// Functions ------------------------------------------------------------------------------------------------

namespace rocket::system {

vector<byte>
exec(const string& cl) {
  vector<byte> result;
  array<byte, 128> buf;
  
  unique_ptr<FILE, decltype(&pclose)> pipe(popen(cl.c_str(), "r"), pclose);
  if (not pipe)
    throw except::InvalidState(S << "Cannot open pipe for command " << cl);
  size_t n;
  while ((n = fread(buf.data(), 1, buf.size(), pipe.get())) > 0) {
    result.insert(result.end(), buf.begin(), buf.begin() + n);
  }
  if (ferror(pipe.get()) != 0)
    throw except::InvalidState(S << "Cannot read from pipe for command " << cl);
  
  return result;
}

vector<byte>
exec(const vector<string_view>& args) {
  return exec(makeCl(args));
}

string_view
executableSuffix() {
#ifdef GAIA_TARGET_OS_FAMILY_WIN
  return ".exe";
#else
  return "";
#endif
}

char
fileSeparator() {
#ifdef GAIA_TARGET_OS_FAMILY_WIN
  return '\\';
#else
  return '/';
#endif
}

char
pathSeparator() {
#ifdef GAIA_TARGET_OS_FAMILY_WIN
  return ';';
#else
  return ':';
#endif
}

namespace env {

// Environment ----------------------------------------------------------------------------------------------

void
unset(const string& name) {
  putenv(const_cast<char*>(name.c_str()));
}

} // namespace env

} // namespace rocket::system

// EOF
