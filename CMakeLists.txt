#
# CMakeLists.txt
#

cmake_minimum_required(VERSION 4.2)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/rocket-functions.cmake)

include(CheckCXXSourceCompiles)
include(FetchContent)

# Project ---------------------------------------------------------------------------------------------------

# Is this the master project?
if(NOT DEFINED ROCKET_MASTER_PROJECT)
  set(ROCKET_MASTER_PROJECT OFF)
  if(NOT DEFINED PROJECT_NAME)
    set(ROCKET_MASTER_PROJECT ON)
  endif()
endif()

project(rocket VERSION 0.3.6)

# Versions --------------------------------------------------------------------------------------------------

set(ROCKET_BENCHMARK_VERSION 1.9.5)
set(ROCKET_BOOST_VERSION 1.92.0)
set(ROCKET_BOOST_INT128_VERSION 4.1.0)
set(ROCKET_FMT_VERSION 12.2.0)
set(ROCKET_GAIA_VERSION 1.2.5)
set(ROCKET_GTEST_VERSION 1.18.0)
set(ROCKET_ICU_VERSION 72.1)
set(ROCKET_ICU_CMAKE_VERSION 0.1.4)
set(ROCKET_ICU_CMAKE_ICU_VERSION 79.1)
set(ROCKET_LIBARCHIVE_VERSION 3.8.11)
set(ROCKET_LIBCBOR_VERSION 0.14.0)
set(ROCKET_SCNLIB_VERSION 4b8a7400ce34208fa47b9abec492a6497a7ab48c) # `master` as of 2026-11-14
set(ROCKET_WXWIDGETS_VERSION 3.3.4)

# Gaia ------------------------------------------------------------------------------------------------------

if(DEFINED ENV{GAIA_DIR})
  FetchContent_Declare(
    gaia
    SOURCE_DIR "$ENV{GAIA_DIR}"
  )
else()
  FetchContent_Declare(
    gaia
    GIT_REPOSITORY https://github.com/phkoester/gaia.git
    GIT_TAG        v${ROCKET_GAIA_VERSION}
    GIT_PROGRESS   TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
endif()
FetchContent_MakeAvailable(gaia)

# CMPs ------------------------------------------------------------------------------------------------------

# Get rid of a warning from libcbor
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# General settings ------------------------------------------------------------------------------------------

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON) # ccache will benefit from this on Windows
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded) # This makes ccache work on Windows
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

set(FETCHCONTENT_QUIET FALSE)

# Feature detection -----------------------------------------------------------------------------------------

# 128-bit integers
# Windows: the MSVC toolchain has no support, and even though the LLVM toolchain does have a builtin
# `__int128` type, it will not work with {fmt}. So we don't use `__int128` at any rate
if(NOT GAIA_OS_WINDOWS)
  check_cxx_source_compiles([[
    int main() {
      __int128 i = 1;
      unsigned __int128 u = 2;
      return static_cast<int>(i + static_cast<__int128>(u)) - 3;
    }
  ]] ROCKET_HAS_INT128)
endif()
gaiaStatus("ROCKET_HAS_INT128: ${ROCKET_HAS_INT128}")

# C++23 span streams
check_cxx_source_compiles([[
  #include <span>
  #include <spanstream>

  int main() {
    char buf[8] = "42";
    std::ispanstream in(std::span<char>(buf, 2));
    int i = 0;
    in >> i;
    return i == 42 ? 0 : 1;
  }
]] ROCKET_HAS_STD_SPANSTREAM)
gaiaStatus("ROCKET_HAS_STD_SPANSTREAM: ${ROCKET_HAS_STD_SPANSTREAM}")

# C++23 stack traces. On Linux, libstdc++ keeps `std::stacktrace` in `libstdc++exp`, which `rocket` links,
# too
block()
  if(GAIA_OS_LINUX)
    set(CMAKE_REQUIRED_LIBRARIES stdc++exp)
  endif()
  check_cxx_source_compiles([[
    #include <stacktrace>
    #include <string>

    int main() {
      return std::to_string(std::stacktrace::current()).empty() ? 1 : 0;
    }
  ]] ROCKET_HAS_STD_STACKTRACE)
endblock()
gaiaStatus("ROCKET_HAS_STD_STACKTRACE: ${ROCKET_HAS_STD_STACKTRACE}")

