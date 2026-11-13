#
# CMakeLists.txt
#

# Executables -----------------------------------------------------------------------------------------------

GaiaAddExecutable(print-sizeof print-sizeof/main.cc)
target_link_libraries(print-sizeof PRIVATE Rocket::rocket)

GaiaAddExecutable(print-versions print-versions/main.cc)
target_link_libraries(print-versions PRIVATE Rocket::rocket)
if(ROCKET_GUI)
  target_link_libraries(print-versions PRIVATE Rocket::rocket-gui)
endif()

# EOF
