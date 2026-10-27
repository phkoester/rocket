#
# CMakeLists.txt
#

# Benchmarks ------------------------------------------------------------------------------------------------

GaiaAddBench(bench-rocket-nio rocket/nio bench-nio.cc)

GaiaAddBench(bench-rocket-map rocket bench-map.cc)

# EOF
