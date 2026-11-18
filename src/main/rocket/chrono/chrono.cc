// chrono.cc

#include "chrono.h"

#include "rocket/icu.h"
#include "rocket/scope.h"

#ifdef ROCKET_OS_WINDOWS
  #include "rocket/windows.h"
#endif

#include <array>

using namespace std;
using namespace std::chrono;

namespace rocket::chrono {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::recursive_mutex&
clockMutex() {
  static std::recursive_mutex mutex;
  return mutex;
}

milliseconds&
clockOffset() {
  static milliseconds offset = 0ms;
  return offset;
}

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

string
abbreviation([[maybe_unused]] string_view name, [[maybe_unused]] bool dst, string_view fallback) {
#ifdef ROCKET_OS_WINDOWS
  #ifdef ROCKET_ICU_CMAKE

  // Windows, icu-cmake: load the API functions from `icu.dll` for localized names

  using UcalOpen = decltype(&ucal_open);
  using UcalClose = decltype(&ucal_close);
  using UcalGetTimeZoneDisplayName = decltype(&ucal_getTimeZoneDisplayName);

  struct IcuApi {
    UcalOpen open = nullptr;
    UcalClose close = nullptr;
    UcalGetTimeZoneDisplayName getTimeZoneDisplayName = nullptr;
  };

  // Loaded once and never freed on purpose: the module stays loaded for the lifetime of the process

  static const IcuApi api = [] {
    IcuApi ret;
    if (const HMODULE icu = LoadLibraryExW(L"icu.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
      icu != nullptr) {
      ret.open = reinterpret_cast<UcalOpen>( // NOLINT
        GetProcAddress(icu, "ucal_open"));
      ret.close = reinterpret_cast<UcalClose>( // NOLINT
        GetProcAddress(icu, "ucal_close"));
      ret.getTimeZoneDisplayName = reinterpret_cast<UcalGetTimeZoneDisplayName>( // NOLINT
        GetProcAddress(icu, "ucal_getTimeZoneDisplayName"));
    }
    return ret;
  }();

  if (api.open == nullptr || api.close == nullptr || api.getTimeZoneDisplayName == nullptr) {
    return string(fallback);
  }

  const auto call_ucal_open = api.open;
  const auto call_ucal_close = api.close;
  const auto call_ucal_getTimeZoneDisplayName = api.getTimeZoneDisplayName;

  #else

  const auto call_ucal_open = ucal_open;
  const auto call_ucal_close = ucal_close;
  const auto call_ucal_getTimeZoneDisplayName = ucal_getTimeZoneDisplayName;

  #endif // ROCKET_ICU_CMAKE

  // Convert the zone name from UTF-8 to UTF-16

  UErrorCode status = U_ZERO_ERROR;
  array<UChar, 128> name16 {};
  i32 name16Len = 0;
  u_strFromUTF8(name16.data(), i32(name16.size()), &name16Len, name.data(), i32(name.size()), &status);
  if (U_FAILURE(status) || name16Len <= 0) {
    return string(fallback);
  }

  // Open a calendar for the zone. A null locale is the default locale

  UCalendar* const cal = call_ucal_open(name16.data(), name16Len, nullptr, UCAL_GREGORIAN, &status);
  if (U_FAILURE(status)) {
    return string(fallback);
  }
  ROCKET_SCOPE_EXIT([&] { call_ucal_close(cal); });

  array<UChar, 64> buf {};
  const i32 len = call_ucal_getTimeZoneDisplayName(
    cal, dst ? UCAL_SHORT_DST : UCAL_SHORT_STANDARD, nullptr, buf.data(), i32(buf.size()), &status);
  if (U_FAILURE(status) || len <= 0) {
    return string(fallback);
  }

  // Convert the result to UTF-8. Localized names may not be ASCII, for example `GMT−5` with U+2212 (MINUS
  // SIGN). Preflight to get the size

  i32 size = 0;
  u_strToUTF8(nullptr, 0, &size, buf.data(), len, &status);
  if (status != U_BUFFER_OVERFLOW_ERROR || size <= 0) {
    return string(fallback);
  }
  status = U_ZERO_ERROR;
  string ret(static_cast<string::size_type>(size), '\0');
  u_strToUTF8(ret.data(), size, nullptr, buf.data(), len, &status);
  if (U_FAILURE(status)) {
    return string(fallback);
  }
  return ret;
#else
  return string(fallback);
#endif // ROCKET_OS_WINDOWS
}

} // namespace rocket::chrono

// EOF
