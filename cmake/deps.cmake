#
# deps.cmake
#

# Boost -----------------------------------------------------------------------------------------------------

find_package(Boost ${GAIA_BOOST_VERSION} QUIET)
if(NOT Boost_FOUND)
  set(ROCKET_BOOST_LIBS algorithm asio bimap headers iostreams preprocessor safe_numerics)
  set(ROCKET_BOOST_NS_LIBS ${ROCKET_BOOST_LIBS})
  list(TRANSFORM ROCKET_BOOST_NS_LIBS PREPEND Boost::)

  set(BOOST_ENABLE_CMAKE ON)
  set(BOOST_INCLUDE_LIBRARIES ${ROCKET_BOOST_LIBS})
  # Build static libraries
  set(BUILD_SHARED_LIBS OFF)

  FetchContent_MakeAvailable(Boost)

  set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT})

  set(ROCKET_BOOST_LINK_TARGETS ${ROCKET_BOOST_NS_LIBS})
endif()

# fmt -------------------------------------------------------------------------------------------------------

FetchContent_MakeAvailable(fmt)

# GTest -----------------------------------------------------------------------------------------------------

find_package(GTest ${GAIA_GTEST_VERSION} QUIET)
if(NOT GTest_FOUND)
  # Build static libraries
  set(BUILD_SHARED_LIBS OFF)
  # Ignore a Clang warning
  if(GAIA_CXX_COMPILER_CLANG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=character-conversion")
  endif()
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  if(GAIA_OS_WINDOWS)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  endif()
  FetchContent_MakeAvailable(GTest)

  set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEFAULT}")
endif()

# benchmark (must follow GTest) -----------------------------------------------------------------------------

find_package(benchmark ${GAIA_BENCHMARK_VERSION} QUIET)
if(NOT benchmark_FOUND)
  set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF)
  # Build static libraries
  set(BUILD_SHARED_LIBS OFF)
  FetchContent_MakeAvailable(benchmark)
  set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT})
endif()

# ICU -------------------------------------------------------------------------------------------------------

find_package(ICU ${GAIA_ICU_VERSION} COMPONENTS uc) # data i18n io

# scnlib ----------------------------------------------------------------------------------------------------

# Build static libraries
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(scnlib)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT})

# EOF
