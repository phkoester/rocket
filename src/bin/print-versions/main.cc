// main.cc

#include "rocket/Process.h"
#include "rocket/icu.h"
#include "rocket/version.h"

#ifdef ROCKET_OS_WINDOWS
  #include <rocket/windows/windows.h>
#endif

#include <boost/version.hpp>

#include <fmt/format.h>

#ifdef ROCKET_ICU_CMAKE
  #include <icu-cmake/version.h>
#endif

#ifdef ROCKET_GUI
  #include <wx/version.h>
#endif

#include <archive.h>

#include <cbor.h>

#include <algorithm>
#include <array>
#include <ranges>
#include <string>
#include <utility> // `std::pair`
#include <vector>

using namespace rocket;
using namespace std;

namespace {

// Local types ----------------------------------------------------------------------------------------------

using Row = pair<string, string>;

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

/// Returns the ICU version shipped with the Windows SDK.
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
printTable(const vector<Row>& rows) {
  const u64 col0w = rows.empty() ?
    0 :
    ranges::max(rows | views::transform([](const Row& row) { return row.first.size(); }));
  for (const auto& [col0, col1] : rows) {
    nio::out.println("{: <{}} | {}", col0, col0w, col1);
  }
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process().init(argc, argv, "print-versions");

  vector<Row> rows;

  // Rocket

  rows.emplace_back("Rocket", rocket::VERSION_NAME);

  // Boost

  rows.emplace_back("Boost", fmt::format("{}.{}.{}",
    BOOST_VERSION / 100'000, BOOST_VERSION / 100 % 1'000, BOOST_VERSION % 100));

  // ICU

#ifdef ROCKET_OS_WINDOWS
  // Windows `<icu.h>` does not provide `U_ICU_VERSION`
  rows.emplace_back("ICU", icuVersion());
  #ifdef ROCKET_ICU_CMAKE
  rows.emplace_back("ICU (Windows SDK)", icuVersionWindowsSdk());
  #endif
#else
  rows.emplace_back("ICU", U_ICU_VERSION);
#endif

  // icu-cmake

#ifdef ROCKET_ICU_CMAKE
  rows.emplace_back("icu-cmake", icu_cmake::VERSION_NAME);
#endif

  // {fmt}

  rows.emplace_back("{fmt}", fmt::format("{}.{}.{}",
    FMT_VERSION / 10'000, FMT_VERSION / 100 % 100, FMT_VERSION % 100));

  // libarchive

  rows.emplace_back("libarchive", ARCHIVE_VERSION_ONLY_STRING);

  // libcbor

  rows.emplace_back("libcbor", fmt::format("{}.{}.{}",
    CBOR_MAJOR_VERSION, CBOR_MINOR_VERSION, CBOR_PATCH_VERSION));

  // scnlib

  rows.emplace_back("scnlib", fmt::format("{}.{}.{}",
    SCN_VERSION / 10'000'000, SCN_VERSION / 10'000 % 1'000, SCN_VERSION % 10'000));

  // wxWidgets

#ifdef ROCKET_GUI
  rows.emplace_back("wxWidgets", fmt::format("{}.{}.{}",
    wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER));
#endif

  printTable(rows);

  process().exit(EXIT_SUCCESS);
}

// EOF
