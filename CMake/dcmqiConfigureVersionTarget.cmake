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
# dcmqiConfigureVersionTarget
# ---------------------------
#
# Add a target named 'dcmqiConfigureVersion'.
#
# It has been designed to be included in the build system of DCMQI.
#
# The following variables are expected to be defined in the including scope:
#
#   GIT_EXECUTABLE
#   DCMQI_BINARY_DIR
#   DCMQI_CMAKE_DIR
#   DCMQI_LATEST_TAG
#   DCMQI_SOURCE_DIR
#   DCMQI_VERSION
#   PROJECT_NAME
#
# It also sets the following variable in the including scope:
#
#   DCMQI_CPACK_PROJECT_CONFIG_FILE
#

# --------------------------------------------------------------------------
# Parameters
# --------------------------------------------------------------------------

# VersionConfigure.h
set(_version_header_filepath_in "${DCMQI_CMAKE_DIR}/dcmqiVersionConfigure.h.in")
set(_version_header_relative_filepath "include/dcmqi/internal/VersionConfigure.h")
set(_version_header_filepath_out "${DCMQI_BINARY_DIR}/${_version_header_relative_filepath}")

# DCMQICPackOptions.cmake
set(_cpack_options_filename "DCMQICPackOptions.cmake")
set(_cpack_options_in "${DCMQI_CMAKE_DIR}/${_cpack_options_filename}.in")
set(DCMQI_CPACK_PROJECT_CONFIG_FILE "${DCMQI_BINARY_DIR}/${_cpack_options_filename}")

set(_version_target_name "dcmqiConfigureVersion")

find_package(Git REQUIRED)

# --------------------------------------------------------------------------
# Sanity checks
# --------------------------------------------------------------------------
set(expected_defined_vars
  GIT_EXECUTABLE
  ${PROJECT_NAME}_BINARY_DIR
  ${PROJECT_NAME}_CMAKE_DIR
  ${PROJECT_NAME}_SOURCE_DIR
  ${PROJECT_NAME}_LATEST_TAG
  ${PROJECT_NAME}_VERSION
  PROJECT_NAME
  )
foreach(var ${expected_defined_vars})
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is mandatory")
  endif()
endforeach()

if(NOT DEFINED ${PROJECT_NAME}_CONFIGURE_VERSION_TARGET)
  set(${PROJECT_NAME}_CONFIGURE_VERSION_TARGET 0)
endif()

# --------------------------------------------------------------------------
# Add configure version target
# --------------------------------------------------------------------------
if(NOT ${PROJECT_NAME}_CONFIGURE_VERSION_TARGET)
  set(script_args)
  foreach(var IN LISTS expected_defined_vars)
    list(APPEND script_args "-D${var}:STRING=${${var}}")
  endforeach()
  add_custom_target(${_version_target_name} ALL
    COMMAND ${CMAKE_COMMAND}
      ${script_args}
      -D${PROJECT_NAME}_CONFIGURE_VERSION_TARGET=1
      -P ${CMAKE_CURRENT_LIST_FILE}
    BYPRODUCTS
      ${_cpack_options_filename}
      ${_version_header_relative_filepath}
    COMMENT "Configuring ${_cpack_options_filename} and ${_version_header_relative_filepath}"
    )
  return()
endif()

# --------------------------------------------------------------------------
# Collect version information
# --------------------------------------------------------------------------

include(${DCMQI_CMAKE_DIR}/dcmqiVersion.cmake)

# --------------------------------------------------------------------------
# Sanity checks
# --------------------------------------------------------------------------

# Variables expected to be set by 'dcmqiVersion' module.
set(expected_defined_vars
  ${PROJECT_NAME}_VERSION_DATE
  ${PROJECT_NAME}_VERSION_QUALIFIER
  ${PROJECT_NAME}_WC_REVISION
  ${PROJECT_NAME}_WC_URL
  ${PROJECT_NAME}_WC_TAG
  )
foreach(var ${expected_defined_vars})
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is mandatory")
  endif()
endforeach()

# --------------------------------------------------------------------------
# Configure version header and packaging options
# --------------------------------------------------------------------------

set(DCMQI_VERSION_FULL "${DCMQI_VERSION}${DCMQI_VERSION_QUALIFIER}")

message(STATUS "Configuring DCMQI version [${DCMQI_VERSION_FULL}]")
message(STATUS "  DCMQI_WC_REVISION [${DCMQI_WC_REVISION}]")
message(STATUS "  DCMQI_WC_TAG [${DCMQI_WC_TAG}]")
message(STATUS "  DCMQI_WC_URL [${DCMQI_WC_URL}]")

configure_file(
  ${_version_header_filepath_in}
  ${_version_header_filepath_out}
  )

configure_file(
  ${_cpack_options_in}
  ${DCMQI_CPACK_PROJECT_CONFIG_FILE}
  @ONLY
  )

