#
# CMakeLists.txt
#

# Benchmarks ------------------------------------------------------------------------------------------------

RocketAddBench(bench-rocket-nio rocket/nio bench-nio.cc)

RocketAddBench(bench-rocket-map rocket bench-map.cc)

# EOF
