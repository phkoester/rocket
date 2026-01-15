/*
 * Process.cc
 */

#include "Process.h"

#include "rocket/Guard.h"
#include "rocket/assert.h"
#include "rocket/log/log.h"
#include "rocket/str/str.h"
#include "rocket/system/system.h"

#include <thread>

using namespace rocket;
using namespace std;

namespace {

// Local constants ------------------------------------------------------------------------------------------

const bool ROCKET_EXIT = system::env::get<bool>("ROCKET_EXIT").value_or(false);
const bool ROCKET_QUICK_EXIT = system::env::get<bool>("ROCKET_QUICK_EXIT").value_or(false);

const thread::id MAIN_THREAD_ID = thread::id();

// Local variables ------------------------------------------------------------------------------------------

recursive_mutex processMutex;

vector<pair<function<void()>, bool>> onExitFns;

// Local functions ------------------------------------------------------------------------------------------

void
callExitFns(bool onTerminate) {
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_GUARD([] { onExitFns.clear(); });

  for (auto it = onExitFns.rbegin(); it != onExitFns.rend(); ++it) {
    const auto& [fn, callOnTerminate] = *it;
    if (not onTerminate || callOnTerminate) {
      try {
        fn();
      } catch (...) {
        nio::stderr.write("While running at-exit function: ");
        printException(nio::stderr, current_exception());
      }
    }
  }
}

#ifdef GAIA_TARGET_OS_LINUX

const string&
invocationName() {
  static string ret(::program_invocation_name);
  return ret;
}

const string&
invocationShortName() {
  static string ret(::program_invocation_short_name);
  return ret;
}

#endif

void
onExit() {
  callExitFns(false);
}

[[noreturn]] void
onTerminate() {
  callExitFns(true);

  try {
    nio::stderr.println("{}: fatal error: Terminate handler called", process.name());
    if (auto ptr = current_exception()) {
      printException(nio::stderr, ptr);
    }
    nio::stderr.writeln("Aborting");
  } catch (...) {
    ROCKET_PROCESS_ERROR("`onTerminate` failed");
  }

  abort();
}

} // namespace

namespace rocket {

// `Process` ------------------------------------------------------------------------------------------------

// Some trickery to keep the ctor private
inline Process makeProcess__() { return Process(); }
Process process = makeProcess__();

void
Process::atExit(std::function<void()> fn, bool callOnTerminate) {
  ROCKET_MUTEX_LOCK(processMutex);

  onExitFns.push_back({ fn, callOnTerminate });
}

string
Process::autoName() {
  ROCKET_MUTEX_LOCK(processMutex);

  return inited_ ? name() : invocationShortName();
}

void
Process::exit(int status) const {
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(inited_, "Process not initialized");

  if (ROCKET_EXIT) {
    std::exit(status);
  }
  if (ROCKET_QUICK_EXIT) {
    std::quick_exit(status);
  }

  if (quickExit_) {
    std::quick_exit(status);
  }
  else {
    std::exit(status);
  }

  // XXX ROCKET_TERMINATE_UNREACHABLE_CODE();
}

void
Process::init(
    int argc,
    char** argv,
    optional<string_view> name,
    optional<std::locale> locale,
    bool quickExit) {
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(thread::id() == MAIN_THREAD_ID, "Process::init must be called in the main thread");
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
    name = str::removeTrailing(name, system::executableSuffix());
    name_ = name;
  }

  quickExit_ = quickExit;

  for (int i = 1; i < argc; ++i)
    args_.emplace_back(argv[i]);

  // Register `onExit` both for `std::exit` and `std::quick_exit`

  std::atexit(onExit);
  std::at_quick_exit(onExit);

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
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(inited_, "Process not initialized");
  return name_;
}

} // namespace rocket

// EOF
