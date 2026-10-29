#
# CMakeLists.txt
#

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# `rocket` --------------------------------------------------------------------------------------------------

# Preprocessor definitions ..................................................................................

set(ROCKET_COMPILE_DEFS)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_C_COMPILER_CLANG)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_C_COMPILER_GNU)
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_C_COMPILER_MSVC)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_CXX_COMPILER_CLANG)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_CXX_COMPILER_GNU)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_CXX_COMPILER_MSVC)
endif()

if(GAIA_OS_LINUX)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_OS_LINUX)
elseif(GAIA_OS_WINDOWS)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_OS_WINDOWS)
endif()

if(BUILD_SHARED_LIBS)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_SHARED_LIBS)
endif()
if(ROCKET_IO_LOG)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_IO_LOG)
endif()
if(ROCKET_NIO_LOG)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_NIO_LOG)
endif()
if(ROCKET_NIO_NO_CONTIGUOUS_SOURCE)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_NIO_NO_CONTIGUOUS_SOURCE)
endif()

# Link targets ..............................................................................................

set(ROCKET_PRIVATE_LINK_TARGETS)

if(GAIA_OS_LINUX)
  list(APPEND ROCKET_PRIVATE_LINK_TARGETS stdc++exp)
endif()

# `version.h` ...............................................................................................

set(VERSION_H ${GENERATED_DIR}/rocket/version.h)
GaiaPythonPath(${VERSION_H} VERSION_H_PYTHON)

set(GENERATE_VERSION ${PROJECT_SOURCE_DIR}/cmake/generate-version)
GaiaPythonPath(${GENERATE_VERSION} GENERATE_VERSION_PYTHON)

add_custom_command(
  OUTPUT   ${VERSION_H}
  COMMAND  ${Python3_EXECUTABLE} ${GENERATE_VERSION_PYTHON} ROCKET ${PROJECT_VERSION} -o ${VERSION_H_PYTHON}
  DEPENDS  ${GENERATE_VERSION} ${PROJECT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_LIST_FILE}
  VERBATIM
)

# Add the library ...........................................................................................

GaiaAddLibrary(rocket ${VERSION_H})
set_target_properties(rocket PROPERTIES VERSION ${PROJECT_VERSION}) # SOVERSION 0
target_include_directories(rocket SYSTEM
  PRIVATE ${CMAKE_BINARY_DIR}/_deps/scnlib-src/src # Make `scn/impl.h` available
  PUBLIC  ${GENERATED_DIR}
)
target_sources(rocket
  PRIVATE
    rocket/Exception.cc
    rocket/Process.cc
    rocket/literal.cc
    rocket/chrono/chrono.cc
    rocket/cl/cl.cc
    rocket/codec/CborCodec.cc
    rocket/codec/JsonCodec.cc
    rocket/codec/codec.cc
    rocket/external/scnlib/impl.cc
    rocket/filesystem/filesystem.cc
    rocket/io/io.cc
    rocket/log/log.cc
    rocket/nio/nio.cc
    rocket/nio/nio-utils.cc
    rocket/str/Noun.cc
    rocket/str/str.cc
    rocket/str/StringConvert.cc
    rocket/str/escape/escape.cc
    rocket/str/location/location.cc
    rocket/str/message/message.cc
    rocket/system/system.cc
    rocket/system/terminal/terminal.cc
    rocket/unicode/Iterator.cc
    rocket/unicode/unicode.cc
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${GENERATED_DIR}
    FILES
      rocket/assert.h
      rocket/Bimap.h
      rocket/Bimap-codec.h
      rocket/Cow.h
      rocket/Exception.h
      rocket/Guard.h
      rocket/InputFailure.h
      rocket/Lazy.h
      rocket/Process.h
      rocket/Result.h
      rocket/enum.h
      rocket/format.h
      rocket/functional.h
      rocket/macro.h
      rocket/rocket.h
      rocket/scan.h
      rocket/std.h
      rocket/std-codec.h
      rocket/type-traits.h
      rocket/chrono/chrono.h
      rocket/cl/cl.h
      rocket/codec/CborCodec.h
      rocket/codec/CompareEncoder.h
      rocket/codec/EqualToEncoder.h
      rocket/codec/FormattedCodec.h
      rocket/codec/HashEncoder.h
      rocket/codec/JsonCodec.h
      rocket/codec/codec.h
      rocket/filesystem/filesystem.h
      rocket/format/Subformat.h
      rocket/hash/hash.h
      rocket/io/io.h
      rocket/log/log.h
      rocket/math/Interval.h
      rocket/math/Interval-codec.h
      rocket/math/math.h
      rocket/math/random/random.h
      rocket/nio/nio.h
      rocket/nio/nio-fwd.h
      rocket/nio/nio-utils.h
      rocket/reflect/Declared.h
      rocket/reflect/Instance.h
      rocket/reflect/MemberRef.h
      rocket/reflect/VarRef.h
      rocket/reflect/reflect.h
      rocket/str/Noun.h
      rocket/str/Range.h
      rocket/str/str.h
      rocket/str/StringConvert.h
      rocket/str/escape/escape.h
      rocket/str/location/location.h
      rocket/str/message/message.h
      rocket/system/system.h
      rocket/system/terminal/terminal.h
      rocket/unicode/Character.h
      rocket/unicode/ConvertTo.h
      rocket/unicode/Iterator.h
      rocket/unicode/unicode.h
      rocket/unicode/unicode-fwd.h
)
if(GAIA_OS_WINDOWS)
  target_sources(rocket PRIVATE rocket/system/system-windows.cc)
