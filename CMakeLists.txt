#
# CMakeLists.txt
#

cmake_minimum_required(VERSION 4.2)

include(FetchContent)

# Project ---------------------------------------------------------------------------------------------------

# Is this the master project?
if(NOT DEFINED ROCKET_MASTER_PROJECT)
  set(ROCKET_MASTER_PROJECT OFF)
  if(NOT DEFINED PROJECT_NAME)
    set(ROCKET_MASTER_PROJECT ON)
  endif()
endif()

project(rocket VERSION 0.3.0)

# Versions --------------------------------------------------------------------------------------------------

set(ROCKET_BENCHMARK_VERSION 1.9.5)
set(ROCKET_BOOST_VERSION 1.92.0)
set(ROCKET_FMT_VERSION 12.2.0)
set(ROCKET_GAIA_VERSION 1.2.0)
set(ROCKET_GTEST_VERSION 1.17.0)
set(ROCKET_ICU_CMAKE_VERSION 0.1.0)
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
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(FETCHCONTENT_QUIET FALSE)

# Configuration ---------------------------------------------------------------------------------------------

GaiaAddVar(ROCKET_BUILD_BENCH              BOOL   ${ROCKET_MASTER_PROJECT} "Enable benchmarking and build benchmarks")
GaiaAddVar(ROCKET_BUILD_TEST               BOOL   ${ROCKET_MASTER_PROJECT} "Enable testing and build tests")
GaiaAddVar(ROCKET_COMPONENTS               STRING ""                       "Additional components to enable: `gui`")
GaiaAddVar(ROCKET_IO_LOG                   BOOL   OFF                      "Enable logging of `rocket::io`")
GaiaAddVar(ROCKET_NIO_LOG                  BOOL   OFF                      "Enable logging of `rocket::nio`")
GaiaAddVar(ROCKET_NIO_NO_CONTIGUOUS_SOURCE BOOL   OFF                      "Disable contiguous-source optimization for `rocket::nio`")

# Set compiler and linker options ---------------------------------------------------------------------------

set(COMPILE_DEFS)
set(COMPILE_FEATURES)
set(COMPILE_FLAGS)
set(LINK_FLAGS)

if(GAIA_OS_LINUX)
  # Linux
  set(COMPILE_FLAGS -Wall -Werror -Wextra)
  if(GAIA_CXX_COMPILER_GNU)
    list(APPEND COMPILE_FLAGS -Wno-ignored-attributes)
  endif()
elseif(GAIA_OS_WINDOWS)
  # Windows
  if(GAIA_CXX_COMPILER_CLANG)
    # Windows, Clang
    set(COMPILE_FLAGS -Werror -Wextra)
  elseif(GAIA_CXX_COMPILER_MSVC)
    # Windows, MSVC
    set(COMPILE_FLAGS /bigobj /Zc:preprocessor)
  endif()
endif()

if(COVERAGE)
  list(APPEND COMPILE_FLAGS -coverage)
  list(APPEND LINK_FLAGS -coverage)
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

# Boost .....................................................................................................

FetchContent_Declare(
  Boost
  URL          https://github.com/boostorg/boost/releases/download/boost-${ROCKET_BOOST_VERSION}/boost-${ROCKET_BOOST_VERSION}-cmake.7z
  GIT_PROGRESS TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

set(ROCKET_BOOST_LIBS algorithm asio bimap headers iostreams preprocessor safe_numerics)
set(ROCKET_BOOST_LINK_TARGETS ${ROCKET_BOOST_LIBS})
list(TRANSFORM ROCKET_BOOST_LINK_TARGETS PREPEND Boost::)

block()
  set(BUILD_SHARED_LIBS OFF)
  set(BOOST_ENABLE_CMAKE ON)
  set(BOOST_INCLUDE_LIBRARIES ${ROCKET_BOOST_LIBS})
  FetchContent_MakeAvailable(Boost)
endblock()

# fmt .......................................................................................................

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

# GTest .....................................................................................................

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
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  endif()
  FetchContent_MakeAvailable(GTest)
  if(GAIA_CXX_COMPILER_CLANG)
    target_compile_options(gtest PRIVATE -Wno-character-conversion)
    target_compile_options(gmock PRIVATE -Wno-character-conversion)
  endif()
endblock()

# Benchmark (must appear after GTest) .......................................................................

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

# icu-cmake .................................................................................................

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

# int128 ....................................................................................................

if(GAIA_OS_WINDOWS)
  list(APPEND ROCKET_BOOST_LINK_TARGETS Boost::int128)

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
  if(GAIA_CXX_COMPILER_CLANG)
    target_compile_options(cbor PRIVATE -Wno-unused-variable) # XXX
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
    # If Rocket is built as shared libraries, wxWidgets must be built as shared libraries as well
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
# add_subdirectory(src/example)
if(ROCKET_MASTER_PROJECT)
  add_subdirectory(src/experimental)
endif()
add_subdirectory(src/main)
if(BUILD_TESTING AND ROCKET_BUILD_TEST)
  enable_testing()
  add_subdirectory(src/test)
endif()

# EOF
