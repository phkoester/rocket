#
# CMakeLists.txt
#

cmake_minimum_required(VERSION 3.28)

# Project ---------------------------------------------------------------------------------------------------

# Is this the master project?
if(NOT DEFINED ROCKET_MASTER_PROJECT)
  set(ROCKET_MASTER_PROJECT OFF)
  if(NOT DEFINED PROJECT_NAME)
    set(ROCKET_MASTER_PROJECT ON)
  endif()
endif()

project(rocket VERSION 0.3.0)

# Configuration ---------------------------------------------------------------------------------------------

include(cmake/base.cmake)

AddVar(ROCKET_BUILD_BENCH BOOL ${ROCKET_MASTER_PROJECT} "Enable benchmarking and build benchmarks")
AddVar(ROCKET_BUILD_TEST  BOOL ${ROCKET_MASTER_PROJECT} "Enable testing and build tests")
AddVar(ROCKET_NIO_LOG     BOOL OFF                      "Enable logging of `rocket::nio`")

AddVar(ROCKET_BENCHMARK_VERSION STRING 1.9.5  "benchmark version")
AddVar(ROCKET_BOOST_VERSION     STRING 1.85.0 "Boost version")
AddVar(ROCKET_FMT_VERSION       STRING 12.1.0 "{fmt} version")
AddVar(ROCKET_GTEST_VERSION     STRING 1.17.0 "GTest version")
AddVar(ROCKET_ICU_VERSION       STRING 74.2   "ICU version")
AddVar(ROCKET_SCN_VERSION       STRING master "scnlib version")

# Set project-specific compiler definitions, features, and options ------------------------------------------

set(ROCKET_COMPILE_DEFS)

# Manage dependencies ---------------------------------------------------------------------------------------

include(cmake/deps.cmake)

# Add subdirectories ----------------------------------------------------------------------------------------

add_subdirectory(src/main)

if(BUILD_TESTING AND ROCKET_BUILD_BENCH)
  enable_testing()
  add_subdirectory(src/bench)
endif()

if(BUILD_TESTING AND ROCKET_BUILD_TEST)
  enable_testing()
  add_subdirectory(src/test)
endif()

# EOF
