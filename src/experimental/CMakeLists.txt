#
# CMakeLists.txt
#

# Experimental ----------------------------------------------------------------------------------------------

AddExecutable(guy guy/main.cc)
target_link_libraries(guy PRIVATE Rocket::rocket-gui)
if (GAIA_OS_WINDOWS)
  target_link_options(guy PRIVATE /subsystem:windows)
endif()
CopyRuntimeFiles(guy)

AddExecutable(toy toy/main.cc)
target_link_libraries(toy PRIVATE Rocket::rocket)

# comm ......................................................................................................

AddLibrary(comm STATIC)
target_sources(comm PRIVATE comm/Message.cc)
target_link_libraries(comm PUBLIC Rocket::rocket)

AddExecutable(comm-local-client comm/comm-local-client.cc)
target_link_libraries(comm-local-client PRIVATE comm)

AddExecutable(comm-local-server comm/comm-local-server.cc)
target_link_libraries(comm-local-server PRIVATE comm)

AddExecutable(comm-tcp-client comm/comm-tcp-client.cc)
target_link_libraries(comm-tcp-client PRIVATE comm)

AddExecutable(comm-tcp-server comm/comm-tcp-server.cc)
target_link_libraries(comm-tcp-server PRIVATE comm)

# EOF
