#
# CMakeLists.txt
#

# Benchmarks ------------------------------------------------------------------------------------------------

AddBench(bench-rocket-nio rocket/nio bench-nio.cc)
AddRuntimeDlls(bench-rocket-nio)

# EOF
