/*
 * main.cc
 */

#include <rocket/Process.h>
#include <rocket/format.h>
#include <rocket/icu.h>
#include <rocket/scan.h>
#include <rocket/version.h>

#ifdef ROCKET_OS_WINDOWS
  #include <rocket/windows.h>
#endif

#include <boost/version.hpp>

#ifdef ROCKET_GUI
  #include <wx/version.h>
#endif

#include <archive.h>

#include <cbor.h>

#include <array>

using namespace rocket;
using namespace std;

namespace {

// Local constants ------------------------------------------------------------------------------------------

constexpr u32 LEFT_COLUMN_WIDTH = 17;

// Local functions ------------------------------------------------------------------------------------------

#ifdef ROCKET_OS_WINDOWS

string
icuVersion() {
  UVersionInfo info {};
  u_getVersion(info);
  array<char, U_MAX_VERSION_STRING_LENGTH> str {};
  u_versionToString(info, str.data());
  return str.data();
}

#ifdef ROCKET_ICU_CMAKE

/**
 * Returns the ICU version shipped with the Windows SDK.
 */
string
icuVersionWindowsSdk() {
  using UGetVersion = decltype(&u_getVersion);

  // Loaded once and never freed on purpose: the module stays loaded for the lifetime of the process

  static const UGetVersion call_u_getVersion = []() -> UGetVersion {
    if (const HMODULE icu = LoadLibraryExW(L"icu.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
      icu != nullptr) {
      return reinterpret_cast<UGetVersion>(GetProcAddress(icu, "u_getVersion")); // NOLINT
    }
    return nullptr;
  }();

  if (call_u_getVersion == nullptr) {
    return "N/A";
  }

  UVersionInfo info {};
  call_u_getVersion(info);
  array<char, U_MAX_VERSION_STRING_LENGTH> str {};
  u_versionToString(info, str.data());
  return str.data();
}

#endif // ROCKET_ICU_CMAKE

#endif // ROCKET_OS_WINDOWS

void
printVersion(string_view name, string_view version) {
  nio::out.println("{: <{}} | {}", name, LEFT_COLUMN_WIDTH, version);
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process().init(argc, argv, "print-versions");

  // Rocket

  printVersion("Rocket", ROCKET_VERSION_NAME);

  // Boost

  printVersion("Boost", fmt::format("{}.{}.{}",
    BOOST_VERSION / 100'000, BOOST_VERSION / 100 % 1'000, BOOST_VERSION % 100));

  // ICU

#if defined(ROCKET_OS_WINDOWS)
  // Windows `<icu.h>` does not provide `U_ICU_VERSION`
  printVersion("ICU", icuVersion());
  #ifdef ROCKET_ICU_CMAKE
  printVersion("ICU (Windows SDK)", icuVersionWindowsSdk());
  #endif
#else
  printVersion("ICU", U_ICU_VERSION);
#endif

  // {fmt}

  printVersion("{fmt}", fmt::format("{}.{}.{}",
    FMT_VERSION / 10'000, FMT_VERSION / 100 % 100, FMT_VERSION % 100));

  // libarchive

  printVersion("libarchive", ARCHIVE_VERSION_ONLY_STRING);

  // libcbor

  printVersion("libcbor", fmt::format("{}.{}.{}",
    CBOR_MAJOR_VERSION, CBOR_MINOR_VERSION, CBOR_PATCH_VERSION));

  // scnlib

  printVersion("scnlib", fmt::format("{}.{}.{}",
    SCN_VERSION / 10'000'000, SCN_VERSION / 10'000 % 1'000, SCN_VERSION % 10'000));

  // wxWidgets

#ifdef ROCKET_GUI
  printVersion("wxWidgets", fmt::format("{}.{}.{}", wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER));
#endif

  process().exit(EXIT_SUCCESS);
}

// EOF
