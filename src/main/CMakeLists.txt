#
# CMakeLists.txt
#

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Rocket ----------------------------------------------------------------------------------------------------

# Propressor definitions ....................................................................................

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

if(LINUX)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_OS_LINUX)
elseif(WIN32)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_OS_WINDOWS)
endif()

# `version.h` -----------------------------------------------------------------------------------------------

set(VERSION_H ${GENERATED_DIR}/rocket/version.h)

add_custom_command(
  OUTPUT  ${VERSION_H}
  COMMAND python3 ${PROJECT_SOURCE_DIR}/cmake/generate-version.py ROCKET ${PROJECT_VERSION} -o ${VERSION_H}
  DEPENDS ${PROJECT_SOURCE_DIR}/CMakeLists.txt
  VERBATIM
)

# Add the library ...........................................................................................

set(ROCKET_PRIVATE_LINK_TARGETS)
if(LINUX)
  list(APPEND ROCKET_PRIVATE_LINK_TARGETS stdc++_libbacktrace)
endif()

add_library(rocket ${VERSION_H})
target_include_directories(rocket PUBLIC ${GENERATED_DIR})
# Move `Process.cc` to front so the global `process` instance is available early
# Move `log.cc` to front so Rocket can use its own logging API
target_sources(rocket
  PRIVATE
    rocket/Process.cc
    rocket/log/log.cc

    rocket/Exception.cc
    rocket/literal.cc
    rocket/rocket.cc
    rocket/3rdparty/scnlib/impl.cc
    rocket/chrono/chrono.cc
    rocket/cl/cl.cc
    rocket/io/io.cc
    rocket/nio/nio.cc
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
      rocket/Cow.h
      rocket/enum.h
      rocket/Exception.h
      rocket/Guard.h
      rocket/hash.h
      rocket/InputFailure.h
      rocket/Lazy.h
      rocket/macro.h
      rocket/Process.h
      rocket/rocket.h
      rocket/type-traits.h
      rocket/UnorderedBimap.h
      rocket/3rdparty/std.h
      rocket/chrono/chrono.h
      rocket/cl/cl.h
      rocket/format/format.h
      rocket/format/std.h
      rocket/io/io.h
      rocket/log/log.h
      rocket/math/Interval.h
      rocket/math/math.h
      rocket/math/random.h
      rocket/nio/nio.h
      rocket/nio/nio-fwd.h
      rocket/reflect/reflect.h
      rocket/scan/scan.h
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
)
set_target_properties(rocket PROPERTIES VERSION ${PROJECT_VERSION}) # SOVERSION 0
target_compile_definitions(rocket
  PRIVATE
    ROCKET_EXPORTING__
  PUBLIC
    ${COMPILE_DEFS}
    ${ROCKET_COMPILE_DEFS}
    ROCKET_TEST_PRIVATE=private
    ROCKET_TEST_PROTECTED=protected
)
target_compile_features(rocket PUBLIC ${COMPILE_FEATURES})
target_compile_options(rocket PUBLIC ${COMPILE_FLAGS})
target_link_libraries(rocket
  PUBLIC ${ROCKET_BOOST_LINK_TARGETS} fmt::fmt-header-only ICU::uc scn::scn
  PRIVATE ${ROCKET_PRIVATE_LINK_TARGETS}
)

add_library(Rocket::rocket ALIAS rocket)

# Rocket Test -----------------------------------------------------------------------------------------------

add_library(rocket-test STATIC)
target_sources(rocket-test
  PRIVATE
    rocket-test/main.cc
    rocket-test/rocket-test.cc
    rocket-test/TracingString.cc
  PUBLIC
    FILE_SET HEADERS
    FILES
      rocket-test/bench.h
      rocket-test/rocket-test.h
      rocket-test/TracingString.h
      rocket-test/matcher/matcher.h
)
target_compile_definitions(rocket-test
  PUBLIC
    ${COMPILE_DEFS}
    ROCKET_TEST
)
target_compile_features(rocket-test PUBLIC ${COMPILE_FEATURES})
target_compile_options(rocket-test PUBLIC ${COMPILE_FLAGS})
target_link_libraries(rocket-test
  PUBLIC GTest::gmock GTest::gtest Rocket::rocket
)

add_library(Rocket::rocket-test ALIAS rocket-test)

set(ROCKET_MAIN_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

# `bare` ----------------------------------------------------------------------------------------------------

AddExecutable(bare bare/main.cc)

# `print-args` ----------------------------------------------------------------------------------------------

AddExecutable(print-args print-args/main.cc)
target_link_libraries(print-args PRIVATE Rocket::rocket)

# `toy` -----------------------------------------------------------------------------------------------------

set(GENERATED_CC ${GENERATED_DIR}/toy/generated.cc)

add_custom_command(
  OUTPUT  ${GENERATED_CC}
  COMMAND python3 ${PROJECT_SOURCE_DIR}/cmake/generate.py -o ${GENERATED_CC}
  VERBATIM
)
AddExecutable(toy toy/main.cc ${GENERATED_CC})
target_link_libraries(toy PRIVATE Rocket::rocket)

# EOF
