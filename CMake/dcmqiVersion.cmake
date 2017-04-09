
#.rst:
# dcmqiVersion
# ------------
#
# This module will set these variables in the including scope:
#
# ::
#
#   DCMQI_VERSION_IS_RELEASE
#   DCMQI_VERSION_QUALIFIER
#
# It will also set all variables describing the SCM associated
# with DCMQI_SOURCE_DIR.
#
# It has been designed to be included in the build system of DCMQI.
#
# The following variables are expected to be defined in the including
# scope:
#
# ::
#
#   GIT_EXECUTABLE
#   DCMQI_CMAKE_DIR
#   DCMQI_LATEST_TAG
#   DCMQI_SOURCE_DIR
#
#
# Details description:
#
# .. variable:: DCMQI_VERSION_IS_RELEASE
#
#   Indicate if the current build is release. A particular build is a
#   release if the HEAD of the associated Git checkout corresponds to
#   a tag (either lightweight or annotated).
#
# .. variable:: DCMQI_CMAKE_DIR
#
#   Directory containing all CMake modules included in this module.
#
# .. variable:: DCMQI_LATEST_TAG
#
#   Name of the latest tag. It is used to reference the "bleeding edge"
#   version on GitHub. This lightweight tag is automatically updated
#   by a script running on CI services like Appveyor, CircleCI or TravisCI.
#
# .. variable:: DCMQI_SOURCE_DIR
#
#   DCMQI Git checkout
#
#
# Dependent CMake modules:
#
# ::
#
#   dcmqiMacroExtractRepositoryInfo
#   FindGit.cmake
#

# --------------------------------------------------------------------------
# Sanity checks
# --------------------------------------------------------------------------
set(expected_defined_vars
  GIT_EXECUTABLE
  DCMQI_CMAKE_DIR
  DCMQI_LATEST_TAG
  DCMQI_SOURCE_DIR
  DCMQI_VERSION
  )
foreach(var ${expected_defined_vars})
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is mandatory")
  endif()
endforeach()

#-----------------------------------------------------------------------------
# Update CMake module path
#-----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH
  ${DCMQI_CMAKE_DIR}
  ${CMAKE_MODULE_PATH}
  )

find_package(Git REQUIRED)

#-----------------------------------------------------------------------------
# DCMQI version number
#-----------------------------------------------------------------------------

set(msg "Checking if building a release")
message(STATUS "${msg}")

include(dcmqiFunctionExtractGitTagNames)
dcmqiFunctionExtractGitTagNames(
  REF "HEAD"
  SOURCE_DIR ${DCMQI_SOURCE_DIR}
  OUTPUT_VARIABLE TAG_NAMES
  )

# If at least one tag (different from "${DCMQI_LATEST_TAG}") is
# associated with HEAD, we are building a release
list(LENGTH TAG_NAMES tag_count)
list(FIND TAG_NAMES "${DCMQI_LATEST_TAG}" latest_index)
if(
    (tag_count GREATER 0 AND latest_index EQUAL -1) OR
    (tag_count GREATER 1 AND NOT latest_index EQUAL -1)
  )
  set(is_release 1)
  set(is_release_answer "yes (found tags ${TAG_NAMES})")
else()
  set(is_release 0)
  if(NOT latest_index EQUAL -1)
    set(is_release_answer "no (found only '${DCMQI_LATEST_TAG}' tag)")
  else()
    set(is_release_answer "no (found 0 tags)")
  endif()
endif()

message(STATUS "${msg} - ${is_release_answer}")

GIT_WC_INFO(${DCMQI_SOURCE_DIR} DCMQI)

string(REGEX MATCH ".*([0-9][0-9][0-9][0-9])\\-([0-9][0-9])\\-([0-9][0-9]).*" _out
  "${DCMQI_WC_LAST_CHANGED_DATE}")
set(DCMQI_VERSION_DATE "${CMAKE_MATCH_1}${CMAKE_MATCH_2}${CMAKE_MATCH_3}") # YYYYMMDD

if(NOT is_release)
  set(_version_qualifier "-${DCMQI_VERSION_DATE}-${DCMQI_WC_REVISION}")
endif()

set(DCMQI_VERSION_QUALIFIER "${_version_qualifier}")
set(DCMQI_VERSION_IS_RELEASE ${is_release})

