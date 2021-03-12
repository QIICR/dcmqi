cmake_minimum_required(VERSION 3.5.0)

#-----------------------------------------------------------------------------
# C++ standard
#
set(_msg "Setting C++ standard")
message(STATUS "${_msg}")
if(DEFINED Slicer_DIR)
  set(CMAKE_CXX_STANDARD 11)
endif()
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 98)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "${_msg} - C++${CMAKE_CXX_STANDARD}")

set(CMAKE_POSITION_INDEPENDENT_CODE 1)

set(DCMQI_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

#-----------------------------------------------------------------------------
# Superbuild
#
set(CMAKE_MODULE_PATH "${DCMQI_CMAKE_DIR}" ${CMAKE_MODULE_PATH})
set(SUPERBUILD_TOPLEVEL_PROJECT DCMQI)
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
# Testing
#
include(CTest)
mark_as_superbuild(BUILD_TESTING)

set(TEMP_DIR ${CMAKE_BINARY_DIR}/Testing/Temporary)
mark_as_superbuild(TEMP_DIR:PATH)

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
# DCMQI version number
#
set(DCMQI_VERSION_MAJOR 1)
set(DCMQI_VERSION_MINOR 2)
set(DCMQI_VERSION_PATCH 4)
set(DCMQI_VERSION
    "${DCMQI_VERSION_MAJOR}.${DCMQI_VERSION_MINOR}.${DCMQI_VERSION_PATCH}")

set(DCMQI_LATEST_TAG "latest")

# Append the library version information to the library target
# properties.  A parent project may set its own properties and/or may
# block this.
if(NOT DCMQI_NO_LIBRARY_VERSION)
  set(DCMQI_LIBRARY_PROPERTIES ${DCMQI_LIBRARY_PROPERTIES}
    VERSION "${DCMQI_VERSION}"
    SOVERSION "${DCMQI_VERSION_MAJOR}.${DCMQI_VERSION_MINOR}"
    )
endif()

#-----------------------------------------------------------------------------
# Install directories, used for install rules.
#
if(NOT DCMQI_INSTALL_BIN_DIR)
  set(DCMQI_INSTALL_BIN_DIR "bin")
endif()
if(NOT DCMQI_INSTALL_LIB_DIR)
  set(DCMQI_INSTALL_LIB_DIR "lib/dcmqi-${DCMQI_VERSION_MAJOR}.${DCMQI_VERSION_MINOR}")
endif()
if(NOT DCMQI_INSTALL_CMAKE_DIR)
  set(DCMQI_INSTALL_CMAKE_DIR "lib/dcmqi-${DCMQI_VERSION_MAJOR}.${DCMQI_VERSION_MINOR}/CMake")
endif()
if(NOT DCMQI_INSTALL_INCLUDE_DIR)
  set(DCMQI_INSTALL_INCLUDE_DIR "include/dcmqi-${DCMQI_VERSION_MAJOR}.${DCMQI_VERSION_MINOR}")
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
# Other options

# macOS rpath
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 1)
endif()
mark_as_superbuild(
  VARS CMAKE_MACOSX_RPATH:BOOL
  ALL_PROJECTS
  )

#-----------------------------------------------------------------------------
# Dependencies
#

# Slicer
if(DCMQI_BUILD_SLICER_EXTENSION)
  find_package(Slicer REQUIRED)
  include(${Slicer_USE_FILE})
  mark_as_superbuild(Slicer_DIR)
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

# Python 2.7
if(BUILD_TESTING)
  find_package(PythonInterp 2.7 REQUIRED)
  mark_as_superbuild(PYTHON_EXECUTABLE)
endif()

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

set(export_targets TRUE)
if(DCMQI_SUPERBUILD_BINARY_DIR)
  string(FIND ${ITK_DIR}   "${DCMQI_SUPERBUILD_BINARY_DIR}" itk_here)
  string(FIND ${DCMTK_DIR} "${DCMQI_SUPERBUILD_BINARY_DIR}" dcmtk_here)

  if(NOT ${itk_here} EQUAL -1 OR NOT ${dcmtk_here} EQUAL -1)
    set(export_targets FALSE)
  endif()
endif()

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
# Version information
#
# Add target extracting version information. The target configures
# 'DCMQICPackOptions.cmake' and 'dcmqiVersionConfigure.h'
#
include(dcmqiConfigureVersionTarget)
add_dependencies(dcmqi dcmqiConfigureVersion)

#-----------------------------------------------------------------------------
# DCMQIConfig
#

