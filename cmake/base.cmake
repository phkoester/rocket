#
# base.cmake
#
# ONLY EDIT THE ORIGINAL FILE, WHICH IS `gaia-base.cmake`.
#

# Check prerequisites ---------------------------------------------------------------------------------------

if(LINUX)
  set(GAIA_OS_LINUX ON)
else()
  set(GAIA_OS_LINUX OFF)
endif()
if(WIN32)
  set(GAIA_OS_WINDOWS ON)
else()
  set(GAIA_OS_WINDOWS OFF)
endif()
if(NOT(GAIA_OS_LINUX) AND NOT(GAIA_OS_WINDOWS))
  message(FATAL_ERROR "Unsupported OS ${CMAKE_SYSTEM_NAME}")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  set(GAIA_C_COMPILER_CLANG ON)
endif()
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(GAIA_C_COMPILER_GNU ON)
endif()
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  set(GAIA_C_COMPILER_MSVC ON)
endif()
if(NOT(GAIA_C_COMPILER_CLANG) AND NOT(GAIA_C_COMPILER_GNU) AND NOT(GAIA_C_COMPILER_MSVC))
  message(FATAL_ERROR "Unsupported C compiler ${CMAKE_C_COMPILER_ID}")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(GAIA_CXX_COMPILER_CLANG ON)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(GAIA_CXX_COMPILER_GNU ON)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(GAIA_CXX_COMPILER_MSVC ON)
endif()
if(NOT(GAIA_CXX_COMPILER_CLANG) AND NOT(GAIA_CXX_COMPILER_GNU) AND NOT(GAIA_CXX_COMPILER_MSVC))
  message(FATAL_ERROR "Unsupported C++ compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

# CMPs ------------------------------------------------------------------------------------------------------

# "More read-only target properties now error when trying to set them."
if(POLICY CMP0160)
  cmake_policy(SET CMP0160 NEW)
endif()

# Configuration ---------------------------------------------------------------------------------------------
#
# - CMake variables take precedence over environment variables.
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

AddVar(BUILD_SHARED_LIBS BOOL OFF "Build shared libraries")
AddVar(BUILD_TESTING     BOOL ON  "Enable testing and build tests")

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

# Set OS-specific compiler options --------------------------------------------------------------------------

if(LINUX)
  # gcc will not accept `__int128` with `-pedantic`
  list(APPEND COMPILE_FLAGS -Wall -Wextra -Wno-ignored-attributes)
elseif(WIN32)
  # In Windows, Clang understands MSVC-like flags
  list(APPEND COMPILE_FLAGS /WX /Zc:preprocessor)
endif()

# Functions -------------------------------------------------------------------------------------------------

function(CopyRuntimeDlls name)
  if(WIN32) # AND $<TARGET_RUNTIME_DLLS:${name}>
    add_custom_command(
      TARGET  ${name} POST_BUILD
      # Since CMake 4.2, there is `copy_if_newer`. If that is available, we can add this to `AddBench` and
      # `AddTest`. For the time being, Visual Studio comes with CMake 4.1.1
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${name}> $<TARGET_FILE_DIR:${name}>
      COMMAND_EXPAND_LISTS
    )
  endif()
endfunction()

# AddExecutable(name srcFile...)
function(AddExecutable name)
  add_executable(${name})
  target_sources(${name} PRIVATE ${ARGN})
  target_compile_definitions(${name} PRIVATE ${COMPILE_DEFS})
  target_compile_features(${name} PRIVATE ${COMPILE_FEATURES})
  target_compile_options(${name} PRIVATE ${COMPILE_FLAGS})
endfunction()

# ParseArgs__(srcFiles env  srcFile... [ENVIRONMENT name=value...])
function(ParseArgs__ srcFiles__ env__)
  set(srcFiles)
  set(env)
  set(appendTo srcFiles)
  foreach(it IN LISTS ARGN)
    if(it STREQUAL "ENVIRONMENT")
      set(appendTo env)
    else()
      list(APPEND ${appendTo} ${it})
    endif()
  endforeach()

  set(${srcFiles__} ${srcFiles} PARENT_SCOPE)
  set(${env__} ${env} PARENT_SCOPE)
endfunction()

# AddBench(name dir srcFile... [ENVIRONMENT name=value...])
function(AddBench name dir)
  ParseArgs__(srcFiles env ${ARGN})

  list(TRANSFORM srcFiles PREPEND "${dir}/")
  AddExecutable(${name} ${srcFiles})
  target_link_libraries(${name}
    PRIVATE
      benchmark::benchmark benchmark::benchmark_main Rocket::rocket Rocket::rocket-bench
  )

  add_test(
    NAME              ${name}
    COMMAND           ${name}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/bench/${dir}
  )

  string(JOIN " " configs ${CMAKE_CONFIGURATION_TYPES})

  set_property(TEST ${name}
    PROPERTY ENVIRONMENT
      "BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
      "CONFIG=$<CONFIG>"
      "CONFIGS=${configs}"
      ${env}
  )
endfunction()

# AddTest(name dir srcFile... [ENVIRONMENT name=value...])
function(AddTest name dir)
  ParseArgs__(srcFiles env ${ARGN})

  list(TRANSFORM srcFiles PREPEND "${dir}/")
  AddExecutable(${name} ${srcFiles})
  target_link_libraries(${name}
    PRIVATE
      Rocket::rocket-test-main Rocket::rocket-test)

  string(JOIN " " configs ${CMAKE_CONFIGURATION_TYPES})

  set(envProps)
  foreach(it IN LISTS env)
    list(APPEND envProps PROPERTIES ENVIRONMENT "${it}")
  endforeach()

  gtest_discover_tests(${name}
    DISCOVERY_MODE PRE_TEST
    EXTRA_ARGS --gtest_catch_exceptions=0
    PROPERTIES ENVIRONMENT "BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    PROPERTIES ENVIRONMENT "CONFIG=$<CONFIG>"
    PROPERTIES ENVIRONMENT "CONFIGS=${configs}"
    ${envProps}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/test/${dir}
  )
endfunction()

# EOF
