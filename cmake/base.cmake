#
# base.cmake
#

# Check prerequisites ---------------------------------------------------------------------------------------

if(NOT(LINUX) AND NOT(WIN32))
  message(FATAL_ERROR "Unsupported OS ${CMAKE_SYSTEM_NAME}")
endif()
if (NOT(CMAKE_C_COMPILER_ID STREQUAL "Clang") AND
    NOT(CMAKE_C_COMPILER_ID STREQUAL "GNU") AND
    NOT(CMAKE_C_COMPILER_ID STREQUAL "MSVC"))
  message(FATAL_ERROR "Unsupported C compiler ${CMAKE_C_COMPILER_ID}")
endif()
if (NOT(CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND
    NOT(CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND
    NOT(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
  message(FATAL_ERROR "Unsupported C++ compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

# CMPs ------------------------------------------------------------------------------------------------------

# Suppress "The FindBoost module is removed."
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# Configuration ---------------------------------------------------------------------------------------------
#
# - Defined variables take precedence over environment variables.
# - Environment variables take precedence over defaults.
#
# -----------------------------------------------------------------------------------------------------------

function(AddVar name type default doc)
  if(NOT DEFINED ${name})
    if(NOT $ENV{${name}} STREQUAL "")
      set(${name} $ENV{${name}} CACHE ${type} "${doc}")
      message(STATUS "Setting ${name} by environment: ${${name}}")
    else()
      set(${name} ${default} CACHE ${type} "${doc}")
      message(STATUS "Setting ${name} to default: ${${name}}")
    endif()
  else()
    message(STATUS "Found ${name}: ${${name}}")
  endif()
endfunction()

AddVar(BUILD_SHARED_LIBS         BOOL OFF                      "Build shared libraries")
AddVar(BUILD_TESTING             BOOL ON                       "Enable testing and build tests")
AddVar(ROCKET_BENCH              BOOL OFF                      "Enable benchmarking and build benchmarks")
AddVar(ROCKET_TEST               BOOL ${ROCKET_MASTER_PROJECT} "Enable testing and build tests")
AddVar(ROCKET_USE_EXTERNAL_BOOST BOOL ON                       "Use external Boost library")

if(NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  AddVar(CMAKE_BUILD_TYPE STRING Release "The build type")
endif()

# General settings ------------------------------------------------------------------------------------------

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(FETCHCONTENT_QUIET FALSE)

set(BUILD_SHARED_LIBS_DEFAULT ${BUILD_SHARED_LIBS})

# Set compiler definitions, features, and options -----------------------------------------------------------

set(COMPILE_DEFS)
set(COMPILE_FEATURES cxx_std_23)
set(COMPILE_FLAGS)
set(ROCKET_COMPILE_DEFS)

# Set OS-specific compiler options --------------------------------------------------------------------------

if(LINUX)
  # gcc will not accept `__int128` with `-pedantic`
  list(APPEND COMPILE_FLAGS -Wall -Wextra -Wno-ignored-attributes)
elseif(WIN32)
  # list(APPEND COMPILE_FLAGS -Wall)
endif()

# Fetch dependencies ----------------------------------------------------------------------------------------

include(FetchContent)

# Boost .....................................................................................................

if(ROCKET_USE_EXTERNAL_BOOST)
  find_package(Boost 1.83)
endif()
if(Boost_FOUND)
  set(ROCKET_BOOST_LINK_TARGETS Boost::headers)
  set(ROCKET_BOOST_EXPORT_TARGETS)
else()
  FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.7z
    SYSTEM
    EXCLUDE_FROM_ALL
  )

  set(ROCKET_BOOST_LIBS bimap headers preprocessor safe_numerics)
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

# fmt .......................................................................................................

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        12.1.0
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(fmt)

# googletest ................................................................................................

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

# ICU -------------------------------------------------------------------------------------------------------

find_package(ICU 74.2 REQUIRED uc) # data i18n io

# scn .......................................................................................................

FetchContent_Declare(
  scn
  GIT_REPOSITORY https://github.com/eliaskosunen/scnlib.git
  GIT_TAG        master
  GIT_PROGRESS   TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

# Build static libraries
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(scn)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT})

# Functions -------------------------------------------------------------------------------------------------

# AddExecutable(name srcFile...)
function(AddExecutable name)
  add_executable(${name})
  target_sources(${name} PRIVATE ${ARGN})
  target_compile_definitions(${name} PRIVATE ${COMPILE_DEFS})
  target_compile_features(${name} PRIVATE ${COMPILE_FEATURES})
  target_compile_options(${name} PRIVATE ${COMPILE_FLAGS})
endfunction()

# AddBench(name dir srcFile...)
function(AddBench name dir)
  list(TRANSFORM ARGN PREPEND "${dir}/")
  AddExecutable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE Rocket::rocket-test)
  # add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/bench/${dir})
  gtest_discover_tests(${name}
    DISCOVERY_MODE POST_BUILD
    EXTRA_ARGS --gtest_catch_exceptions=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/bench/${dir}
  )
endfunction()

# AddTest(name dir srcFile...)
function(AddTest name dir)
  list(TRANSFORM ARGN PREPEND "${dir}/")
  AddExecutable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE Rocket::rocket-test)
  # add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/test/${dir})
  gtest_discover_tests(${name}
    DISCOVERY_MODE POST_BUILD
    EXTRA_ARGS --gtest_catch_exceptions=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/test/${dir}
  )
endfunction()

# EOF
