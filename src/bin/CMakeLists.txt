# CMakeLists.txt

# Executables -----------------------------------------------------------------------------------------------

gaiaAddExecutable(bare bare/main.cc)

gaiaAddExecutable(print-env print-env/main.cc)
target_link_libraries(print-env PRIVATE Rocket::rocket)

gaiaAddExecutable(print-sizeof print-sizeof/main.cc)
target_link_libraries(print-sizeof PRIVATE Rocket::rocket)

gaiaAddExecutable(print-versions print-versions/main.cc)
target_link_libraries(print-versions PRIVATE Rocket::rocket)
if(ROCKET_GUI)
  target_link_libraries(print-versions PRIVATE Rocket::rocket-gui)
endif()

# EOF
