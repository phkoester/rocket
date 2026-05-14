#
# CMakeLists.txt
#

# Benchmarks ------------------------------------------------------------------------------------------------

AddBench(bench-rocket-nio rocket/nio bench-nio.cc)

AddBench(bench-rocket-map rocket bench-map.cc)

# EOF
