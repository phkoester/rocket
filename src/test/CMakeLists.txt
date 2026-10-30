#
# CMakeLists.txt
#

include(GoogleTest)

# Tests -----------------------------------------------------------------------------------------------------

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Rocket ....................................................................................................

RocketAddTest(test-rocket rocket test-rocket.cc)

set(GENERATE_HELLO ${PROJECT_SOURCE_DIR}/cmake/generate-hello)
GaiaPythonPath(${GENERATE_HELLO} GENERATE_HELLO_PYTHON)

set(HELLO_CC ${GENERATED_DIR}/hello.cc)
GaiaPythonPath(${HELLO_CC} HELLO_CC_PYTHON)

add_custom_command(
  OUTPUT   ${HELLO_CC}
  COMMAND  ${Python3_EXECUTABLE} ${GENERATE_HELLO_PYTHON} -o ${HELLO_CC_PYTHON}
  DEPENDS  ${GENERATE_HELLO} ${PROJECT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_LIST_FILE}
  VERBATIM
)
RocketAddTest(test-rocket-external-CMake rocket/external test-CMake.cc
  ENVIRONMENT
    "A=A A"
    "B=B B")
target_sources(test-rocket-external-CMake PRIVATE ${HELLO_CC})

RocketAddTest(test-rocket-assert                       rocket                 test-assert.cc)
RocketAddTest(test-rocket-Bimap                        rocket                 test-Bimap.cc)
RocketAddTest(test-rocket-Cow                          rocket                 test-Cow.cc)
RocketAddTest(test-rocket-enum                         rocket                 test-enum.cc)
RocketAddTest(test-rocket-Exception                    rocket                 test-Exception.cc)
RocketAddTest(test-rocket-functional                   rocket                 test-functional.cc)
RocketAddTest(test-rocket-Guard                        rocket                 test-Guard.cc)
RocketAddTest(test-rocket-macro                        rocket                 test-macro.cc)
RocketAddTest(test-rocket-Process                      rocket                 test-Process.cc)
RocketAddTest(test-rocket-type-traits                  rocket                 test-type-traits.cc)
RocketAddTest(test-rocket-cl                           rocket/cl              test-cl.cc)
RocketAddTest(test-rocket-codec                        rocket/codec           test-codec.cc)
RocketAddTest(test-rocket-codec-CborCodec              rocket/codec           test-CborCodec.cc)
RocketAddTest(test-rocket-codec-CompareEncoder         rocket/codec           test-CompareEncoder.cc)
RocketAddTest(test-rocket-codec-EqualToEncoder         rocket/codec           test-EqualToEncoder.cc)
RocketAddTest(test-rocket-codec-FormattedCodec         rocket/codec           test-FormattedCodec.cc)
RocketAddTest(test-rocket-codec-HashEncoder            rocket/codec           test-HashEncoder.cc)
RocketAddTest(test-rocket-codec-JsonCodec              rocket/codec           test-JsonCodec.cc)
RocketAddTest(test-rocket-external-boost-safe-numerics rocket/external        test-boost-safe-numerics.cc)
RocketAddTest(test-rocket-external-Cxx                 rocket/external        test-Cxx.cc)
RocketAddTest(test-rocket-external-fmt                 rocket/external        test-fmt.cc)
RocketAddTest(test-rocket-external-icu                 rocket/external        test-icu.cc)
RocketAddTest(test-rocket-external-scnlib              rocket/external        test-scnlib.cc)
RocketAddTest(test-rocket-external-std                 rocket/external        test-std.cc)
RocketAddTest(test-rocket-format                       rocket/format          test-Subformat.cc)
RocketAddTest(test-rocket-format-std                   rocket/format          test-std.cc)
RocketAddTest(test-rocket-hash                         rocket/hash            test-hash.cc)
RocketAddTest(test-rocket-io                           rocket/io              test-io.cc)
RocketAddTest(test-rocket-log                          rocket/log             test-log.cc)
RocketAddTest(test-rocket-math                         rocket/math            test-math.cc)
RocketAddTest(test-rocket-math-interval                rocket/math            test-Interval.cc)
RocketAddTest(test-rocket-nio                          rocket/nio             test-nio.cc)
RocketAddTest(test-rocket-reflect                      rocket/reflect         test-reflect.cc)
RocketAddTest(test-rocket-str                          rocket/str             test-str.cc)
RocketAddTest(test-rocket-str-Range                    rocket/str             test-Range.cc)
RocketAddTest(test-rocket-str-StringConvert            rocket/str             test-StringConvert.cc)
RocketAddTest(test-rocket-str-escape                   rocket/str/escape      test-escape.cc)
RocketAddTest(test-rocket-str-location                 rocket/str/location    test-location.cc)
RocketAddTest(test-rocket-system                       rocket/system          test-system.cc)
RocketAddTest(test-rocket-system-terminal              rocket/system/terminal test-terminal.cc)
RocketAddTest(test-rocket-unicode                      rocket/unicode         test-unicode.cc)
RocketAddTest(test-rocket-unicode-Character            rocket/unicode         test-Character.cc)
RocketAddTest(test-rocket-unicode-Iterator             rocket/unicode         test-Iterator.cc)

if(GAIA_OS_WINDOWS)
  RocketAddTest(test-rocket-external-boost-int128 rocket/external test-boost-int128.cc)
endif()

# Rocket.Test ...............................................................................................

RocketAddTest(test-rocket-test-matcher rocket-test/matcher test-matcher.cc)

# EOF
