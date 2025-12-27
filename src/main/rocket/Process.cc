/*
 * Process.cc
 */

#include "Process.h"

#include "assert.h"
#include "except.h"
#include "locale.h"
#include "log.h"
#include "strings.h"
#include "system.h"

using namespace rocket;
using namespace std;

// Local functions ------------------------------------------------------------------------------------------

namespace {

#ifdef GAIA_TARGET_OS_LINUX

const string&
invocationName() {
  static string ret(program_invocation_name);
  return ret;
}

const string&
invocationShortName() {
  static string ret(program_invocation_short_name);
  return ret;
}

#endif

[[noreturn]] void
onTerminate() {
  try {
    nio::stderr.println("{}: fatal error: Terminate handler called", process.name());
    if (auto ptr = current_exception())
      except::printException(nio::stderr, ptr);
    nio::stderr.writeln("Aborting");
  } catch (...) {
    ROCKET_PROCESS_ERROR("`onTerminate` failed");
  }
  abort();
}

} // namespace

namespace rocket {

// `Process` ------------------------------------------------------------------------------------------------

Process process;

void
Process::atExit(void (*f)()) const { // cppcheck-suppress constParameterPointer
  ROCKET_ASSERT(inited_, "Process not initialized");

  if (quickExit_) {
    if (at_quick_exit(f)) {
      throw except::InvalidState("`at_quick_exit()` failed");
    }
  } else {
    if (atexit(f)) {
      throw except::InvalidState("`atexit()` failed");
    }
  }
}

void
Process::exit(int status) const {
  ROCKET_ASSERT(inited_, "Process not initialized");

  if (quickExit_)
    std::quick_exit(status);
  else
    std::exit(status);
}

void
Process::init(
    int argc,
    char** argv,
    optional<string_view> name,
    optional<std::locale> locale,
    bool quickExit) {
  ROCKET_ASSERT(not inited_, "Process already initialized");

  // Set the C locale from the environment

  string localeName = locale ? locale->name() : "";
  setlocale(LC_ALL, localeName.c_str());

  // Set the C++ locale from the environment, add `char32_t` support to STL streams

  initLocale_ = locale ? *locale : std::locale(localeName);
  systemLocale_ = std::locale::global(initLocale_);

  // Initialize members

  argc_ = argc;
  argv_ = argv;

  if (name)
    name_ = *name;
  else {
    string_view name(argv[0]);
    auto lastFileSep = name.find_last_of(system::fileSeparator());
    if (lastFileSep != string::npos)
      name = name.substr(lastFileSep + 1);
    name = strings::removeTrailing(name, system::executableSuffix());
    name_ = name;
  }

  quickExit_ = quickExit;

  for (int i = 1; i < argc; ++i)
    args_.emplace_back(argv[i]);

  inited_ = true;

  // Set the terminate handler. This must be done AFTER setting `inited_` to `true`

  set_terminate(onTerminate);

  // Init the logging module

  log::internal::init();
}

const string&
Process::invocationName() const {
  return ::invocationName();
}

const string&
Process::invocationShortName() const {
  return ::invocationShortName();
}

const string&
Process::name() const {
  ROCKET_ASSERT(inited_, "Process not initialized");
  return name_;
}

} // namespace rocket

// EOF
