#
# CMakeLists.txt
#

# Benchmarks ------------------------------------------------------------------------------------------------

include(GoogleTest)

AddBench(bench-nio rocket/nio bench-nio.cc)
AddRuntimeDlls(bench-nio)

# EOF
