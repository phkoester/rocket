#
# CMakeLists.txt
#

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Auxiliary text executables --------------------------------------------------------------------------------

AddExecutable(bare bare/main.cc)

AddExecutable(logger logger/main.cc)
target_link_libraries(logger PRIVATE Rocket::rocket)

AddExecutable(print-args print-args/main.cc)
target_link_libraries(print-args PRIVATE Rocket::rocket)

AddExecutable(toy toy/main.cc)
target_link_libraries(toy PRIVATE Rocket::rocket)

# Tests -----------------------------------------------------------------------------------------------------

include(GoogleTest)

# Rocket ....................................................................................................

AddTest(test-rocket rocket test-rocket.cc)
CopyRuntimeDlls(test-rocket)

set(GENERATED_CC ${GENERATED_DIR}/generated.cc)
add_custom_command(
  OUTPUT  ${GENERATED_CC}
  COMMAND python3 ${PROJECT_SOURCE_DIR}/cmake/generate.py -o ${GENERATED_CC}
  VERBATIM
)
AddTest(test-rocket-external-CMake rocket/external test-CMake.cc
  ENVIRONMENT
    "A=A A"
    "B=B B")
target_sources(test-rocket-external-CMake PRIVATE ${GENERATED_CC})

AddTest(test-rocket-assert                       rocket                 test-assert.cc)
AddTest(test-rocket-Bimap                        rocket                 test-Bimap.cc)
AddTest(test-rocket-Cow                          rocket                 test-Cow.cc)
AddTest(test-rocket-enum                         rocket                 test-enum.cc)
AddTest(test-rocket-Exception                    rocket                 test-Exception.cc)
AddTest(test-rocket-functional                   rocket                 test-functional.cc)
AddTest(test-rocket-Guard                        rocket                 test-Guard.cc)
AddTest(test-rocket-macro                        rocket                 test-macro.cc)
AddTest(test-rocket-Process                      rocket                 test-Process.cc)
AddTest(test-rocket-type-traits                  rocket                 test-type-traits.cc)
AddTest(test-rocket-cl                           rocket/cl              test-cl.cc)
AddTest(test-rocket-codec                        rocket/codec           test-codec.cc)
AddTest(test-rocket-codec-CompareEncoder         rocket/codec           test-CompareEncoder.cc)
AddTest(test-rocket-codec-EqualToEncoder         rocket/codec           test-EqualToEncoder.cc)
AddTest(test-rocket-codec-FormattedCodec         rocket/codec           test-FormattedCodec.cc)
AddTest(test-rocket-codec-HashEncoder            rocket/codec           test-HashEncoder.cc)
AddTest(test-rocket-external-boost-safe-numerics rocket/external        test-boost-safe-numerics.cc)
AddTest(test-rocket-external-Cxx                 rocket/external        test-Cxx.cc)
AddTest(test-rocket-external-fmt                 rocket/external        test-fmt.cc)
AddTest(test-rocket-external-icu                 rocket/external        test-icu.cc)
AddTest(test-rocket-external-scnlib              rocket/external        test-scnlib.cc)
AddTest(test-rocket-external-std                 rocket/external        test-std.cc)
AddTest(test-rocket-format                       rocket/format          test-Subformat.cc)
AddTest(test-rocket-format-std                   rocket/format          test-std.cc)
AddTest(test-rocket-hash                         rocket/hash            test-hash.cc)
AddTest(test-rocket-log                          rocket/log             test-log.cc)
AddTest(test-rocket-math                         rocket/math            test-math.cc)
AddTest(test-rocket-math-interval                rocket/math            test-Interval.cc)
AddTest(test-rocket-nio                          rocket/nio             test-nio.cc)
AddTest(test-rocket-reflect                      rocket/reflect         test-reflect.cc)
AddTest(test-rocket-str                          rocket/str             test-str.cc)
AddTest(test-rocket-str-Range                    rocket/str             test-Range.cc)
AddTest(test-rocket-str-StringConvert            rocket/str             test-StringConvert.cc)
AddTest(test-rocket-str-escape                   rocket/str/escape      test-escape.cc)
AddTest(test-rocket-str-location                 rocket/str/location    test-location.cc)
AddTest(test-rocket-system                       rocket/system          test-system.cc)
AddTest(test-rocket-system-terminal              rocket/system/terminal test-terminal.cc)
AddTest(test-rocket-unicode                      rocket/unicode         test-unicode.cc)
AddTest(test-rocket-unicode-Character            rocket/unicode         test-Character.cc)
AddTest(test-rocket-unicode-Iterator             rocket/unicode         test-Iterator.cc)

# Rocket Test ...............................................................................................

AddTest(test-rocket-test-matcher rocket-test/matcher test-matcher.cc)

# EOF
