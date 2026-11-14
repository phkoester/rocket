#
# apply-patch.cmake
#
# Applies a patch to the Git checkout in the current directory. Meant as the `PATCH_COMMAND` of
# `FetchContent_Declare`:
#
#   PATCH_COMMAND ${CMAKE_COMMAND} -DPATCH=<file> [-DPATCH_HASH=<hash>] -P <path>/apply-patch.cmake
#
# The tracked files of the checkout are restored first, so that the patch also applies when an older version
# of it is applied already, as happens when the patch changes. Local changes in the checkout are lost. Pass
# the hash of the patch as `PATCH_HASH` to make a changed patch change the command, which makes
# `FetchContent` run the command again; the script itself ignores it.
#

if(NOT DEFINED PATCH)
  message(FATAL_ERROR "`PATCH` is not set")
endif()

find_package(Git REQUIRED)

execute_process(
  COMMAND "${GIT_EXECUTABLE}" checkout -- .
  RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Cannot restore the checkout in `${CMAKE_CURRENT_SOURCE_DIR}`")
endif()

execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply "${PATCH}"
  RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Cannot apply patch `${PATCH}`")
endif()
message(STATUS "Applied patch `${PATCH}`")

# EOF
