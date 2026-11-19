# CMakeLists.txt

# Executables -----------------------------------------------------------------------------------------------

# guy .......................................................................................................

if(ROCKET_GUI)
  gaiaAddExecutable(guy guy/main.cc)
  target_link_libraries(guy PRIVATE Rocket::rocket-gui)
  if (GAIA_OS_WINDOWS)
    target_link_options(guy PRIVATE /subsystem:windows)
  endif()

  set(xrc_files ${CMAKE_CURRENT_SOURCE_DIR}/guy/resource.xrc)
  # The output depends on the configuration through `wxrc`. With a multi-config generator, a shared output
  # would be regenerated on every switch of the configuration.
  get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(isMultiConfig)
    set(resource_cc ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resource.cc)
  else()
    set(resource_cc ${CMAKE_CURRENT_BINARY_DIR}/resource.cc)
  endif()
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

gaiaAddExecutable(toy toy/main.cc)
target_link_libraries(toy PRIVATE Rocket::rocket)

# comm ......................................................................................................

gaiaAddLibrary(comm STATIC)
target_sources(comm PRIVATE comm/Message.cc)
target_link_libraries(comm PUBLIC Rocket::rocket)

gaiaAddExecutable(comm-local-client comm/comm-local-client.cc)
target_link_libraries(comm-local-client PRIVATE comm)

gaiaAddExecutable(comm-local-server comm/comm-local-server.cc)
target_link_libraries(comm-local-server PRIVATE comm)

gaiaAddExecutable(comm-tcp-client comm/comm-tcp-client.cc)
target_link_libraries(comm-tcp-client PRIVATE comm)

gaiaAddExecutable(comm-tcp-server comm/comm-tcp-server.cc)
target_link_libraries(comm-tcp-server PRIVATE comm)

# EOF
