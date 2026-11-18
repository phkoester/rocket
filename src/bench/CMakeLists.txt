# CMakeLists.txt

# Benchmarks ------------------------------------------------------------------------------------------------

rocketAddBench(bench-rocket-nio rocket/nio bench-nio.cc)

rocketAddBench(bench-rocket-map rocket bench-map.cc)

# EOF
