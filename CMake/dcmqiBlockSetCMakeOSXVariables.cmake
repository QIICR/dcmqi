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

#
# dcmqiBlockSetCMakeOSXVariables
#

#
# Adapted from Paraview/Superbuild/CMakeLists.txt
#

# Note: Change architecture *before* any enable_language() or project()
#       calls so that it's set properly to detect 64-bit-ness...
#
if(APPLE)

  # Waiting universal binaries are supported and tested, complain if
  # multiple architectures are specified.
  if(NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
    list(LENGTH CMAKE_OSX_ARCHITECTURES arch_count)
    if(arch_count GREATER 1)
      message(FATAL_ERROR "error: Only one value (i386 or x86_64) should be associated with CMAKE_OSX_ARCHITECTURES.")
    endif()
  endif()

  # See CMake/Modules/Platform/Darwin.cmake)
  #   8.x == Mac OSX 10.4 (Tiger)
  #   9.x == Mac OSX 10.5 (Leopard)
  #  10.x == Mac OSX 10.6 (Snow Leopard)
  #  11.x == Mac OSX 10.7 (Lion)
  #  12.x == Mac OSX 10.8 (Mountain Lion)
  #  13.x == Mac OSX 10.9 (Mavericks)
  #  14.x == Mac OSX 10.10 (Yosemite)
  set(OSX_SDK_104_NAME "Tiger")
  set(OSX_SDK_105_NAME "Leopard")
  set(OSX_SDK_106_NAME "Snow Leopard")
  set(OSX_SDK_107_NAME "Lion")
  set(OSX_SDK_108_NAME "Mountain Lion")
  set(OSX_SDK_109_NAME "Mavericks")
  set(OSX_SDK_1010_NAME "Yosemite")

  set(OSX_SDK_ROOTS
    /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
    /Developer/SDKs
    )

  # Explicitly set the OSX_SYSROOT to the latest one, as its required
  #       when the OSX_DEPLOYMENT_TARGET is explicitly set
  foreach(SDK_ROOT ${OSX_SDK_ROOTS})
    if( "x${CMAKE_OSX_SYSROOT}x" STREQUAL "xx")
      file(GLOB SDK_SYSROOTS "${SDK_ROOT}/MacOSX*.sdk")

      if(NOT "x${SDK_SYSROOTS}x" STREQUAL "xx")
        set(SDK_SYSROOT_NEWEST "")
        set(SDK_VERSION "0")
        # find the latest SDK
        foreach(SDK_ROOT_I ${SDK_SYSROOTS})
          # extract version from SDK
          string(REGEX MATCH "MacOSX([0-9]+\\.[0-9]+)\\.sdk" _match "${SDK_ROOT_I}")
          if("${CMAKE_MATCH_1}" VERSION_GREATER "${SDK_VERSION}")
            set(SDK_SYSROOT_NEWEST "${SDK_ROOT_I}")
            set(SDK_VERSION "${CMAKE_MATCH_1}")
          endif()
        endforeach()

        if(NOT "x${SDK_SYSROOT_NEWEST}x" STREQUAL "xx")
          string(REPLACE "." "" sdk_version_no_dot ${SDK_VERSION})
          set(OSX_NAME ${OSX_SDK_${sdk_version_no_dot}_NAME})
          set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Force build for 64-bit ${OSX_NAME}." FORCE)
          set(CMAKE_OSX_DEPLOYMENT_TARGET "${SDK_VERSION}" CACHE STRING "Force build for 64-bit ${OSX_NAME}." FORCE)
          set(CMAKE_OSX_SYSROOT "${SDK_SYSROOT_NEWEST}" CACHE PATH "Force build for 64-bit ${OSX_NAME}." FORCE)
          message(STATUS "Setting OSX_ARCHITECTURES to '${CMAKE_OSX_ARCHITECTURES}' as none was specified.")
          message(STATUS "Setting OSX_DEPLOYMENT_TARGET to '${CMAKE_OSX_DEPLOYMENT_TARGET}' as none was specified.")
          message(STATUS "Setting OSX_SYSROOT to latest '${CMAKE_OSX_SYSROOT}' as none was specified.")
        endif()
      endif()
    endif()
  endforeach()

  if(NOT "${CMAKE_OSX_SYSROOT}" STREQUAL "")
    if(NOT EXISTS "${CMAKE_OSX_SYSROOT}")
      message(FATAL_ERROR "error: CMAKE_OSX_SYSROOT='${CMAKE_OSX_SYSROOT}' does not exist")
    endif()
  endif()
endif()
