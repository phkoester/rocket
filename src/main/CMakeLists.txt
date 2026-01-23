#
# CMakeLists.txt
#

# Rocket ----------------------------------------------------------------------------------------------------

# Propressor definitions ....................................................................................

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

if(LINUX)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_OS_LINUX)
elseif(WIN32)
  list(APPEND ROCKET_COMPILE_DEFS ROCKET_OS_WINDOWS)
endif()

# Add the library ...........................................................................................

add_library(rocket)
# Move `Process.cc` to front so the global `process` instance is available early
# Move `log.cc` to front so Rocket can use its own logging API
target_sources(rocket
  PRIVATE
    rocket/Process.cc
    rocket/log/log.cc

    rocket/Exception.cc
    rocket/rocket.cc
    rocket/Type.cc
    rocket/3rdparty/scanlib/impl.cc
    rocket/chrono/chrono.cc
    rocket/cl/cl.cc
    rocket/io/io.cc
    rocket/nio/nio.cc
    rocket/str/escape/escape.cc
    rocket/str/location/location.cc
    rocket/str/message/message.cc
    rocket/str/Noun.cc
    rocket/str/str.cc
    rocket/system/system.cc
    rocket/system/terminal/terminal.cc
    rocket/unicode/Iterator.cc
    rocket/unicode/unicode.cc
  PUBLIC
    FILE_SET HEADERS
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
      rocket/numeric.h
      rocket/Process.h
      rocket/rocket.h
      rocket/Type.h
      rocket/type-traits.h
      rocket/UnorderedBimap.h
      rocket/version.h
      rocket/3rdparty/boost-spirit-x3.h
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
set_target_properties(rocket PROPERTIES VERSION ${PROJECT_VERSION}) # XXX 0.1.0 SOVERSION 0.1)
target_compile_definitions(rocket
  PUBLIC
    ${COMPILE_DEFS}
    ${ROCKET_COMPILE_DEFS}
    BOOST_SPIRIT_UNICODE
    ROCKET_TEST_PRIVATE=private
    ROCKET_TEST_PROTECTED=protected
)
target_compile_features(rocket PUBLIC ${COMPILE_FEATURES})
target_compile_options(rocket PUBLIC ${COMPILE_FLAGS})
target_link_libraries(rocket
  PUBLIC fmt::fmt-header-only scn::scn
  INTERFACE ${ROCKET_BOOST_LINK_TARGETS} # XXX
  PRIVATE ICU::uc stdc++_libbacktrace
)

# Rocket Test -----------------------------------------------------------------------------------------------

add_library(rocket-test STATIC) # XXX
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
  PUBLIC GTest::gmock GTest::gtest rocket # XXX Mit rocket???
)

# `bare` ----------------------------------------------------------------------------------------------------

AddExecutable(bare bare/main.cc)

# `print-args` ----------------------------------------------------------------------------------------------

AddExecutable(print-args print-args/main.cc)
target_link_libraries(print-args PRIVATE rocket)

# `toy` -----------------------------------------------------------------------------------------------------

AddExecutable(toy toy/main.cc)
target_link_libraries(toy PRIVATE rocket)

# EOF