# C++20 time zones: `std::chrono::time_zone` and `std::chrono::zoned_time`. This links, too, so the time-zone
# database must be present in the standard library
check_cxx_source_compiles([[
  #include <chrono>

  int main() {
    const std::chrono::time_zone* tz = std::chrono::locate_zone("UTC");
    std::chrono::zoned_time zt(tz, std::chrono::sys_seconds());
    return zt.get_time_zone()->name().empty() ? 1 : 0;
  }
]] ROCKET_HAS_STD_CHRONO_TIME_ZONE)
gaiaStatus("ROCKET_HAS_STD_CHRONO_TIME_ZONE: ${ROCKET_HAS_STD_CHRONO_TIME_ZONE}")

# Experimental scope guards (Library Fundamentals TS v3)
check_cxx_source_compiles([[
  #include <experimental/scope>

  int main() {
    int i = 0;
    {
      std::experimental::scope_exit exit([&] { ++i; });
      std::experimental::scope_fail fail([&] { i += 10; });
      std::experimental::scope_success success([&] { ++i; });
    }
    return i == 2 ? 0 : 1;
  }
]] ROCKET_HAS_STD_EXPERIMENTAL_SCOPE)
gaiaStatus("ROCKET_HAS_STD_EXPERIMENTAL_SCOPE: ${ROCKET_HAS_STD_EXPERIMENTAL_SCOPE}")

# Configuration ---------------------------------------------------------------------------------------------

set(defaultComponents)
if(ROCKET_MASTER_PROJECT)
  set(defaultComponents gui)
endif()

gaiaAddVar(ROCKET_BUILD_BENCH BOOL   "${ROCKET_MASTER_PROJECT}" "Build benchmarks")
gaiaAddVar(ROCKET_BUILD_TEST  BOOL   "${ROCKET_MASTER_PROJECT}" "Build tests")
gaiaAddVar(ROCKET_COMPONENTS  STRING "${defaultComponents}"     "Additional components to enable: `gui`")
gaiaAddVar(ROCKET_ICU_CMAKE   BOOL   OFF                        "Use icu-cmake to build ICU")

# Set compiler and linker options ---------------------------------------------------------------------------

set(COMPILE_DEFS)
set(COMPILE_FEATURES)
set(COMPILE_FLAGS)
set(LINK_FLAGS)

if(GAIA_OS_LINUX OR GAIA_OS_MACOS)
  # Linux/macOS
  list(APPEND COMPILE_FLAGS -Wall -Werror -Wextra)
elseif(GAIA_OS_WINDOWS)
  # Windows
  # Windows 10, needed by `<icu.h>` from the Windows SDK, which only declares its API for Windows 10
  list(APPEND COMPILE_DEFS _WIN32_WINNT=0x0A00)
  if(GAIA_CXX_COMPILER_LLVM)
    # Windows, LLVM
    list(APPEND COMPILE_FLAGS -Werror -Wextra # No `-Wall` on Windows
      -Wno-unknown-attributes)
  elseif(GAIA_CXX_COMPILER_MSVC)
    # Windows, MSVC
    # `/utf-8`: The sources are UTF-8 without a BOM. MSVC would read them in the ANSI code page
    list(APPEND COMPILE_FLAGS /bigobj /utf-8 /wd4244 /Zc:preprocessor)
  endif()
endif()

# Configure components --------------------------------------------------------------------------------------

list(FIND ROCKET_COMPONENTS gui index__)
if(index__ GREATER_EQUAL 0)
  set(ROCKET_GUI ON)
else()
  set(ROCKET_GUI OFF)
endif()
gaiaStatus("ROCKET_GUI: ${ROCKET_GUI}")

# Dependencies ----------------------------------------------------------------------------------------------

# ICU root (macOS); must come before Boost ..................................................................

# Homebrew’s `icu4c` is keg-only, so point `FindICU` at it. Otherwise it picks up the partial ICU headers
# from the macOS SDK, which come without a linkable `icuuc`/`icui18n`. This must come before Boost, whose
# Boost.Regex and Boost.Locale run `find_package(ICU)` themselves and would cache the SDK’s `ICU_INCLUDE_DIR`

