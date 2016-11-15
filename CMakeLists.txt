cmake_minimum_required(VERSION 3.5.0)

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
  include(dcmqiBlockSetCMakeOSXVariables)
  mark_as_superbuild(
    VARS CMAKE_OSX_ARCHITECTURES:STRING CMAKE_OSX_SYSROOT:PATH CMAKE_OSX_DEPLOYMENT_TARGET:STRING
    ALL_PROJECTS
    )
endif()

project(DCMQI)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LC)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://www.slicer.org/wiki/Documentation/Nightly/Extensions/DCMQI")
set(EXTENSION_CATEGORY "DICOM")
set(EXTENSION_CONTRIBUTORS "Andrey Fedorov (BWH, SPL), Christian Herz (BWH, SPL), Jean-Christophe Fillion-Robin (Kitware)")
set(EXTENSION_DESCRIPTION "dcmqi (DICOM (dcm) for Quantitative Imaging (qi)) is a collection of libraries and command line tools with minimum dependencies to support standardized communication of quantitative image analysis research data using DICOM standard.")
set(EXTENSION_ICONURL "https://raw.githubusercontent.com/QIICR/dcmqi/master/dcmqi.png")
set(EXTENSION_SCREENSHOTURLS "https://raw.githubusercontent.com/QIICR/dcmqi/master/Design/DataFlow-001.png")
set(EXTENSION_BUILD_SUBDIRECTORY "dcmqi-build")

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
# Option default values

set(build_doc_default ON)
set(superbuild_default ON)
if(Slicer_SOURCE_DIR)
  # Default values used when DCMQI is integrated into Slicer as Remote project.
  set(build_doc_default OFF)
  set(superbuild_default OFF)
endif()

#-----------------------------------------------------------------------------
# Options

include(CMakeDependentOption)

option(DCMQI_BUILD_APPS "Build ${PROJECT_NAME} applications." ON)
mark_as_superbuild(DCMQI_BUILD_APPS)

cmake_dependent_option(
  DCMQI_BUILD_PARAMETRIC_MAPS "Build parametric maps for ${PROJECT_NAME}." ON
  "DCMQI_BUILD_APPS" OFF
  )
mark_as_superbuild(DCMQI_BUILD_PARAMETRIC_MAPS)

option(DCMQI_BUILD_DOC "Build ${PROJECT_NAME} documentation." ${build_doc_default})
mark_as_superbuild(DCMQI_BUILD_DOC)

#-----------------------------------------------------------------------------
# Standalone vs Slicer extension option
#

# This option should be named after the project name, it corresponds to the
# option set to ON when the project is build by the Slicer Extension build
# system.

set(_default OFF)
set(_reason "DCMQI_BUILD_SLICER_EXTENSION is ON")
if(NOT DEFINED DCMQI_BUILD_SLICER_EXTENSION AND DEFINED Slicer_DIR)
  set(_default ON)
  set(_reason "Slicer_DIR is SET")
endif()

option(DCMQI_BUILD_SLICER_EXTENSION "Build as a Slicer Extension" ${_default})

set(_msg "Checking if building as a Slicer extension")
message(STATUS ${_msg})
if(DCMQI_BUILD_SLICER_EXTENSION)
  message(STATUS "${_msg} - yes (${_reason})")
else()
  message(STATUS "${_msg} - no (DCMQI_BUILD_SLICER_EXTENSION is OFF)")
endif()

#-----------------------------------------------------------------------------
# Superbuild Option - Enabled by default
#
option(DCMQI_SUPERBUILD "Build ${PROJECT_NAME} and the projects it depends on." ${superbuild_default})
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
# Dependencies
#

# Slicer
if(DCMQI_BUILD_SLICER_EXTENSION)
  find_package(Slicer REQUIRED)
  include(${Slicer_USE_FILE})
  mark_as_superbuild(Slicer_DIR)
  # XXX Temporary workaround until issue is fixed in Slicer
  set(EXTENSION_SUPERBUILD_BINARY_DIR "${EXTENSION_SUPERBUILD_BINARY_DIR}/..")
endif()

# JsonCpp
set(_msg "Checking if using built-in JsonCpp")
message(STATUS ${_msg})
if(EXISTS "${JsonCpp_LIBRARY}" AND EXISTS "${JsonCpp_INCLUDE_DIR}")
  set(DCMQI_BUILTIN_JSONCPP 0)
  message(STATUS "${_msg} - no")
else()
  set(DCMQI_BUILTIN_JSONCPP 1)
  message(STATUS "${_msg} - yes")
endif()
mark_as_superbuild(JsonCpp_INCLUDE_DIR:PATH)
mark_as_superbuild(JsonCpp_LIBRARY:FILEPATH)
mark_as_superbuild(DCMQI_BUILTIN_JSONCPP:BOOL)

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

if(NOT EXISTS "${Slicer_DIR}")
  # XXX Do not attempt to find ITK if it was already included by Slicer.
  #     Doing so would remove the "ITKFactoryRegistration" include directory
  #     and library automatically appended to ITK_LIBRARIES and ITK_INCLUDE_DIRS
  find_package(ITK NO_MODULE REQUIRED)
  include(${ITK_USE_FILE})
endif()

#-----------------------------------------------------------------------------
# Testing
#
enable_testing()
set(TEMP_DIR ${CMAKE_BINARY_DIR}/Testing/Temporary)

#-----------------------------------------------------------------------------
# DCMQI library, applications and tests
#

add_subdirectory("libsrc")

if(DCMQI_BUILD_APPS)
  add_subdirectory("apps")
endif()

if(DCMQI_BUILD_DOC)
  add_subdirectory("doc")
endif()

#-----------------------------------------------------------------------------
# DCMQITargets
#

if(NOT Slicer_DIR)

  get_property(DCMQI_TARGETS GLOBAL PROPERTY DCMQI_TARGETS)
  export(TARGETS ${DCMQI_TARGETS} FILE ${CMAKE_BINARY_DIR}/DCMQITargets.cmake)

endif()

#-----------------------------------------------------------------------------
# DCMQIConfig
#

if(NOT Slicer_DIR)

  # Configure 'DCMQIConfig.cmake'
  include(CMakePackageConfigHelpers)

  # For a build tree
  set(CONFIG_DIR_CONFIG ${CMAKE_BINARY_DIR})
  set(DCMTK_DIR_CONFIG ${DCMTK_DIR})
  set(SlicerExecutionModel_DIR_CONFIG ${SlicerExecutionModel_DIR})
  set(${PROJECT_NAME}_CONFIG_CODE "####### Expanded from \@DCMQI_CONFIG_CODE\@ #######\n")
  set(${PROJECT_NAME}_CONFIG_CODE "find_package(DCMTK NO_MODULE REQUIRED)\n")
  set(${PROJECT_NAME}_CONFIG_CODE "${${PROJECT_NAME}_CONFIG_CODE}##################################################")
  set(_config ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake)
  configure_package_config_file(
      CMake/${PROJECT_NAME}Config.cmake.in
      ${_config}
      INSTALL_DESTINATION ${CMAKE_BINARY_DIR}
      PATH_VARS
        CONFIG_DIR_CONFIG
        DCMTK_DIR_CONFIG
        SlicerExecutionModel_DIR_CONFIG
      NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )

endif()

#-----------------------------------------------------------------------------
# Packaging
#

if(DEFINED Slicer_DIR)
  include(${Slicer_EXTENSION_CPACK})
endif()
