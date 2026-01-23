#
# CMakeLists.txt
#

# Tests -----------------------------------------------------------------------------------------------------

include(GoogleTest)

# Rocket ....................................................................................................

AddTest(test-rocket rocket/test-rocket.cc)
AddTest(test-rocket-assert rocket/test-assert.cc)
AddTest(test-rocket-Cow rocket/test-Cow.cc)
AddTest(test-rocket-enum rocket/test-enum.cc)
AddTest(test-rocket-Exception rocket/test-Exception.cc)
AddTest(test-rocket-Guard rocket/test-Guard.cc)
AddTest(test-rocket-hash rocket/test-hash.cc)
AddTest(test-rocket-macro rocket/test-macro.cc)
AddTest(test-rocket-numeric rocket/test-numeric.cc)
AddTest(test-rocket-Process rocket/test-Process.cc)
AddTest(test-rocket-Type rocket/test-Type.cc)
AddTest(test-rocket-UnorderedBimap rocket/test-UnorderedBimap.cc)
AddTest(test-rocket-3rdparty-boost-spirit-x3 rocket/3rdparty/test-boost-spirit-x3.cc)
AddTest(test-rocket-3rdparty-C++ rocket/3rdparty/test-C++.cc)
AddTest(test-rocket-3rdparty-fmt rocket/3rdparty/test-fmt.cc)
AddTest(test-rocket-3rdparty-icu rocket/3rdparty/test-icu.cc)
AddTest(test-rocket-3rdparty-scnlib rocket/3rdparty/test-scnlib.cc)
AddTest(test-rocket-3rdparty-std rocket/3rdparty/test-std.cc)
AddTest(test-rocket-cl rocket/cl/test-cl.cc)
AddTest(test-rocket-format rocket/format/test-format.cc)
AddTest(test-rocket-format-std rocket/format/test-format.cc)
AddTest(test-rocket-log rocket/log/test-log.cc)
AddTest(test-rocket-math rocket/math/test-math.cc)
AddTest(test-rocket-math-Interval rocket/math/test-Interval.cc)
AddTest(test-rocket-nio rocket/nio/test-nio.cc)
AddTest(test-rocket-reflect rocket/reflect/test-reflect.cc)
AddTest(test-rocket-str rocket/str/test-str.cc)
AddTest(test-rocket-str-StringConvert rocket/str/test-StringConvert.cc)
AddTest(test-rocket-str-escape rocket/str/escape/test-escape.cc)
AddTest(test-rocket-str-location rocket/str/location/test-location.cc)
AddTest(test-rocket-system rocket/system/test-system.cc)
AddTest(test-rocket-system-terminal rocket/system/terminal/test-terminal.cc)
AddTest(test-rocket-unicode rocket/unicode/test-unicode.cc)
AddTest(test-rocket-unicode-Character rocket/unicode/test-Character.cc)
AddTest(test-rocket-unicode-Iterator rocket/unicode/test-Iterator.cc)

# Rocket Test ...............................................................................................

AddTest(test-rocket-test-matcher rocket-test/matcher/test-matcher.cc)

# EOF
