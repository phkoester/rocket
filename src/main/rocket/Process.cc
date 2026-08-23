/*
 * Process.cc
 */

#include "Process.h"

#include "rocket/Guard.h"
#include "rocket/assert.h"
#include "rocket/log/log.h"
#include "rocket/str/str.h"
#include "rocket/system/system.h"

#include <cstdlib>
#include <ranges>

#ifdef ROCKET_OS_WINDOWS
#include <process.h>
#endif

using namespace rocket;
using namespace std;

namespace {

// Local constants ------------------------------------------------------------------------------------------

const bool ROCKET_EXIT = system::env::get<bool>("ROCKET_EXIT").value_or(false);
const bool ROCKET_QUICK_EXIT = system::env::get<bool>("ROCKET_QUICK_EXIT").value_or(false);

// Local variables ------------------------------------------------------------------------------------------

recursive_mutex processMutex;

vector<pair<function<void()>, bool>> onExitFns;

// Local functions ------------------------------------------------------------------------------------------

string shortName(string_view str);

void
callExitFns(bool onTerminate) {
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_GUARD([] { onExitFns.clear(); });

  for (const auto& [fn, callOnTerminate] : ranges::reverse_view(onExitFns)) {
    if (not onTerminate || callOnTerminate) {
      try {
        fn();
      } catch (...) {
        nio::err.write("While running at-exit function: ");
        printException(nio::err, current_exception());
      }
    }
  }
}

#ifdef ROCKET_OS_WINDOWS

const string&
invocationName() {
  static string ret(__argv[0]);
  return ret;
}

const string&
invocationShortName() {
  static string ret(shortName(__argv[0]));
  return ret;
}

#else

const string&
invocationName() {
  static const string ret(::program_invocation_name);
  return ret;
}

const string&
invocationShortName() {
  static const string ret(::program_invocation_short_name);
  return ret;
}

#endif // ROCKET_OS_WINDOWS

void
onExit() {
  callExitFns(false);
}

[[noreturn]] void
onTerminate() {
  callExitFns(true);

  try {
    nio::err.println("{}: fatal error: Terminate handler called", process.name());
    if (auto ptr = current_exception()) {
      printException(nio::err, ptr);
    }
    nio::err.writeln("Aborting");
  } catch (...) {
    ROCKET_PROCESS_ERROR(0, "`onTerminate` failed");
  }

  abort();
}

string
shortName(string_view str) {
  const string fileName = filesystem::path(str).filename().string();
  const auto sv = str::removeTrailing<char>(fileName, system::executableSuffix());
  return string(sv);
}

} // namespace

namespace rocket {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

thread_local string threadName;

ROCKET_INIT([&] { threadName = "main"; });

/// @ThreadSafe
const string&
setThreadName(std::string_view name) {
  if (not name.empty()) {
    threadName = name;
  }
  return threadName;
}

} // namespace internal

// Constants ------------------------------------------------------------------------------------------------

const thread::id MAIN_THREAD_ID = this_thread::get_id();

// `Process` ------------------------------------------------------------------------------------------------

// Some trickery to keep the ctor private
inline Process makeProcess__() { return {}; }
ROCKET_PUBLIC const Process process = makeProcess__();

void
Process::atExit(std::function<void()>&& fn, bool callOnTerminate) {
  ROCKET_MUTEX_LOCK(processMutex);

  onExitFns.emplace_back(std::move(fn), callOnTerminate);
}

const string&
Process::autoName() const { // NOLINT(*-recursion)
  ROCKET_MUTEX_LOCK(processMutex);

  return inited_ ? name() : invocationShortName();
}

void
Process::exit(i32 status, bool allowUninited) const { // NOLINT(*-recursion)
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(allowUninited || inited_, "Process not initialized");

  if (ROCKET_EXIT) {
    std::exit(status); // NOLINT(concurrency-*)
  }
  if (ROCKET_QUICK_EXIT) {
    std::quick_exit(status);
  }

  if (quickExit_) {
    std::quick_exit(status);
  }
  else {
    std::exit(status); // NOLINT(concurrency-*)
  }
}

i32
Process::id() {
#ifdef ROCKET_OS_WINDOWS
  return _getpid();
#else
  return getpid();
#endif
}

void
Process::init(
    i32 argc,
    char** argv,
    optional<string_view> name,
    optional<std::locale> locale,
    bool quickExit) const {
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(this_thread::get_id() == MAIN_THREAD_ID, "`Process::init` must be called in the main thread");
  ROCKET_ASSERT(not inited_, "Process already initialized");

  // Set the C locale from the environment

  const string localeName = locale ? locale->name() : "";
  setlocale(LC_ALL, localeName.c_str()); // NOLINT(concurrency-*)

  // Set the C++ locale from the environment

  initLocale_ = locale ? *locale : std::locale(localeName);
  systemLocale_ = std::locale::global(initLocale_);

  // Initialize members

  argc_ = argc;
  argv_ = argv;

  name_ = name ? *name : shortName(argv[0]);

  quickExit_ = quickExit;

  for (i32 i = 1; i < argc; ++i) {
    args_.emplace_back(argv[i]);
  }

  // Register `onExit` both for `std::exit` and `std::quick_exit`

  std::atexit(onExit);
  std::at_quick_exit(onExit);

  inited_ = true;

  // Set the terminate handler. This must be done *after* setting `inited_` to `true`

  set_terminate(onTerminate);

  // Init the logging module

  log::internal::logInit();
}

const string&
Process::invocationName() {
  return ::invocationName();
}

const string&
Process::invocationShortName() {
  return ::invocationShortName();
}

const string&
Process::name() const { // NOLINT(*-recursion)
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(inited_, "Process not initialized");
  return name_;
}

} // namespace rocket

// EOF
