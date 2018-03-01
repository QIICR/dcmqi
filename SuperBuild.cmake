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
    -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
    -DEXTENSION_SUPERBUILD_BINARY_DIR:PATH=${${PROJECT_NAME}_BINARY_DIR}
    ${DCMQI_EP_CMAKE_CACHE_ARGS}
  SOURCE_DIR ${DCMQI_SOURCE_DIR}
  BINARY_DIR ${DCMQI_BINARY_DIR}/${PROJECT_NAME_LC}-build
  PREFIX ${PROJECT_NAME_LC}-prefix
  INSTALL_COMMAND ""
  DEPENDS
    ${DCMQI_DEPENDENCIES}
  STEP_TARGETS configure
  )

ExternalProject_AlwaysConfigure(${proj})
