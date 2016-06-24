cmake_minimum_required (VERSION 2.8.9)

foreach(p
  CMP0054 # CMake 3.1
  )
  if(POLICY ${p})
    cmake_policy(SET ${p} NEW)
  endif()
endforeach()

#-----------------------------------------------------------------------------
# Superbuild
#
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(EXTERNAL_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CMakeExternals)
set(EXTERNAL_PROJECT_FILE_PREFIX "")
include(ExternalProject)
include(ctkMacroCheckExternalProjectDependency)

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

#-----------------------------------------------------------------------------
# Library mode: SHARED (default) or STATIC
#
set(DCMQI_LIBRARY_MODE "SHARED")

option(DCMQI_BUILD_SHARED_LIBS "Build DCMQI libraries as shared module." ON)
mark_as_advanced(DCMQI_BUILD_SHARED_LIBS)
mark_as_superbuild(DCMQI_BUILD_SHARED_LIBS)
if(NOT DCMQI_BUILD_SHARED_LIBS)
  set(DCMQI_LIBRARY_MODE "STATIC")
endif()

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
message("Configuring with build type ${CMAKE_BUILD_TYPE}")
if(NOT CMAKE_CONFIGURATION_TYPES)
  mark_as_superbuild(VARS CMAKE_BUILD_TYPE ALL_PROJECTS)
endif()

mark_as_superbuild(
  VARS
    CMAKE_PREFIX_PATH:STRING
    CMAKE_DEBUG_POSTFIX:STRING
  ALL_PROJECTS
  )

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
        set(DCMQI_CMAKE_${type}_OUTPUT_DIRECTORY ${DCMQI_BINARY_DIR}/dcmqi-build/bin)
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
    if(NOT DEFINED DCMQI_PLUGIN_${type}_OUTPUT_DIRECTORY)
      set(DCMQI_PLUGIN_${type}_OUTPUT_DIRECTORY ${CMAKE_${type}_OUTPUT_DIRECTORY})
    endif()
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
# CMake function(s) and macro(s)
#
foreach(file
  CMake/ctkListToString.cmake
  CMake/ctkMacroParseArguments.cmake
  CMake/ctkMacroSetPaths.cmake
  CMake/ctkMacroListFilter.cmake
  CMake/ctkMacroOptionUtils.cmake
  CMake/ctkMacroBuildLib.cmake
  CMake/ctkMacroBuildLibWrapper.cmake
  CMake/ctkMacroBuildApp.cmake
  CMake/ctkMacroSetupQt.cmake
  CMake/ctkMacroTargetLibraries.cmake # Import multiple macros
  CMake/ctkFunctionExtractOptionNameAndValue.cmake
  CMake/ctkMacroValidateBuildOptions.cmake
  CMake/ctkMacroAddCtkLibraryOptions.cmake
  CMake/ctkFunctionGenerateDGraphInput.cmake
  CMake/ctkFunctionGeneratePluginManifest.cmake
  CMake/ctkMacroGeneratePluginResourceFile.cmake
  CMake/ctkFunctionAddPluginRepo.cmake
  CMake/ctkFunctionCheckCompilerFlags.cmake
  CMake/ctkFunctionCheckoutRepo.cmake
  CMake/ctkFunctionGetIncludeDirs.cmake
  CMake/ctkFunctionGetLibraryDirs.cmake
  CMake/ctkFunctionGetGccVersion.cmake
  CMake/ctkFunctionGetCompilerVisibilityFlags.cmake
  CMake/ctkFunctionCompileSnippets.cmake
  )
  include(${file})
  install(FILES ${file} DESTINATION ${DCMQI_INSTALL_CMAKE_DIR} COMPONENT Development)
endforeach()

#-----------------------------------------------------------------------------
# DCMQI dependencies - Projects should be TOPOLOGICALLY ordered
#-----------------------------------------------------------------------------
set(DCMQI_DEPENDENCIES
  zlib
  DCMTK
  ITK
  SlicerExecutionModel
  )

include(CMake/ctkBlockCheckDependencies.cmake)


#-----------------------------------------------------------------------------
# Superbuild script
#

if(DCMQI_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

find_package(DCMTK NO_MODULE REQUIRED)
find_package(SlicerExecutionModel NO_MODULE REQUIRED GenerateCLP)
find_package(ITK NO_MODULE REQUIRED)
include(${ITK_USE_FILE})

include(${SlicerExecutionModel_USE_FILE})

include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindGit.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/dcmqiMacroExtractRepositoryInfo.cmake)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

dcmqiMacroExtractRepositoryInfo(VAR_PREFIX dcmqi)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/dcmqiVersionConfigure.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/dcmqiVersionConfigure.h
)

enable_testing()
set(TEMP_DIR ${CMAKE_BINARY_DIR}/Testing/Temporary)

# jsoncpp
# https://github.com/open-source-parsers/jsoncpp/
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp)
set(JSONCPP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/jsoncpp.cpp)

# utilities
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/util/cliwrap.py
  ${DCMQI_INSTALL_BIN_DIR}
  COPYONLY
  )

add_subdirectory("apps")
