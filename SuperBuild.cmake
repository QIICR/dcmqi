###########################################################################
#
#  Library:   DCMQI
#
#  Copyright (c) Kitware Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0.txt
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
###########################################################################

#-----------------------------------------------------------------------------
# DCMQI dependencies - Projects should be TOPOLOGICALLY ordered
#-----------------------------------------------------------------------------
set(DCMQI_DEPENDENCIES
  zlib
  DCMTK
  ITK
  )
if(DCMQI_BUILD_APPS)
  list(APPEND DCMQI_DEPENDENCIES
    SlicerExecutionModel
    )
endif()

#-----------------------------------------------------------------------------
# WARNING - No change should be required after this comment
#           when you are adding a new external project dependency.
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Make sure ${DCMQI_BINARY_DIR}/${PROJECT_NAME_LC}-build/bin exists
# May be used by some external project to install libs
if(NOT EXISTS ${DCMQI_BINARY_DIR}/${PROJECT_NAME_LC}-build/bin)
  file(MAKE_DIRECTORY ${DCMQI_BINARY_DIR}/${PROJECT_NAME_LC}-build/bin)
endif()

#-----------------------------------------------------------------------------
set(proj DCMQI)

ExternalProject_Include_Dependencies(DCMQI DEPENDS_VAR DCMQI_DEPENDENCIES)

set(DCMQI_EP_CMAKE_ARGS)
if(NOT DCMQI_BUILD_SLICER_EXTENSION)
  set(DCMQI_EP_CMAKE_ARGS CMAKE_ARGS
    -USlicer_DIR
    )
endif()

set(DCMQI_EP_CMAKE_CACHE_ARGS)
# XXX Workaround https://gitlab.kitware.com/cmake/cmake/issues/15448
#     and explicitly pass GIT_EXECUTABLE and Subversion_SVN_EXECUTABLE
foreach(varname IN ITEMS GIT_EXECUTABLE Subversion_SVN_EXECUTABLE)
  if(EXISTS "${${varname}}")
    list(APPEND DCMQI_EP_CMAKE_CACHE_ARGS -D${varname}:FILEPATH=${${varname}})
  endif()
endforeach()
list(APPEND DCMQI_EP_CMAKE_CACHE_ARGS -DCMAKE_CXX_STANDARD:STRING=${DCMQI_CMAKE_CXX_STANDARD})

ExternalProject_Add(${proj}
  ${${proj}_EP_ARGS}
  DOWNLOAD_COMMAND ""
  ${DCMQI_EP_CMAKE_ARGS}
  CMAKE_CACHE_ARGS
    -DDCMQI_SUPERBUILD:BOOL=OFF
    -DDCMQI_SUPERBUILD_BINARY_DIR:PATH=${DCMQI_BINARY_DIR}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_FLAGS_INIT:STRING=${CMAKE_CXX_FLAGS_INIT}
    -DCMAKE_C_FLAGS_INIT:STRING=${CMAKE_C_FLAGS_INIT}
    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
    -DEXTENSION_SUPERBUILD_BINARY_DIR:PATH=${${PROJECT_NAME}_BINARY_DIR}
    ${DCMQI_EP_CMAKE_CACHE_ARGS}
  SOURCE_DIR ${DCMQI_SOURCE_DIR}
  BINARY_DIR ${DCMQI_BINARY_DIR}/${PROJECT_NAME_LC}-build
  PREFIX ${PROJECT_NAME_LC}-prefix
  USES_TERMINAL_CONFIGURE 1
  USES_TERMINAL_BUILD 1
  INSTALL_COMMAND ""
  DEPENDS
    ${DCMQI_DEPENDENCIES}
  STEP_TARGETS configure
  )

# This custom external project step forces the build and later
# steps to run whenever a top level build is done...
#
# BUILD_ALWAYS flag is available in CMake 3.1 that allows force build
# of external projects without this workaround. Remove this workaround
# and use the CMake flag instead, when DCMQI's required minimum CMake
# version will be at least 3.1.
#
if(CMAKE_CONFIGURATION_TYPES)
  set(BUILD_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${proj}-prefix/src/${proj}-stamp/${CMAKE_CFG_INTDIR}/${proj}-build")
else()
  set(BUILD_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${proj}-prefix/src/${proj}-stamp/${proj}-build")
endif()
ExternalProject_Add_Step(${proj} forcebuild
  COMMAND ${CMAKE_COMMAND} -E remove ${BUILD_STAMP_FILE}
  COMMENT "Forcing build step for '${proj}'"
  DEPENDEES build
  ALWAYS 1
  )
