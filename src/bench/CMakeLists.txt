#
# CMakeLists.txt
#

# Benchmarks ------------------------------------------------------------------------------------------------

AddBench(bench-rocket-nio rocket/nio bench-nio.cc)
AddRuntimeDlls(bench-rocket-nio)

AddBench(bench-rocket-map rocket bench-map.cc)

# EOF