if(GAIA_OS_MACOS AND NOT ROCKET_ICU_CMAKE AND NOT ICU_ROOT AND NOT DEFINED ENV{ICU_ROOT})
  execute_process(
    COMMAND brew --prefix icu4c
    OUTPUT_VARIABLE ROCKET_BREW_ICU_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE ROCKET_BREW_RESULT
  )
  if(ROCKET_BREW_RESULT EQUAL 0 AND IS_DIRECTORY "${ROCKET_BREW_ICU_PREFIX}")
    set(ICU_ROOT "${ROCKET_BREW_ICU_PREFIX}")
  endif()
endif()

# Boost .....................................................................................................

set(ROCKET_BOOST_COMPONENTS
  algorithm
  asio
  bimap
  filesystem
  headers
  preprocessor
)

if(NOT ROCKET_HAS_STD_STACKTRACE)
  list(APPEND ROCKET_BOOST_COMPONENTS stacktrace)
endif()

find_package(Boost ${ROCKET_BOOST_VERSION} QUIET COMPONENTS ${ROCKET_BOOST_COMPONENTS})
if(NOT Boost_FOUND)
  gaiaStatus("Boost ${ROCKET_BOOST_VERSION} not found; fetching it")

  # This works with Boost 1.87.0 or higher
  FetchContent_Declare(
    Boost
    URL          https://github.com/boostorg/boost/releases/download/boost-${ROCKET_BOOST_VERSION}/boost-${ROCKET_BOOST_VERSION}-cmake.7z
    SYSTEM
    EXCLUDE_FROM_ALL
  )

  block()
    set(BUILD_SHARED_LIBS OFF)
    set(BOOST_ENABLE_CMAKE ON)
    set(BOOST_INCLUDE_LIBRARIES ${ROCKET_BOOST_COMPONENTS})
    FetchContent_MakeAvailable(Boost)
  endblock()
endif()

# {fmt} .....................................................................................................

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        ${ROCKET_FMT_VERSION}
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

block()
  set(BUILD_SHARED_LIBS OFF)
  set(CACHE{FMT_MODULE} FORCE VALUE OFF)
  FetchContent_MakeAvailable(fmt)
endblock()

# GoogleTest ................................................................................................

FetchContent_Declare(
  GTest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v${ROCKET_GTEST_VERSION}
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

block()
  set(BUILD_SHARED_LIBS OFF)
  if(GAIA_OS_WINDOWS)
    # For Windows: Prevent overriding the parent project’s compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  endif()
  FetchContent_MakeAvailable(GTest)
endblock()

# Benchmark; must come after GoogleTest .....................................................................

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v${ROCKET_BENCHMARK_VERSION}
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

block()
  set(BUILD_SHARED_LIBS OFF)
  set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF)
  # Benchmark registers its own tests with CTest, which are excluded from the build and thus fail as "Not Run"
  set(BENCHMARK_ENABLE_TESTING OFF)
  FetchContent_MakeAvailable(benchmark)
endblock()

# ICU .......................................................................................................

if(ROCKET_ICU_CMAKE)
  if(DEFINED ENV{GAIA_DIR})
    FetchContent_Declare(
      icu-cmake
      SOURCE_DIR "$ENV{GAIA_DIR}/../icu-cmake"
    )
  else()
    FetchContent_Declare(
      icu-cmake
      GIT_REPOSITORY https://github.com/phkoester/icu-cmake.git
      GIT_TAG        v${ROCKET_ICU_CMAKE_VERSION}
      GIT_PROGRESS   TRUE
      SYSTEM
      EXCLUDE_FROM_ALL
    )
  endif()

  block()
    set(ICU_CMAKE_ICU_VERSION ${ROCKET_ICU_CMAKE_ICU_VERSION})
    FetchContent_MakeAvailable(icu-cmake)
  endblock()
else()
  # On Windows, with `ROCKET_ICU_CMAKE` disabled, Rocket uses the ICU shipped with Windows through `<icu.h>`
  # from the Windows SDK, which `FindICU` cannot find (it looks for `unicode/utypes.h`). Its version depends
  # on the Windows build and is checked at runtime by the test `icu.version`
  if(NOT GAIA_OS_WINDOWS)
    find_package(ICU ${ROCKET_ICU_VERSION} REQUIRED COMPONENTS i18n uc)
  endif()
endif()

# Boost.Int128 ..............................................................................................

