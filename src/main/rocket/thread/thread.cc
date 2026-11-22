// thread.cc

#include "thread.h"

#include "rocket/assert.h"
#include "rocket/SystemError.h"
#include "rocket/unicode/unicode.h"

#include <array>

#if defined(ROCKET_OS_LINUX) || defined(ROCKET_OS_MACOS)
  #include <pthread.h>
#else
  #include "rocket/scope.h"
  #include "rocket/windows/windows.h"
#endif

using namespace std;

namespace rocket::thread {

// Functions ------------------------------------------------------------------------------------------------

#ifdef ROCKET_OS_LINUX

// Linux limits thread names to 16 bytes, including the terminating NUL

string
currentThreadName() {
  array<char, 16> buf {};
  if (const int err = pthread_getname_np(pthread_self(), buf.data(), buf.size()); err != 0) {
    throw SystemError(errorCode(err), "cannot get thread name");
  }
  return buf.data();
}

void
setCurrentThreadName(string_view name) {
  ROCKET_CHECK(name, name.find('\0') == string_view::npos);

  const string str(unicode::utf8::cut(name, 15));
  if (const int err = pthread_setname_np(pthread_self(), str.c_str()); err != 0) {
    throw SystemError(errorCode(err), "cannot set thread name");
  }
}

#elifdef ROCKET_OS_MACOS

// macOS limits thread names to 64 bytes (`MAXTHREADNAMESIZE`), including the terminating NUL. A thread can
// only name itself, so `pthread_setname_np` takes no thread argument

string
currentThreadName() {
  array<char, 64> buf {};
  if (const int err = pthread_getname_np(pthread_self(), buf.data(), buf.size()); err != 0) {
    throw SystemError(errorCode(err), "cannot get thread name");
  }
  return buf.data();
}

void
setCurrentThreadName(string_view name) {
  ROCKET_CHECK(name, name.find('\0') == string_view::npos);

  const string str(unicode::utf8::cut(name, 63));
  if (const int err = pthread_setname_np(str.c_str()); err != 0) {
    throw SystemError(errorCode(err), "cannot set thread name");
  }
}

#else

// Windows stores thread descriptions as UTF-16 (Windows 10 1607 and later)

string
currentThreadName() {
  PWSTR desc = nullptr;
  if (const HRESULT hr = GetThreadDescription(GetCurrentThread(), &desc); FAILED(hr)) {
    throw SystemError(windows::errorCode(hr), "cannot get thread name");
  }
  ROCKET_SCOPE_EXIT([desc] { LocalFree(desc); });
  return windows::toUtf8(desc);
}

void
setCurrentThreadName(string_view name) {
  ROCKET_CHECK(name, name.find('\0') == string_view::npos);

  const wstring str = windows::toUtf16(name);
  if (const HRESULT hr = SetThreadDescription(GetCurrentThread(), str.c_str()); FAILED(hr)) {
    throw SystemError(windows::errorCode(hr), "cannot set thread name");
  }
}

#endif // ROCKET_OS_LINUX

} // namespace rocket::thread

// EOF
