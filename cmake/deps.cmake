#
# deps.cmake
#

# Boost -----------------------------------------------------------------------------------------------------

set(BOOST_ENABLE_CMAKE ON)
set(BOOST_INCLUDE_LIBRARIES ${ROCKET_BOOST_LIBS})

set(ROCKET_BOOST_LIBS algorithm asio bimap headers iostreams preprocessor safe_numerics)
set(ROCKET_BOOST_NS_LIBS ${ROCKET_BOOST_LIBS})
list(TRANSFORM ROCKET_BOOST_NS_LIBS PREPEND Boost::)
set(ROCKET_BOOST_LINK_TARGETS ${ROCKET_BOOST_NS_LIBS})

find_package(Boost ${GAIA_BOOST_VERSION} QUIET)
if(NOT Boost_FOUND)
  block()
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(Boost)
  endblock()
endif()

# fmt -------------------------------------------------------------------------------------------------------

set(FMT_MODULE OFF CACHE BOOL "Build a module library" FORCE)

find_package(fmt ${GAIA_FMT_VERSION} QUIET)
if(NOT fmt_FOUND)
  FetchContent_MakeAvailable(fmt)
endif()

# GTest -----------------------------------------------------------------------------------------------------

if(GAIA_OS_WINDOWS)
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

find_package(GTest ${GAIA_GTEST_VERSION} QUIET)
if(NOT GTest_FOUND)
  block()
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(GTest)
  endblock()
endif()

if(GAIA_CXX_COMPILER_CLANG)
  target_compile_options(gtest PRIVATE -Wno-character-conversion)
endif()

# benchmark (must follow GTest) -----------------------------------------------------------------------------

set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF)

find_package(benchmark ${GAIA_BENCHMARK_VERSION} QUIET)
if(NOT benchmark_FOUND)
  block()
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(benchmark)
  endblock()
endif()

# libcbor ---------------------------------------------------------------------------------------------------

find_package(libcbor ${GAIA_LIBCBOR_VERSION} QUIET)
if(NOT libcbor_FOUND)
  block()
    set(BUILD_SHARED_LIBS OFF)
    set(SANITIZE OFF)
    FetchContent_MakeAvailable(libcbor)
  endblock()
endif()

if(GAIA_CXX_COMPILER_CLANG)
  target_compile_options(cbor PRIVATE -Wno-unused-variable)
endif()

# ICU -------------------------------------------------------------------------------------------------------

find_package(ICU ${GAIA_ICU_VERSION} COMPONENTS uc) # data i18n io

# int128 ----------------------------------------------------------------------------------------------------

find_package(int128 ${GAIA_INT128_VERSION} QUIET)
if(NOT int128_FOUND)
  block()
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(int128)
  endblock()
endif()

# scnlib ----------------------------------------------------------------------------------------------------

# find_package(scnlib ${GAIA_SCNLIB_VERSION} QUIET) # Commented out because version is `master`
if(NOT scnlib_FOUND)
  block()
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(scnlib)
  endblock()
endif()

# EOF
