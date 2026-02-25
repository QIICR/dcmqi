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
# Version is determined by (in order of priority):
#
# 1. CMake cache variables (``-DDCMQI_VERSION_MAJOR=...`` etc.)
# 2. Git tag (nearest ``v*`` tag)
# 3. ``VERSION`` file in the source root
#

# Allow explicit override via cache variables
if(DEFINED DCMQI_VERSION_MAJOR AND DEFINED DCMQI_VERSION_MINOR AND DEFINED DCMQI_VERSION_PATCH)
  return()
endif()

# Try extracting from git tag
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
    return()
  endif()
endif()

# Fall back to VERSION file (for source tarballs without .git)
set(_version_file "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
if(EXISTS "${_version_file}")
  file(READ "${_version_file}" _version_contents)
  string(STRIP "${_version_contents}" _version_contents)
  if(_version_contents MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    set(DCMQI_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(DCMQI_VERSION_MINOR ${CMAKE_MATCH_2})
    set(DCMQI_VERSION_PATCH ${CMAKE_MATCH_3})
    return()
  endif()
endif()

message(FATAL_ERROR
  "Could not determine DCMQI version. Provide version via:\n"
  "  - git tag (v1.2.3)\n"
  "  - VERSION file in source root\n"
  "  - cmake -DDCMQI_VERSION_MAJOR=1 -DDCMQI_VERSION_MINOR=2 -DDCMQI_VERSION_PATCH=3"
)
