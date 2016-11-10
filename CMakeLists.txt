cmake_minimum_required (VERSION 3.5.0)

#-----------------------------------------------------------------------------
# Superbuild
#
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(EXTERNAL_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CMakeExternals)
set(EXTERNAL_PROJECT_FILE_PREFIX "")
include(ExternalProject)
include(ExternalProjectDependency)

#-----------------------------------------------------------------------------
if(APPLE)
  # Note: By setting CMAKE_OSX_* variables before any enable_language() or project() calls,
  #       we ensure that the bitness will be properly detected.
  include(ctkBlockSetCMakeOSXVariables)
  mark_as_superbuild(
    VARS CMAKE_OSX_ARCHITECTURES:STRING CMAKE_OSX_SYSROOT:PATH CMAKE_OSX_DEPLOYMENT_TARGET:STRING
    ALL_PROJECTS
    )
endif()

project(DCMQI)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LC)

#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Set a default build type if none was specified
#
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  mark_as_advanced(CMAKE_BUILD_TYPE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
if(NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Configuring with build type '${CMAKE_BUILD_TYPE}'")
  mark_as_superbuild(VARS CMAKE_BUILD_TYPE ALL_PROJECTS)
endif()

mark_as_superbuild(
  VARS
    CMAKE_PREFIX_PATH:STRING
    CMAKE_DEBUG_POSTFIX:STRING
  ALL_PROJECTS
  )

#-----------------------------------------------------------------------------
# Options

include(CMakeDependentOption)

option(BUILD_APPS "Build ${PROJECT_NAME} applications." ON)
mark_as_advanced(BUILD_APPS)
mark_as_superbuild(BUILD_APPS)

cmake_dependent_option(
  BUILD_PARAMETRIC_MAPS "Build parametric maps for ${PROJECT_NAME}." OFF
  "BUILD_APPS" OFF
  )
mark_as_advanced(BUILD_PARAMETRIC_MAPS)
mark_as_superbuild(BUILD_PARAMETRIC_MAPS)

option(BUILD_DOC "Build ${PROJECT_NAME} documentation." ON)
mark_as_advanced(BUILD_DOC)
mark_as_superbuild(BUILD_DOC)

#-----------------------------------------------------------------------------
# Superbuild Option - Enabled by default
#
option(DCMQI_SUPERBUILD "Build ${PROJECT_NAME} and the projects it depends on." ON)
mark_as_advanced(DCMQI_SUPERBUILD)

#-----------------------------------------------------------------------------
# Output directories.
#
foreach(type LIBRARY RUNTIME ARCHIVE)
  # Make sure the directory exists
  if(DEFINED DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY
     AND NOT EXISTS ${DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY})
    message(FATAL_ERROR "DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY is set to a non-existing directory [${DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY}]")
  endif()

  if(DCMQI_SUPERBUILD)
    set(output_dir ${DCMQI_BINARY_DIR}/bin)
    if(NOT DEFINED DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY)
      set(DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY ${DCMQI_BINARY_DIR}/${PROJECT_NAME_LC}-build/bin)
    endif()
    mark_as_superbuild(DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY:PATH)
  else()
    if(NOT DEFINED DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY)
      set(output_dir ${DCMQI_BINARY_DIR}/bin)
    else()
      set(output_dir ${DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY})
    endif()
  endif()
  set(CMAKE_${type}_OUTPUT_DIRECTORY ${output_dir} CACHE INTERNAL "Single output directory for building all libraries.")
endforeach()

#-----------------------------------------------------------------------------
# DCMQI version number.  An even minor number corresponds to releases.
#
set(DCMQI_MAJOR_VERSION 0)
set(DCMQI_MINOR_VERSION 0)
set(DCMQI_PATCH_VERSION 1)
set(DCMQI_VERSION
    "${DCMQI_MAJOR_VERSION}.${DCMQI_MINOR_VERSION}.${DCMQI_PATCH_VERSION}")

# Append the library version information to the library target
# properties.  A parent project may set its own properties and/or may
# block this.
if(NOT DCMQI_NO_LIBRARY_VERSION)
  set(DCMQI_LIBRARY_PROPERTIES ${DCMQI_LIBRARY_PROPERTIES}
    VERSION "${DCMQI_VERSION}"
    SOVERSION "${DCMQI_MAJOR_VERSION}.${DCMQI_MINOR_VERSION}"
    )
endif()


#-----------------------------------------------------------------------------
# Install directories, used for install rules.
#
if(NOT DCMQI_INSTALL_BIN_DIR)
  set(DCMQI_INSTALL_BIN_DIR "bin")
endif()
if(NOT DCMQI_INSTALL_LIB_DIR)
  set(DCMQI_INSTALL_LIB_DIR "lib/dcmqi-${DCMQI_MAJOR_VERSION}.${DCMQI_MINOR_VERSION}")
endif()
if(NOT DCMQI_INSTALL_CMAKE_DIR)
  set(DCMQI_INSTALL_CMAKE_DIR "lib/dcmqi-${DCMQI_MAJOR_VERSION}.${DCMQI_MINOR_VERSION}/CMake")
endif()
if(NOT DCMQI_INSTALL_INCLUDE_DIR)
  set(DCMQI_INSTALL_INCLUDE_DIR "include/dcmqi-${DCMQI_MAJOR_VERSION}.${DCMQI_MINOR_VERSION}")
endif()
if(NOT DCMQI_INSTALL_DOC_DIR)
  set(DCMQI_INSTALL_DOC_DIR "doc")
endif()

mark_as_superbuild(
  DCMQI_INSTALL_BIN_DIR:STRING
  DCMQI_INSTALL_LIB_DIR:STRING
  DCMQI_INSTALL_CMAKE_DIR:STRING
  DCMQI_INSTALL_INCLUDE_DIR:STRING
  DCMQI_INSTALL_DOC_DIR:STRING
  )

#-----------------------------------------------------------------------------
# Additional CXX/C Flags
#
set(CMAKE_C_FLAGS "-fPIC")
set(ADDITIONAL_C_FLAGS "" CACHE STRING "Additional C Flags")
mark_as_advanced(ADDITIONAL_C_FLAGS)
mark_as_superbuild(ADDITIONAL_C_FLAGS)

set(ADDITIONAL_CXX_FLAGS "" CACHE STRING "Additional CXX Flags")
mark_as_advanced(ADDITIONAL_CXX_FLAGS)
mark_as_superbuild(ADDITIONAL_CXX_FLAGS)

#-----------------------------------------------------------------------------
# Other options

# Git protocol option
option(DCMQI_USE_GIT_PROTOCOL "If behind a firewall turn this OFF to use http instead." ON)
mark_as_advanced(DCMQI_USE_GIT_PROTOCOL)
set(git_protocol "git")
if(NOT DCMQI_USE_GIT_PROTOCOL)
  set(git_protocol "http")
endif()

#-----------------------------------------------------------------------------
#

#-----------------------------------------------------------------------------
# Superbuild script
#

if(DCMQI_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

#-----------------------------------------------------------------------------
# Dependencies
#

find_package(DCMTK NO_MODULE REQUIRED)
find_package(ITK NO_MODULE REQUIRED)
include(${ITK_USE_FILE})


#-----------------------------------------------------------------------------
# Testing
#
enable_testing()
set(TEMP_DIR ${CMAKE_BINARY_DIR}/Testing/Temporary)

#-----------------------------------------------------------------------------
# DCMQI library, applications and tests
#

add_subdirectory("libsrc")

if(BUILD_APPS)
  add_subdirectory("apps")
endif()

if(BUILD_DOC)
  add_subdirectory("doc")
endif()
