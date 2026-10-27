#
# CMakeLists.txt
#

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Tests -----------------------------------------------------------------------------------------------------

include(GoogleTest)

# Rocket ....................................................................................................

GaiaAddTest(test-rocket rocket test-rocket.cc)

GaiaUnixPath(${PROJECT_SOURCE_DIR} dir)
set(HELLO_CC ${GENERATED_DIR}/hello.cc)
add_custom_command(
  OUTPUT   ${HELLO_CC}
  COMMAND  python3 ${dir}/bin/generate-hello -o ${HELLO_CC}
  DEPENDS  ${PROJECT_SOURCE_DIR}/bin/generate-hello ${PROJECT_SOURCE_DIR}/CMakeLists.txt
  VERBATIM
)
GaiaAddTest(test-rocket-external-CMake rocket/external test-CMake.cc
  ENVIRONMENT
    "A=A A"
    "B=B B")
target_sources(test-rocket-external-CMake PRIVATE ${HELLO_CC})

GaiaAddTest(test-rocket-assert                       rocket                 test-assert.cc)
GaiaAddTest(test-rocket-Bimap                        rocket                 test-Bimap.cc)
GaiaAddTest(test-rocket-Cow                          rocket                 test-Cow.cc)
GaiaAddTest(test-rocket-enum                         rocket                 test-enum.cc)
GaiaAddTest(test-rocket-Exception                    rocket                 test-Exception.cc)
GaiaAddTest(test-rocket-functional                   rocket                 test-functional.cc)
GaiaAddTest(test-rocket-Guard                        rocket                 test-Guard.cc)
GaiaAddTest(test-rocket-macro                        rocket                 test-macro.cc)
GaiaAddTest(test-rocket-Process                      rocket                 test-Process.cc)
GaiaAddTest(test-rocket-type-traits                  rocket                 test-type-traits.cc)
GaiaAddTest(test-rocket-cl                           rocket/cl              test-cl.cc)
GaiaAddTest(test-rocket-codec                        rocket/codec           test-codec.cc)
GaiaAddTest(test-rocket-codec-CborCodec              rocket/codec           test-CborCodec.cc)
GaiaAddTest(test-rocket-codec-CompareEncoder         rocket/codec           test-CompareEncoder.cc)
GaiaAddTest(test-rocket-codec-EqualToEncoder         rocket/codec           test-EqualToEncoder.cc)
GaiaAddTest(test-rocket-codec-FormattedCodec         rocket/codec           test-FormattedCodec.cc)
GaiaAddTest(test-rocket-codec-HashEncoder            rocket/codec           test-HashEncoder.cc)
GaiaAddTest(test-rocket-codec-JsonCodec              rocket/codec           test-JsonCodec.cc)
GaiaAddTest(test-rocket-external-boost-int128        rocket/external        test-boost-int128.cc)
GaiaAddTest(test-rocket-external-boost-safe-numerics rocket/external        test-boost-safe-numerics.cc)
GaiaAddTest(test-rocket-external-Cxx                 rocket/external        test-Cxx.cc)
GaiaAddTest(test-rocket-external-fmt                 rocket/external        test-fmt.cc)
GaiaAddTest(test-rocket-external-icu                 rocket/external        test-icu.cc)
GaiaAddTest(test-rocket-external-scnlib              rocket/external        test-scnlib.cc)
GaiaAddTest(test-rocket-external-std                 rocket/external        test-std.cc)
GaiaAddTest(test-rocket-format                       rocket/format          test-Subformat.cc)
GaiaAddTest(test-rocket-format-std                   rocket/format          test-std.cc)
GaiaAddTest(test-rocket-hash                         rocket/hash            test-hash.cc)
GaiaAddTest(test-rocket-io                           rocket/io              test-io.cc)
GaiaAddTest(test-rocket-log                          rocket/log             test-log.cc)
GaiaAddTest(test-rocket-math                         rocket/math            test-math.cc)
GaiaAddTest(test-rocket-math-interval                rocket/math            test-Interval.cc)
GaiaAddTest(test-rocket-nio                          rocket/nio             test-nio.cc)
GaiaAddTest(test-rocket-reflect                      rocket/reflect         test-reflect.cc)
GaiaAddTest(test-rocket-str                          rocket/str             test-str.cc)
GaiaAddTest(test-rocket-str-Range                    rocket/str             test-Range.cc)
GaiaAddTest(test-rocket-str-StringConvert            rocket/str             test-StringConvert.cc)
GaiaAddTest(test-rocket-str-escape                   rocket/str/escape      test-escape.cc)
GaiaAddTest(test-rocket-str-location                 rocket/str/location    test-location.cc)
GaiaAddTest(test-rocket-system                       rocket/system          test-system.cc)
GaiaAddTest(test-rocket-system-terminal              rocket/system/terminal test-terminal.cc)
GaiaAddTest(test-rocket-unicode                      rocket/unicode         test-unicode.cc)
GaiaAddTest(test-rocket-unicode-Character            rocket/unicode         test-Character.cc)
GaiaAddTest(test-rocket-unicode-Iterator             rocket/unicode         test-Iterator.cc)

# Rocket Test ...............................................................................................

GaiaAddTest(test-rocket-test-matcher rocket-test/matcher test-matcher.cc)

# EOF
