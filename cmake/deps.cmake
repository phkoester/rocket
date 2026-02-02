#
# deps.cmake
#

include(FetchContent)

# Boost -----------------------------------------------------------------------------------------------------

if(ROCKET_USE_EXTERNAL_BOOST)
  find_package(Boost ${ROCKET_BOOST_VERSION} CONFIG)
endif()
if(Boost_FOUND)
  set(ROCKET_BOOST_LINK_TARGETS Boost::headers)
  set(ROCKET_BOOST_EXPORT_TARGETS)
else()
  FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-${ROCKET_BOOST_VERSION}/boost-${ROCKET_BOOST_VERSION}-cmake.7z
    SYSTEM
    EXCLUDE_FROM_ALL
  )

  set(ROCKET_BOOST_LIBS algorithm bimap headers preprocessor safe_numerics)
  set(ROCKET_BOOST_NS_LIBS ${ROCKET_BOOST_LIBS})
  list(TRANSFORM ROCKET_BOOST_NS_LIBS PREPEND Boost::)

  set(BOOST_ENABLE_CMAKE ON)
  set(BOOST_INCLUDE_LIBRARIES ${ROCKET_BOOST_LIBS})
  # Build static libraries
  set(BUILD_SHARED_LIBS OFF)
  FetchContent_MakeAvailable(Boost)
  set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT})

  set(ROCKET_BOOST_LINK_TARGETS ${ROCKET_BOOST_NS_LIBS})
  set(ROCKET_BOOST_EXPORT_TARGETS
    boost_assert boost_bimap boost_bind boost_concept_check boost_config boost_container_hash boost_core
    boost_describe boost_detail boost_function boost_functional boost_fusion boost_headers
    boost_function_types boost_io boost_integer boost_iterator boost_lambda boost_logic
    boost_move boost_mp11 boost_mpl boost_multi_index boost_optional boost_predef boost_preprocessor
    boost_safe_numerics boost_smart_ptr boost_static_assert boost_tuple boost_type_traits
    boost_throw_exception boost_typeof boost_utility
  )
endif()

# fmt -------------------------------------------------------------------------------------------------------

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        12.1.0
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(fmt)

# googletest ------------------------------------------------------------------------------------------------

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Build static libraries
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(googletest)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT})

# benchmark -------------------------------------------------------------------------------------------------

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.5
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF)
# Build static libraries
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(benchmark)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT})

# ICU -------------------------------------------------------------------------------------------------------

find_package(ICU 74.2 REQUIRED uc) # data i18n io

# scnlib ----------------------------------------------------------------------------------------------------

FetchContent_Declare(
  scnlib
  GIT_REPOSITORY https://github.com/eliaskosunen/scnlib.git
  GIT_TAG        master
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

# Build static libraries
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(scnlib)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT})

# EOF