if(NOT ROCKET_HAS_INT128)
  list(APPEND ROCKET_BOOST_COMPONENTS int128)

  FetchContent_Declare(
    int128
    GIT_REPOSITORY https://github.com/boostorg/int128.git
    GIT_TAG        v${ROCKET_BOOST_INT128_VERSION}
    GIT_PROGRESS   TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )

  block()
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(int128)
  endblock()
endif()

# libarchive ................................................................................................

FetchContent_Declare(
  libarchive
  GIT_REPOSITORY https://github.com/libarchive/libarchive.git
  GIT_TAG        v${ROCKET_LIBARCHIVE_VERSION}
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

block()
  set(BUILD_SHARED_LIBS OFF)
  # libarchive's test setup references `${CMAKE_SOURCE_DIR}/build/cmake/...`, which breaks when it is not the
  # top-level project. We don't need its tests anyway
  set(ENABLE_TEST OFF)
  # libarchive adds `-Werror` in Debug builds, so deprecations in newer system libraries (e.g., lz4) break the
  # build
  set(ENABLE_WERROR OFF)
  FetchContent_MakeAvailable(libarchive)
endblock()

# libcbor ...................................................................................................

FetchContent_Declare(
  libcbor
  GIT_REPOSITORY https://github.com/PJK/libcbor.git
  GIT_TAG        v${ROCKET_LIBCBOR_VERSION}
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

block()
  set(BUILD_SHARED_LIBS OFF)
  set(SANITIZE OFF) #
  FetchContent_MakeAvailable(libcbor)
endblock()

# scnlib ....................................................................................................

# The patch:
#
# - Instantiates the built-in scanners for `bool`, `char32_t`, and `void*`, which `master` misses
# - Fixes `FILE` input being returned twice after a scan backtracks across a buffer refill
# - Makes scanning a `system_clock` time point take the fields as UTC instead of local time
# - Makes `%Y` accept a negative year with up to five digits, as {fmt} writes it, such as `-005` or `-10000`
# - Makes the fraction of `%.S` optional and rounds it to the nearest nanosecond instead of truncating it
# - Makes `%z` accept `Z` (Zulu time) as the UTC offset +00:00, as in RFC 3339, and reject minutes above 59
# - Makes the format-string checker return after an out-of-range argument ID instead of indexing its argument
#   types out of bounds, which clang-tidy reports as `clang-analyzer-security.ArrayBound`
#
# Its hash is part of the command, so that editing the patch repopulates scnlib
set(ROCKET_SCNLIB_PATCH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/scnlib.patch")
file(SHA256 "${ROCKET_SCNLIB_PATCH}" ROCKET_SCNLIB_PATCH_HASH)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ROCKET_SCNLIB_PATCH}")

FetchContent_Declare(
  scnlib
  GIT_REPOSITORY https://github.com/eliaskosunen/scnlib.git
  GIT_TAG        ${ROCKET_SCNLIB_VERSION}
  GIT_PROGRESS   TRUE
  PATCH_COMMAND  ${CMAKE_COMMAND} -DPATCH=${ROCKET_SCNLIB_PATCH} -DPATCH_HASH=${ROCKET_SCNLIB_PATCH_HASH}
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply-patch.cmake
  SYSTEM
  EXCLUDE_FROM_ALL
)

block()
  set(BUILD_SHARED_LIBS OFF)
  FetchContent_MakeAvailable(scnlib)
endblock()

# wxWidgets .................................................................................................

if(ROCKET_GUI)
  FetchContent_Declare(
    wxWidgets
    GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
    GIT_TAG        v${ROCKET_WXWIDGETS_VERSION}
    GIT_PROGRESS   TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )

  block()
    # Don’t change `BUILD_SHARED_LIBS` here
    set(wxUSE_UNICODE_UTF8 ON)
    FetchContent_MakeAvailable(wxWidgets)
  endblock()
endif()

# Add subdirectories ----------------------------------------------------------------------------------------

if(BUILD_TESTING AND ROCKET_BUILD_BENCH)
  enable_testing()
  add_subdirectory(src/bench)
endif()
add_subdirectory(src/bin)
if(ROCKET_MASTER_PROJECT)
  add_subdirectory(src/experimental)
endif()
add_subdirectory(src/main)
if(BUILD_TESTING AND ROCKET_BUILD_TEST)
  enable_testing()
  add_subdirectory(src/test)
endif()

# EOF
