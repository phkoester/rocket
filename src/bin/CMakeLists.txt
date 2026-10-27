#
# CMakeLists.txt
#

# Executables -----------------------------------------------------------------------------------------------

GaiaAddExecutable(bare bare/main.cc)

GaiaAddExecutable(logger logger/main.cc)
target_link_libraries(logger PRIVATE Rocket::rocket)
GaiaCopyRuntimeFiles(logger)

GaiaAddExecutable(print-args print-args/main.cc)
target_link_libraries(print-args PRIVATE Rocket::rocket)

GaiaAddExecutable(print-sizeof print-sizeof/main.cc)
target_link_libraries(print-sizeof PRIVATE Rocket::rocket)

# EOF
