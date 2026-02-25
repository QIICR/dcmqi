#.rst:
# dcmqiVersionFromGit
# -------------------
#
# Extract DCMQI version components from the nearest git tag matching ``v*``.
#
# Sets the following variables in the including scope:
#
# ::
#
#   DCMQI_VERSION_MAJOR
#   DCMQI_VERSION_MINOR
#   DCMQI_VERSION_PATCH
#
# Falls back to ``0.0.0`` with a warning if not in a git repository
# or no matching tag is found.

find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --match "v*" --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE _git_tag
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE _git_result
  )
  if(_git_result EQUAL 0 AND _git_tag MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    set(DCMQI_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(DCMQI_VERSION_MINOR ${CMAKE_MATCH_2})
    set(DCMQI_VERSION_PATCH ${CMAKE_MATCH_3})
  endif()
endif()

if(NOT DEFINED DCMQI_VERSION_MAJOR)
  message(WARNING "Could not extract version from git tag, using fallback version 0.0.0")
  set(DCMQI_VERSION_MAJOR 0)
  set(DCMQI_VERSION_MINOR 0)
  set(DCMQI_VERSION_PATCH 0)
endif()
