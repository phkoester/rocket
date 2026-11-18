#
# CMakeLists.txt
#

include(GoogleTest)

# Test executables ------------------------------------------------------------------------------------------

gaiaAddExecutable(logger logger/main.cc)
target_link_libraries(logger PRIVATE Rocket::rocket)

gaiaAddExecutable(print-args print-args/main.cc)
target_link_libraries(print-args PRIVATE Rocket::rocket)

# Tests -----------------------------------------------------------------------------------------------------

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Rocket ....................................................................................................

rocketAddTest(test-rocket                              rocket                 test-rocket.cc)
rocketAddTest(test-rocket-Cow                          rocket                 test-Cow.cc)
rocketAddTest(test-rocket-Exception                    rocket                 test-Exception.cc)
rocketAddTest(test-rocket-InputFailure                 rocket                 test-InputFailure.cc)
rocketAddTest(test-rocket-Process                      rocket                 test-Process.cc)
rocketAddTest(test-rocket-TypeId                       rocket                 test-TypeId.cc)
rocketAddTest(test-rocket-assert                       rocket                 test-assert.cc)
rocketAddTest(test-rocket-enum                         rocket                 test-enum.cc)
rocketAddTest(test-rocket-format                       rocket                 test-format.cc)
rocketAddTest(test-rocket-functional                   rocket                 test-functional.cc)
rocketAddTest(test-rocket-icu                          rocket                 test-icu.cc)
rocketAddTest(test-rocket-literal                      rocket                 test-literal.cc)
rocketAddTest(test-rocket-macro                        rocket                 test-macro.cc)
rocketAddTest(test-rocket-scan                         rocket                 test-scan.cc)
rocketAddTest(test-rocket-scope                        rocket                 test-scope.cc)
rocketAddTest(test-rocket-type-traits                  rocket                 test-type-traits.cc)
rocketAddTest(test-rocket-chrono-time_zone             rocket/chrono          test-time_zone.cc)
rocketAddTest(test-rocket-cl                           rocket/cl              test-cl.cc)
rocketAddTest(test-rocket-codec                        rocket/codec           test-codec.cc)
rocketAddTest(test-rocket-codec-CborCodec              rocket/codec           test-CborCodec.cc)
rocketAddTest(test-rocket-codec-CompareEncoder         rocket/codec           test-CompareEncoder.cc)
rocketAddTest(test-rocket-codec-EqualToEncoder         rocket/codec           test-EqualToEncoder.cc)
rocketAddTest(test-rocket-codec-FormattedCodec         rocket/codec           test-FormattedCodec.cc)
rocketAddTest(test-rocket-codec-HashEncoder            rocket/codec           test-HashEncoder.cc)
rocketAddTest(test-rocket-codec-JsonCodec              rocket/codec           test-JsonCodec.cc)
rocketAddTest(test-rocket-external-Cxx                 rocket/external        test-Cxx.cc)
rocketAddTest(test-rocket-external-std                 rocket/external        test-std.cc)
rocketAddTest(test-rocket-format-Subformat             rocket/format          test-Subformat.cc)
rocketAddTest(test-rocket-hash                         rocket/hash            test-hash.cc)
rocketAddTest(test-rocket-io                           rocket/io              test-io.cc)
rocketAddTest(test-rocket-io-spanstream                rocket/io              test-spanstream.cc)
rocketAddTest(test-rocket-log                          rocket/log             test-log.cc)
rocketAddTest(test-rocket-math                         rocket/math            test-math.cc)
rocketAddTest(test-rocket-math-interval                rocket/math            test-Interval.cc)
rocketAddTest(test-rocket-nio                          rocket/nio             test-nio.cc)
rocketAddTest(test-rocket-nio-utils                    rocket/nio             test-nio-utils.cc)
rocketAddTest(test-rocket-reflect                      rocket/reflect         test-reflect.cc)
rocketAddTest(test-rocket-str                          rocket/str             test-str.cc)
rocketAddTest(test-rocket-str-split                    rocket/str             test-split.cc)
rocketAddTest(test-rocket-str-Range                    rocket/str             test-Range.cc)
rocketAddTest(test-rocket-str-StringConvert            rocket/str             test-StringConvert.cc)
rocketAddTest(test-rocket-str-escape                   rocket/str/escape      test-escape.cc)
rocketAddTest(test-rocket-str-location                 rocket/str/location    test-location.cc)
rocketAddTest(test-rocket-system                       rocket/system          test-system.cc)
rocketAddTest(test-rocket-system-terminal              rocket/system/terminal test-terminal.cc)
rocketAddTest(test-rocket-unicode                      rocket/unicode         test-unicode.cc)
rocketAddTest(test-rocket-unicode-Character            rocket/unicode         test-Character.cc)
rocketAddTest(test-rocket-unicode-Iterator             rocket/unicode         test-Iterator.cc)

# `test-rocket-external-CMake`  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

set(GENERATE_HELLO ${PROJECT_SOURCE_DIR}/cmake/generate-hello)
gaiaPythonPath(${GENERATE_HELLO} GENERATE_HELLO_PYTHON)

set(HELLO_CC ${GENERATED_DIR}/hello.cc)

add_custom_command(
  OUTPUT   ${HELLO_CC}
  COMMAND  ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
  COMMAND  ${Python3_EXECUTABLE} ${GENERATE_HELLO_PYTHON} > ${HELLO_CC}
  DEPENDS  ${GENERATE_HELLO} ${PROJECT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_LIST_FILE}
  VERBATIM
)
rocketAddTest(test-rocket-external-CMake rocket/external test-CMake.cc
  ENVIRONMENT
    "A=A A"
    "B=B B")
target_sources(test-rocket-external-CMake PRIVATE ${HELLO_CC})

# Rocket.Test ...............................................................................................

rocketAddTest(test-rocket-test-matcher rocket-test/matcher test-matcher.cc)

# EOF
