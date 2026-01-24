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

# Include helper file ---------------------------------------------------------------------------------------

include(cmake/base.cmake)

# Add subdirectories ----------------------------------------------------------------------------------------

add_subdirectory(src/main)

if(ROCKET_BENCH)
  enable_testing()
  add_subdirectory(src/bench)
endif()

if(BUILD_TESTING AND ROCKET_TEST)
  enable_testing()
  add_subdirectory(src/test)
endif()

# EOF
