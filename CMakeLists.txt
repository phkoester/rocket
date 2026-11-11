#
# CMakeLists.txt
#

cmake_minimum_required(VERSION 4.2)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/RocketFunctions.cmake)

include(FetchContent)

# Project ---------------------------------------------------------------------------------------------------

# Is this the master project?
if(NOT DEFINED ROCKET_MASTER_PROJECT)
  set(ROCKET_MASTER_PROJECT OFF)
  if(NOT DEFINED PROJECT_NAME)
    set(ROCKET_MASTER_PROJECT ON)
  endif()
endif()

project(rocket VERSION 0.3.5)

# Versions --------------------------------------------------------------------------------------------------

set(ROCKET_BENCHMARK_VERSION 1.9.5)
set(ROCKET_BOOST_VERSION 1.92.0)
set(ROCKET_FMT_VERSION 12.2.0)
set(ROCKET_GAIA_VERSION 1.2.4)
set(ROCKET_GTEST_VERSION 1.18.0)
set(ROCKET_ICU_VERSION 72.1)
set(ROCKET_ICU_CMAKE_VERSION 0.1.3)
set(ROCKET_ICU_CMAKE_ICU_VERSION 79.1)
set(ROCKET_INT128_VERSION 4.1.0)
set(ROCKET_LIBCBOR_VERSION 0.14.0)
set(ROCKET_SCNLIB_VERSION master)
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

# Configuration ---------------------------------------------------------------------------------------------

GaiaAddVar(ROCKET_BUILD_BENCH              BOOL   ${ROCKET_MASTER_PROJECT} "Build benchmarks")
GaiaAddVar(ROCKET_BUILD_TEST               BOOL   ${ROCKET_MASTER_PROJECT} "Build tests")
GaiaAddVar(ROCKET_COMPONENTS               STRING ""                       "Additional components to enable: `gui`")
GaiaAddVar(ROCKET_ICU_CMAKE                BOOL   OFF                      "Use icu-cmake to build ICU")
GaiaAddVar(ROCKET_IO_LOG                   BOOL   OFF                      "Enable logging of `rocket::io`")
GaiaAddVar(ROCKET_NIO_LOG                  BOOL   OFF                      "Enable logging of `rocket::nio`")
GaiaAddVar(ROCKET_NIO_NO_CONTIGUOUS_SOURCE BOOL   OFF                      "Disable contiguous-source optimization for `rocket::nio`")

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
  if(GAIA_CXX_COMPILER_LLVM)
    # Windows, LLVM
    list(APPEND COMPILE_FLAGS -Werror -Wextra # No `-Wall` on Windows
      -Wno-unknown-attributes)
  elseif(GAIA_CXX_COMPILER_MSVC)
    # Windows 10, needed by `<icu.h>` from the Windows SDK, which only declares its API for Windows 10
    list(APPEND COMPILE_DEFS _WIN32_WINNT=0x0A00)
    list(APPEND COMPILE_FLAGS /bigobj /wd4244 /Zc:preprocessor)
  endif()
endif()

# Configure components --------------------------------------------------------------------------------------

list(FIND ROCKET_COMPONENTS gui index__)
if(ROCKET_MASTER_PROJECT OR index__ GREATER_EQUAL 0)
  set(ROCKET_GUI ON)
else()
  set(ROCKET_GUI OFF)
endif()
GaiaStatus("ROCKET_GUI: ${ROCKET_GUI}")

# Dependencies ----------------------------------------------------------------------------------------------

# ICU root (macOS); must come before Boost ..................................................................

# Homebrew's `icu4c` is keg-only, so point `FindICU` at it. Otherwise it picks up the partial ICU headers
# from the macOS SDK, which come without a linkable `icuuc`/`icui18n`. This must come before Boost, whose
# Boost.Regex and Boost.Locale run `find_package(ICU)` themselves and would cache the SDK's `ICU_INCLUDE_DIR`

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
  iostreams
  preprocessor
  safe_numerics
)

if(GAIA_OS_MACOS)
  list(APPEND ROCKET_BOOST_COMPONENTS stacktrace)
endif()

find_package(Boost ${ROCKET_BOOST_VERSION} QUIET COMPONENTS ${ROCKET_BOOST_COMPONENTS})
if(NOT Boost_FOUND)
  GaiaStatus("Boost ${ROCKET_BOOST_VERSION} not found; fetching it")

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
  if(GAIA_CXX_COMPILER_LLVM)
    target_compile_options(gtest PRIVATE -Wno-character-conversion)
    target_compile_options(gmock PRIVATE -Wno-character-conversion)
  endif()
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

if(GAIA_OS_WINDOWS)
  list(APPEND ROCKET_BOOST_COMPONENTS int128)

  FetchContent_Declare(
    int128
    GIT_REPOSITORY https://github.com/boostorg/int128.git
    GIT_TAG        v${ROCKET_INT128_VERSION}
    GIT_PROGRESS   TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )

  block()
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(int128)
  endblock()
endif()

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
  if(GAIA_CXX_COMPILER_LLVM)
    target_compile_options(cbor PRIVATE -Wno-unused-variable)
  endif()
endblock()

# scnlib ....................................................................................................

FetchContent_Declare(
  scnlib
  GIT_REPOSITORY https://github.com/eliaskosunen/scnlib.git
  GIT_TAG        ${ROCKET_SCNLIB_VERSION}
  GIT_PROGRESS   TRUE
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

    foreach(target IN ITEMS wxscintilla)
      if(TARGET ${target})
        if(GAIA_CXX_COMPILER_GNU OR GAIA_CXX_COMPILER_LLVM)
          target_compile_options(${target} PRIVATE -Wno-deprecated-declarations)
        endif()
      endif()
    endforeach()
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
