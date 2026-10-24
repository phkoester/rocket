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

# XXX
# One or more XRC files containing your resources.
# set(xrc_files ${CMAKE_CURRENT_SOURCE_DIR}/resource.xrc)

# Generate this file somewhere under the build directory.
# set(resource_cpp ${CMAKE_CURRENT_BINARY_DIR}/resource.cpp)

# Not needed with the installed version, just use "wxrc".
# set(wxrc $<TARGET_FILE:wxrc>)

#add_custom_command(
#    OUTPUT ${resource_cpp}
#    COMMAND ${wxrc} -c -o ${resource_cpp} ${xrc_files}
#    DEPENDS ${xrc_files}
#    DEPENDS wxrc                        # Not needed with the installed version.
#    COMMENT "Compiling XRC resources"
#)

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
