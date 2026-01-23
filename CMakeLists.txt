#
# CMakeLists.txt
#

cmake_minimum_required(VERSION 3.28)

# Project ---------------------------------------------------------------------------------------------------

if(NOT DEFINED ROCKET_MASTER_PROJECT)
  set(ROCKET_MASTER_PROJECT OFF)
  if(NOT DEFINED PROJECT_NAME)
    set(ROCKET_MASTER_PROJECT ON)
  endif()
endif()

project(rocket VERSION 0.3.0)

# Project options -------------------------------------------------------------------------------------------

option(BUILD_SHARED_LIBS "Build shared libraries"                   OFF)
option(BUILD_TESTING     "Enable testing and build tests"           ON)
option(ROCKET_BENCH      "Enable benchmarking and build benchmarks" OFF)
option(ROCKET_TEST       "Enable testing and build tests"           ${ROCKET_MASTER_PROJECT})

# Include helper file ---------------------------------------------------------------------------------------

include(cmake/base.cmake)

# Add subdirectories ----------------------------------------------------------------------------------------

add_subdirectory(src/main)

# XXX
if(ROCKET_BENCH)
  enable_testing()
  add_subdirectory(src/bench)
endif()

if(BUILD_TESTING AND ROCKET_TEST)
  enable_testing()
  add_subdirectory(src/test)
endif()

# Install ---------------------------------------------------------------------------------------------------

# XXX
set(ROCKET_EXPORT_TARGETS
  ${ROCKET_BOOST_EXPORT_TARGETS}
  fast_float
  fmt-header-only
  rocket
  rocket-test
)

# XXX
if(false)
include(GNUInstallDirs)

install(
  TARGETS seam scn fast_float fmt-header-only
  EXPORT ${PROJECT_NAME}-targets
  FILE_SET HEADERS
)

install(
  EXPORT ${PROJECT_NAME}-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  NAMESPACE ${PROJECT_NAME}::
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  COMPATIBILITY AnyNewerVersion
)

install(
  FILES
    cmake/${PROJECT_NAME}-config.cmake
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
endif()

# Export ----------------------------------------------------------------------------------------------------

# XXX
export(
  TARGETS   ${ROCKET_EXPORT_TARGETS}
  NAMESPACE ${PROJECT_NAME}::
  FILE      ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
)

# EOF
