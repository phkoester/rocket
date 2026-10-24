#
# CMakeLists.txt
#

# Executables -----------------------------------------------------------------------------------------------

AddExecutable(bare bare/main.cc)

AddExecutable(logger logger/main.cc)
target_link_libraries(logger PRIVATE Rocket::rocket)

AddExecutable(print-args print-args/main.cc)
target_link_libraries(print-args PRIVATE Rocket::rocket)

AddExecutable(print-sizeof print-sizeof/main.cc)
target_link_libraries(print-sizeof PRIVATE Rocket::rocket)

# EOF
