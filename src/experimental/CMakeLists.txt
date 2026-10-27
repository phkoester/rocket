#
# CMakeLists.txt
#

# Excutables ------------------------------------------------------------------------------------------------

# guy .......................................................................................................

if(ROCKET_GUI)
  GaiaAddExecutable(guy guy/main.cc)
  target_link_libraries(guy PRIVATE Rocket::rocket-gui)
  if (GAIA_OS_WINDOWS)
    target_link_options(guy PRIVATE /subsystem:windows)
  endif()
  GaiaCopyRuntimeFiles(guy)

  set(xrc_files ${CMAKE_CURRENT_SOURCE_DIR}/guy/resource.xrc)
  set(resource_cc ${CMAKE_CURRENT_BINARY_DIR}/resource.cc)
  set(wxrc $<TARGET_FILE:wxrc>)

  add_custom_command(
    OUTPUT ${resource_cc}
    COMMAND ${wxrc} -c -o ${resource_cc} ${xrc_files}
    DEPENDS ${xrc_files}
            ${CMAKE_CURRENT_SOURCE_DIR}/guy/logo.svg
            ${CMAKE_CURRENT_SOURCE_DIR}/guy/back.svg
            ${CMAKE_CURRENT_SOURCE_DIR}/guy/forward.svg
            wxrc
    COMMENT "Compiling XRC resources"
  )

  target_sources(guy PRIVATE ${resource_cc})
endif()

# toy .......................................................................................................

GaiaAddExecutable(toy toy/main.cc)
target_link_libraries(toy PRIVATE Rocket::rocket)

# comm ......................................................................................................

GaiaAddLibrary(comm STATIC)
target_sources(comm PRIVATE comm/Message.cc)
target_link_libraries(comm PUBLIC Rocket::rocket)

GaiaAddExecutable(comm-local-client comm/comm-local-client.cc)
target_link_libraries(comm-local-client PRIVATE comm)

GaiaAddExecutable(comm-local-server comm/comm-local-server.cc)
target_link_libraries(comm-local-server PRIVATE comm)

GaiaAddExecutable(comm-tcp-client comm/comm-tcp-client.cc)
target_link_libraries(comm-tcp-client PRIVATE comm)

GaiaAddExecutable(comm-tcp-server comm/comm-tcp-server.cc)
target_link_libraries(comm-tcp-server PRIVATE comm)

# EOF