endif()
target_compile_definitions(rocket
  PRIVATE ROCKET_EXPORTING__
  PUBLIC  ${ROCKET_COMPILE_DEFS} ROCKET_TEST_PRIVATE=private ROCKET_TEST_PROTECTED=protected
)
target_link_libraries(rocket
  PRIVATE
    ${ROCKET_PRIVATE_LINK_TARGETS}
  PUBLIC
    ${ROCKET_BOOST_LINK_TARGETS}
    cbor
    fmt::fmt-header-only
    icu-cmake::uc
    scn::scn
)
add_library(Rocket::rocket ALIAS rocket)

# `rocket-bench` --------------------------------------------------------------------------------------------

GaiaAddLibrary(rocket-bench STATIC)
target_sources(rocket-bench
  PRIVATE
    rocket-bench/rocket-bench.cc
  PUBLIC
    FILE_SET HEADERS
    FILES
      rocket-bench/rocket-bench.h
)
target_link_libraries(rocket-bench PUBLIC benchmark::benchmark Rocket::rocket)
add_library(Rocket::rocket-bench ALIAS rocket-bench)

# `rocket-gui` ----------------------------------------------------------------------------------------------

if(ROCKET_GUI)
  GaiaAddLibrary(rocket-gui)
  set_target_properties(rocket-gui PROPERTIES VERSION ${PROJECT_VERSION}) # SOVERSION 0
  target_sources(rocket-gui PRIVATE rocket-gui/rocket-gui.cc)
  target_compile_definitions(rocket-gui PUBLIC ROCKET_GUI)
  target_link_libraries(rocket-gui PUBLIC Rocket::rocket wxWidgets::wxWidgets)
  add_library(Rocket::rocket-gui ALIAS rocket-gui)
endif()

# `rocket-test` ---------------------------------------------------------------------------------------------

GaiaAddLibrary(rocket-test STATIC)
target_sources(rocket-test
  PRIVATE
    rocket-test/rocket-test.cc
    rocket-test/TracingString.cc
  PUBLIC
    FILE_SET HEADERS
    FILES
      rocket-test/rocket-test.h
      rocket-test/TracingString.h
      rocket-test/matcher/matcher.h
)
target_compile_definitions(rocket-test PUBLIC ROCKET_TEST)
target_link_libraries(rocket-test PUBLIC GTest::gmock GTest::gtest Rocket::rocket)
add_library(Rocket::rocket-test ALIAS rocket-test)

# `rocket-test-main` ----------------------------------------------------------------------------------------

GaiaAddLibrary(rocket-test-main STATIC)
target_sources(rocket-test-main PRIVATE rocket-test-main/main.cc)
target_compile_definitions(rocket-test-main PRIVATE ROCKET_TEST)
target_link_libraries(rocket-test-main PRIVATE GTest::gmock GTest::gtest Rocket::rocket)
add_library(Rocket::rocket-test-main ALIAS rocket-test-main)

# EOF
