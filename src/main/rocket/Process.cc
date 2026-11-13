/*
 * Process.cc
 */

#include "Process.h"

#include "rocket/assert.h"
#include "rocket/scope.h"
#include "rocket/log/log.h"
#include "rocket/nio/nio.h"
#include "rocket/system/system.h"

#include <cstdlib>

#ifdef ROCKET_OS_MACOS
  #include <crt_externs.h>
#endif
#ifdef ROCKET_OS_WINDOWS
  #include <process.h>
#endif

using namespace rocket;
using namespace std;

namespace {

// Local constants ------------------------------------------------------------------------------------------

const bool ROCKET_EXIT = system::env::get<bool>("ROCKET_EXIT").value_or(false);
const bool ROCKET_QUICK_EXIT = system::env::get<bool>("ROCKET_QUICK_EXIT").value_or(false);

/// The name used if `argv[0]` is missing or empty, which is possible: `execve` may pass an empty `argv`.
constexpr string_view FALLBACK_NAME = "program";

// Local variables ------------------------------------------------------------------------------------------

recursive_mutex processMutex;

vector<pair<function<void()>, bool>> onExitFns;

// Local functions ------------------------------------------------------------------------------------------

/// Returns @p argv0, or #FALLBACK_NAME if @p argv0 is null or empty.
string
argv0OrFallback(const char* argv0) {
  return argv0 != nullptr && *argv0 != '\0' ? string(argv0) : string(FALLBACK_NAME);
}

void
callOnExitFns(bool onTerminate) {
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_SCOPE_FAIL([] { onExitFns.clear(); });

  // Take each function off the vector before calling it. A function may call `Process::atExit`, which may
  // reallocate the vector. Newly registered functions are called next
  while (not onExitFns.empty()) {
    auto [fn, callOnTerminate] = std::move(onExitFns.back());
    onExitFns.pop_back();

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

// Flushes `nio::out` and `nio::err`. Neither `std::quick_exit` nor `abort` flushes C streams
void
flushStandardSinks() {
  nio::out.flush();
  nio::err.flush();
}

const filesystem::path&
invocationPath() {
#if defined(ROCKET_OS_LINUX)
  static const filesystem::path path = argv0OrFallback(::program_invocation_name);
#elif defined(ROCKET_OS_MACOS)
  static const filesystem::path path = []() {
    char** const argv = *_NSGetArgv();
    return argv0OrFallback(argv != nullptr ? argv[0] : nullptr);
  }();
#else
  static const filesystem::path path = argv0OrFallback(__argv != nullptr ? __argv[0] : nullptr);
#endif
  return path;
}

const filesystem::path&
invocationStem() {
  static const filesystem::path stem = invocationPath().stem();
  return stem;
}

void
onExit() {
  callOnExitFns(false);
  flushStandardSinks();
}

[[noreturn]] void
onTerminate() {
  callOnExitFns(true);
  flushStandardSinks(); // Before the error message, to keep the order of the output

  try {
    nio::err.println("{}: fatal error: Terminate handler called", process().name());
    if (auto ptr = current_exception()) {
      printException(nio::err, ptr);
    }
    nio::err.writeln("Aborting");
  } catch (...) {
    ROCKET_PROCESS_ERROR(0, "`onTerminate` failed");
  }

  flushStandardSinks();
  abort();
}

} // namespace

namespace rocket {

// Internal -------------------------------------------------------------------------------------------------

namespace internal::process {

thread_local string threadName;

ROCKET_INIT([&] { threadName = "main"; });

locale
localeFromEnvironment(nio::Sink& err) {
  if (auto ret = tryLocale("")) {
    return *ret;
  }
  rocket::process().warn(err, "Cannot use the locale from the environment; using the classic locale instead");
  return locale::classic();
}

/// @ThreadSafe
const string&
setThreadName(string_view name) {
  if (not name.empty()) {
    threadName = name;
  }
  return threadName;
}

optional<locale>
tryLocale(const string& name) {
  try {
    return locale(name);
  } catch (const runtime_error&) {
    return {};
  }
}

} // namespace internal::process

// Constants ------------------------------------------------------------------------------------------------

const thread::id MAIN_THREAD_ID = this_thread::get_id();

// `Process` ------------------------------------------------------------------------------------------------

const vector<string>&
Process::args() const {
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(inited_, "Process not initialized");
  return args_;
}

void
Process::atExit(function<void()>&& fn, bool callOnTerminate) const {
  ROCKET_MUTEX_LOCK(processMutex);

  onExitFns.emplace_back(std::move(fn), callOnTerminate);
}

string
Process::autoName() const {
  ROCKET_MUTEX_LOCK(processMutex);

  return inited_ ? name() : invocationStem().string();
}

void
Process::exit(i32 status) const {
  ROCKET_MUTEX_LOCK(processMutex);

  if (ROCKET_EXIT) {
    std::exit(status);
  }
  if (ROCKET_QUICK_EXIT) {
    std::quick_exit(status);
  }

  if (quickExit_) {
    std::quick_exit(status);
  } else {
    std::exit(status);
  }
}

u32
Process::id() const {
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
  string_view name,
  optional<locale> locale,
  bool quickExit) const {
  ROCKET_CHECK(name, not name.empty());

  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(this_thread::get_id() == MAIN_THREAD_ID, "`Process::init` must be called in the main thread");
  ROCKET_ASSERT(not inited_, "Process already initialized");

  // Set given locale, if any, otherwise the locale from the environment

  initLocale_ = locale ? *locale : internal::process::localeFromEnvironment(nio::err);
  std::locale::global(initLocale_);

  // Initialize more members

  argc_ = argc;
  argv_ = argv;
  name_ = name;
  quickExit_ = quickExit;
  for (i32 i = 1; i < argc; ++i) {
    args_.emplace_back(argv[i]);
  }

  // Register `onExit` both for `std::exit` and `std::quick_exit`

  std::atexit(onExit);
  std::at_quick_exit(onExit);

  inited_ = true;

  // Set the terminate handler

  set_terminate(onTerminate);

  // Init the logging module

  log::internal::logInit();
}

const locale&
Process::initLocale() const {
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(inited_, "Process not initialized");
  return initLocale_;
}

const filesystem::path&
Process::invocationPath() const {
  ROCKET_MUTEX_LOCK(processMutex);

  if (invocationPath_.empty()) {
    invocationPath_ = ::invocationPath();
  }
  return invocationPath_;
}

const filesystem::path&
Process::invocationStem() const {
  ROCKET_MUTEX_LOCK(processMutex);

  if (invocationStem_.empty()) {
    invocationStem_ = ::invocationStem();
  }
  return invocationStem_;
}

const string&
Process::name() const {
  ROCKET_MUTEX_LOCK(processMutex);

  ROCKET_ASSERT(inited_, "Process not initialized");
  return name_;
}

const Process&
process() {
  static const Process proc;
  return proc;
}

} // namespace rocket

// EOF