if(NOT Slicer_DIR)

  # Configure 'DCMQIConfig.cmake'
  include(CMakePackageConfigHelpers)

  set(path_vars
    CONFIG_DIR_CONFIG
    DCMTK_DIR_CONFIG
    )
  if(DCMQI_BUILD_APPS)
    list(APPEND path_vars
      SlicerExecutionModel_DIR_CONFIG
      )
  endif()

  # For a build tree
  set(CONFIG_DIR_CONFIG ${CMAKE_BINARY_DIR})
  set(DCMTK_DIR_CONFIG ${DCMTK_DIR})
  set(SlicerExecutionModel_DIR_CONFIG ${SlicerExecutionModel_DIR})
  set(${PROJECT_NAME}_CONFIG_CODE "####### Expanded from \@DCMQI_CONFIG_CODE\@ #######\n")
  set(${PROJECT_NAME}_CONFIG_CODE "find_package(DCMTK NO_MODULE REQUIRED)\n")
  set(${PROJECT_NAME}_CONFIG_CODE "set(DCMQI_BUILD_APPS ${DCMQI_BUILD_APPS})\n")
  set(${PROJECT_NAME}_CONFIG_CODE "${${PROJECT_NAME}_CONFIG_CODE}##################################################")
  set(_config ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake)
  configure_package_config_file(
      CMake/${PROJECT_NAME}Config.cmake.in
      ${_config}
      INSTALL_DESTINATION ${CMAKE_BINARY_DIR}
      PATH_VARS
        ${path_vars}
      NO_CHECK_REQUIRED_COMPONENTS_MACRO
      )
  set(CONFIG_DIR_CONFIG cmake)
  set(_install_config ${CMAKE_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake)
  configure_package_config_file(
      CMake/${PROJECT_NAME}Config.cmake.in
      ${_install_config}
      INSTALL_DESTINATION ${CMAKE_BINARY_DIR}
      PATH_VARS
        ${path_vars}
      NO_CHECK_REQUIRED_COMPONENTS_MACRO
      )
  write_basic_package_version_file(
      ${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
      VERSION ${DCMQI_VERSION}
      COMPATIBILITY ExactVersion
      )

  if(export_targets)
    install(EXPORT ${PROJECT_NAME}Targets
            DESTINATION cmake
            )
    install(FILES ${_install_config}
            DESTINATION cmake
            )
    install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
            DESTINATION cmake
            )
    install(EXPORT ${PROJECT_NAME}Targets
            FILE ${PROJECT_NAME}Targets.cmake
            DESTINATION cmake
            )
    install(DIRECTORY ${PROJECT_BINARY_DIR}/include DESTINATION .)
  endif()
endif()

#-----------------------------------------------------------------------------
# Packaging
#

set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${PROJECT_NAME};RuntimeLibraries;/")

if(DEFINED Slicer_DIR)
  include(${Slicer_EXTENSION_GENERATE_CONFIG})
  include(${Slicer_EXTENSION_CPACK})
else()
  if(EXISTS "${DCMTK_DIR}/cmake_install.cmake")
    set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${DCMTK_DIR};DCMTK;shlib;/")
  endif()
  if(EXISTS "${ITK_DIR}/cmake_install.cmake")
    set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${ITK_DIR};ITK;RuntimeLibraries;/")
  endif()
  set(CPACK_PACKAGE_NAME "dcmqi")
  set(CPACK_PACKAGE_VENDOR "Quantitative Image Informatics for Cancer Research (QIICR)")
  set(CPACK_PACKAGE_VERSION_MAJOR ${DCMQI_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${DCMQI_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${DCMQI_VERSION_PATCH})
  if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CPACK_SYSTEM_NAME "mac")
  elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CPACK_SYSTEM_NAME "linux")
  elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(CPACK_SYSTEM_NAME win64)
    else()
      set(CPACK_SYSTEM_NAME win32)
    endif()
  endif()
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${DCMQI_VERSION}-${CPACK_SYSTEM_NAME}")
  set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${EXTENSION_DESCRIPTION})
  if(UNIX)
    set(CPACK_GENERATOR "TGZ")
  elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")
  endif()
  if(NOT DEFINED DCMQI_CPACK_PROJECT_CONFIG_FILE)
    message(FATAL_ERROR "DCMQI_CPACK_PROJECT_CONFIG_FILE is not defined")
  endif()
  set(CPACK_PROJECT_CONFIG_FILE "${DCMQI_CPACK_PROJECT_CONFIG_FILE}")
  include(CPack)
endif()
