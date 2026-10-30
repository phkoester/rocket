#
# RocketFunctions.cmake
#
# Helper functions for Rocket.
#

# Private functions -----------------------------------------------------------------------------------------

# RocketParseArgs__(outSrcFiles outEnv srcFile... [ENVIRONMENT name=value]...)
function(RocketParseArgs__ outSrcFiles outEnv)
  set(srcFiles)
  set(env)
  set(appendTo srcFiles)
  foreach(it IN LISTS ARGN)
    if(it STREQUAL "ENVIRONMENT")
      set(appendTo env)
    else()
      list(APPEND ${appendTo} ${it})
    endif()
  endforeach()

  set(${outSrcFiles} ${srcFiles} PARENT_SCOPE)
  set(${outEnv} ${env} PARENT_SCOPE)
endfunction()

# Functions -------------------------------------------------------------------------------------------------

# RocketAddBench(name dir srcFile... [ENVIRONMENT name=value...])
function(RocketAddBench name dir)
  RocketParseArgs__(srcFiles env ${ARGN})

  list(TRANSFORM srcFiles PREPEND "${dir}/")
  GaiaAddExecutable(${name} ${srcFiles})
  target_link_libraries(${name} PRIVATE benchmark::benchmark_main Rocket::rocket-bench)

  add_test(
    NAME    ${name}
    COMMAND ${name}
  )

  string(JOIN " " configs ${CMAKE_CONFIGURATION_TYPES})

  set_property(TEST ${name}
    PROPERTY ENVIRONMENT
      "BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
      "CONFIG=$<CONFIG>"
      "CONFIGS=${configs}"
      "SOURCE_DIR=${CMAKE_SOURCE_DIR}/src/test/${dir}"
      ${env}
  )
endfunction()

# RocketAddTest(name dir srcFile... [ENVIRONMENT name=value...])
function(RocketAddTest name dir)
  RocketParseArgs__(srcFiles env ${ARGN})

  list(TRANSFORM srcFiles PREPEND "${dir}/")
  GaiaAddExecutable(${name} ${srcFiles})
  target_link_libraries(${name} PRIVATE Rocket::rocket-test-main Rocket::rocket-test)

  string(JOIN " " configs ${CMAKE_CONFIGURATION_TYPES})

  set(envProps)
  foreach(it IN LISTS env)
    list(APPEND envProps PROPERTIES ENVIRONMENT "${it}")
  endforeach()

  gtest_discover_tests(${name}
    DISCOVERY_MODE PRE_TEST
    EXTRA_ARGS --gtest_catch_exceptions=0
    PROPERTIES ENVIRONMENT "BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    PROPERTIES ENVIRONMENT "CONFIG=$<CONFIG>"
    PROPERTIES ENVIRONMENT "CONFIGS=${configs}"
    PROPERTIES ENVIRONMENT "SOURCE_DIR=${CMAKE_SOURCE_DIR}/src/test/${dir}"
    ${envProps}
  )
endfunction()

# EOF
