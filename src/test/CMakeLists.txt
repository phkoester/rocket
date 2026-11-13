#
# CMakeLists.txt
#

include(GoogleTest)

# Test executables ------------------------------------------------------------------------------------------

GaiaAddExecutable(logger logger/main.cc)
target_link_libraries(logger PRIVATE Rocket::rocket)

GaiaAddExecutable(print-args print-args/main.cc)
target_link_libraries(print-args PRIVATE Rocket::rocket)

GaiaAddExecutable(read-stdin read-stdin/main.cc)
target_link_libraries(read-stdin PRIVATE Rocket::rocket)

# Tests -----------------------------------------------------------------------------------------------------

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Rocket ....................................................................................................

RocketAddTest(test-rocket-Bimap                        rocket                 test-Bimap.cc)
RocketAddTest(test-rocket-Cow                          rocket                 test-Cow.cc)
RocketAddTest(test-rocket-Exception                    rocket                 test-Exception.cc)
RocketAddTest(test-rocket-Process                      rocket                 test-Process.cc)
RocketAddTest(test-rocket-TypeId                       rocket                 test-TypeId.cc)
RocketAddTest(test-rocket-assert                       rocket                 test-assert.cc)
RocketAddTest(test-rocket-enum                         rocket                 test-enum.cc)
RocketAddTest(test-rocket-functional                   rocket                 test-functional.cc)
RocketAddTest(test-rocket-int128                       rocket                 test-int128.cc)
RocketAddTest(test-rocket-literal                      rocket                 test-literal.cc)
RocketAddTest(test-rocket-macro                        rocket                 test-macro.cc)
RocketAddTest(test-rocket-scope                        rocket                 test-scope.cc)
RocketAddTest(test-rocket-type-traits                  rocket                 test-type-traits.cc)
RocketAddTest(test-rocket-chrono                       rocket/chrono          test-chrono.cc)
RocketAddTest(test-rocket-cl                           rocket/cl              test-cl.cc)
RocketAddTest(test-rocket-codec                        rocket/codec           test-codec.cc)
RocketAddTest(test-rocket-codec-CborCodec              rocket/codec           test-CborCodec.cc)
RocketAddTest(test-rocket-codec-CompareEncoder         rocket/codec           test-CompareEncoder.cc)
RocketAddTest(test-rocket-codec-EqualToEncoder         rocket/codec           test-EqualToEncoder.cc)
RocketAddTest(test-rocket-codec-FormattedCodec         rocket/codec           test-FormattedCodec.cc)
RocketAddTest(test-rocket-codec-HashEncoder            rocket/codec           test-HashEncoder.cc)
RocketAddTest(test-rocket-codec-JsonCodec              rocket/codec           test-JsonCodec.cc)
RocketAddTest(test-rocket-external-Cxx                 rocket/external        test-Cxx.cc)
RocketAddTest(test-rocket-external-fmt                 rocket/external        test-fmt.cc)
RocketAddTest(test-rocket-external-icu                 rocket/external        test-icu.cc)
RocketAddTest(test-rocket-external-scnlib              rocket/external        test-scnlib.cc)
RocketAddTest(test-rocket-external-std                 rocket/external        test-std.cc)
RocketAddTest(test-rocket-format                       rocket/format          test-Subformat.cc)
RocketAddTest(test-rocket-format-std                   rocket/format          test-std.cc)
RocketAddTest(test-rocket-hash                         rocket/hash            test-hash.cc)
RocketAddTest(test-rocket-io                           rocket/io              test-io.cc)
RocketAddTest(test-rocket-io-spanstream                rocket/io              test-spanstream.cc)
RocketAddTest(test-rocket-log                          rocket/log             test-log.cc)
RocketAddTest(test-rocket-math                         rocket/math            test-math.cc)
RocketAddTest(test-rocket-math-interval                rocket/math            test-Interval.cc)
RocketAddTest(test-rocket-nio                          rocket/nio             test-nio.cc)
RocketAddTest(test-rocket-nio-utils                    rocket/nio             test-nio-utils.cc)
RocketAddTest(test-rocket-reflect                      rocket/reflect         test-reflect.cc)
RocketAddTest(test-rocket-str                          rocket/str             test-str.cc)
RocketAddTest(test-rocket-str-split                    rocket/str             test-split.cc)
RocketAddTest(test-rocket-str-Range                    rocket/str             test-Range.cc)
RocketAddTest(test-rocket-str-StringConvert            rocket/str             test-StringConvert.cc)
RocketAddTest(test-rocket-str-escape                   rocket/str/escape      test-escape.cc)
RocketAddTest(test-rocket-str-location                 rocket/str/location    test-location.cc)
RocketAddTest(test-rocket-system                       rocket/system          test-system.cc)
RocketAddTest(test-rocket-system-terminal              rocket/system/terminal test-terminal.cc)
RocketAddTest(test-rocket-unicode                      rocket/unicode         test-unicode.cc)
RocketAddTest(test-rocket-unicode-Character            rocket/unicode         test-Character.cc)
RocketAddTest(test-rocket-unicode-Iterator             rocket/unicode         test-Iterator.cc)

# `test-rocket-external-CMake`  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

set(GENERATE_HELLO ${PROJECT_SOURCE_DIR}/cmake/generate-hello)
GaiaPythonPath(${GENERATE_HELLO} GENERATE_HELLO_PYTHON)

set(HELLO_CC ${GENERATED_DIR}/hello.cc)

add_custom_command(
  OUTPUT   ${HELLO_CC}
  COMMAND  ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
  COMMAND  ${Python3_EXECUTABLE} ${GENERATE_HELLO_PYTHON} > ${HELLO_CC}
  DEPENDS  ${GENERATE_HELLO} ${PROJECT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_LIST_FILE}
  VERBATIM
)
RocketAddTest(test-rocket-external-CMake rocket/external test-CMake.cc
  ENVIRONMENT
    "A=A A"
    "B=B B")
target_sources(test-rocket-external-CMake PRIVATE ${HELLO_CC})

# Rocket.Test ...............................................................................................

RocketAddTest(test-rocket-test-matcher rocket-test/matcher test-matcher.cc)

# EOF
