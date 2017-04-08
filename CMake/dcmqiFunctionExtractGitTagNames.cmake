################################################################################
#
#  Program: 3D Slicer
#
#  Copyright (c) Kitware Inc.
#
#  See COPYRIGHT.txt
#  or http://www.slicer.org/copyright/copyright.txt for details.
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
#  and was partially funded by NIH grant 3P41RR013218-12S1
#
################################################################################

#.rst:
# dcmqiFunctionExtractGitTagNames
# -------------------------------
#
# dcmqiFunctionExtractGitTagNames(
#   OUTPUT_VARIABLE <varname>
#   GIT_REF <ref>
#   [SOURCE_DIR <dir>]
#   )
#
# If any, this function set the variable <varname> with the list of tag
# names associated with <ref>.
#
# If <ref> is not provided, it defaults to "HEAD"
#
# If <dir> is not provided, it defaults to the value of CMAKE_CURRENT_SOURCE_DIR
#
function(dcmqiFunctionExtractGitTagNames)
  include(CMakeParseArguments)
  set(options)
  set(oneValueArgs OUTPUT_VARIABLE REF SOURCE_DIR)
  set(multiValueArgs)
  cmake_parse_arguments(MY "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Defaults
  if(NOT DEFINED MY_REF)
    set(MY_REF "HEAD")
  endif()
  if(NOT DEFINED MY_SOURCE_DIR)
    set(MY_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  # Sanity checks
  set(expected_defined_vars
    REF
    OUTPUT_VARIABLE
    SOURCE_DIR
    )
  foreach(var ${expected_defined_vars})
    if(NOT DEFINED MY_${var})
      message(FATAL_ERROR "Parameter ${var} is mandatory")
    endif()
  endforeach()

  #
  # Note: Since git does *NOT* provide a direct way to get
  #       these, we first get the <sha> of one of the tag associated
  #       with <ref>, then we group all the tags by SHA and return the
  #       list of tags associated with <sha>.
  #

  set(verbose 0) # For debugging

  # Is ref associated with at least one tag ?
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --exact-match ${MY_REF}
    OUTPUT_VARIABLE output
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    WORKING_DIRECTORY ${MY_SOURCE_DIR}
    )
  if(verbose)
    message(STATUS "output [${output}]")
  endif()
  if("${output}" STREQUAL "")
    set(${MY_OUTPUT_VARIABLE} "" PARENT_SCOPE)
    return()
  endif()

  # Get tag's sha
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-list -n 1 "${output}"
    OUTPUT_VARIABLE tag_sha
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY ${MY_SOURCE_DIR}
    )
  if(verbose)
    message(STATUS "tag_sha [${tag_sha}]")
  endif()

  # Get all tags
  execute_process(
    COMMAND ${GIT_EXECUTABLE} tag --list
    OUTPUT_VARIABLE tags
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY ${MY_SOURCE_DIR}
    )

  # Convert space separated string into a list
  string(REPLACE "\n" ";" tags ${tags})
  if(verbose)
    message(STATUS "tags: ${tags}")
  endif()

  # Get sha of each tag
  foreach(tag IN LISTS tags)
    execute_process(
      COMMAND ${GIT_EXECUTABLE}  rev-list -n 1 "refs/tags/${tag}"
      OUTPUT_VARIABLE sha 
      OUTPUT_STRIP_TRAILING_WHITESPACE
      WORKING_DIRECTORY ${MY_SOURCE_DIR}
      )
    if(verbose)
      message(STATUS "tag[${tag}] -> sha[${sha}]")
    endif()
    if(NOT DEFINED all_tags_${sha})
      set(all_tags_${sha} ${tag})
    else()
      list(APPEND all_tags_${sha} ${tag})
    endif()
  endforeach()

  set(${MY_OUTPUT_VARIABLE} ${all_tags_${tag_sha}} PARENT_SCOPE)
endfunction()

