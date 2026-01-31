#
# CMakeLists.txt
#

# Benchmarks ------------------------------------------------------------------------------------------------

# XXX include(GoogleTest)

AddBench(bench-rocket-nio rocket/nio bench-nio.cc)
# XXX AddRuntimeDlls(bench-rocket-nio)

# EOF
