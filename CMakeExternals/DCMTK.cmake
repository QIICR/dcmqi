#
# DCMTK
#

set(proj DCMTK)

set(${proj}_DEPENDENCIES "")

ExternalProject_Include_Dependencies(${proj}
  PROJECT_VAR proj
  DEPENDS_VAR ${proj}_DEPENDENCIES
  EP_ARGS_VAR ${proj}_EXTERNAL_PROJECT_ARGS
  USE_SYSTEM_VAR ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj}
  )

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  unset(DCMTK_DIR CACHE)
  find_package(DCMTK REQUIRED)
endif()

# Sanity checks
if(DEFINED DCMTK_DIR AND NOT EXISTS ${DCMTK_DIR})
  message("DCMTK_DIR is ${DCMTK_DIR}")
  message(FATAL_ERROR "DCMTK_DIR variable is defined but corresponds to non-existing directory")
endif()

if(NOT DEFINED DCMTK_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  set(EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS)

  if(CTEST_USE_LAUNCHERS)
    set(EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
      "-DCMAKE_PROJECT_DCMTK_INCLUDE:FILEPATH=${CMAKE_ROOT}/Modules/CTestUseLaunchers.cmake")
  endif()

  if(UNIX)
    list(APPEND EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
      -DDCMTK_FORCE_FPIC_ON_UNIX:BOOL=ON
      )
  endif()

  ExternalProject_SetIfNotDefined(
    ${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/commontk/DCMTK.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${proj}_GIT_TAG
    "ea07125078cd097245867a71d8fba8b36fd92878" # patched-DCMTK-3.6.8_20241024
    QUIET
    )

  set(location_args )
  if(${proj}_URL)
    set(location_args URL ${${proj}_URL})
  else()
    set(location_args GIT_REPOSITORY ${${proj}_GIT_REPOSITORY}
                      GIT_TAG ${${proj}_GIT_TAG})
  endif()

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

  ExternalProject_Add(${proj}
    ${${proj}_EXTERNAL_PROJECT_ARGS}
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    ${location_args}
    INSTALL_COMMAND ""
    CMAKE_ARGS
      -DDCMTK_INSTALL_BINDIR:STRING=bin/${CMAKE_CFG_INTDIR}
      -DDCMTK_INSTALL_LIBDIR:STRING=${DCMQI_INSTALL_LIB_DIR}
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
      -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
      -DBUILD_SHARED_LIBS:BOOL=OFF
      -DDCMTK_WITH_DOXYGEN:BOOL=OFF
      -DDCMTK_WITH_ZLIB:BOOL=OFF # see github issue #25
      -DDCMTK_WITH_OPENSSL:BOOL=OFF # see github issue #25
      -DDCMTK_WITH_PNG:BOOL=OFF # see github issue #25
      -DDCMTK_WITH_TIFF:BOOL=OFF  # see github issue #25
      -DDCMTK_WITH_XML:BOOL=OFF  # see github issue #25
      -DDCMTK_WITH_ICONV:BOOL=OFF  # see github issue #178
      -DDCMTK_WITH_SNDFILE:BOOL=OFF # see github issue #395
      -DDCMTK_OVERWRITE_WIN32_COMPILER_FLAGS:BOOL=OFF
      -DDCMTK_DEFAULT_DICT:STRING=builtin
      -DDCMTK_ENABLE_PRIVATE_TAGS:BOOL=ON
      -DDCMTK_COMPILE_WIN32_MULTITHREADED_DLL:BOOL=ON
      -DDCMTK_ENABLE_STL:BOOL=ON
      ${EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS}
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )
  set(DCMTK_DIR ${CMAKE_CURRENT_BINARY_DIR}/${proj}-build)

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(
  VARS DCMTK_DIR:PATH
  LABELS "FIND_PACKAGE"
  )

# If an external DCMTK was provided via DCMTK_DIR and the external DCMTK
# build/install used a CMAKE_DEBUG_POSTFIX value for distinguishing debug
# and release libraries in the same build/install tree, the same debug
# postfix needs to be passed to the CTK configure step. The FindDCMTK
# script then takes the DCMTK_CMAKE_DEBUG_POSTFIX variable into account
# when looking for DCMTK debug libraries.
mark_as_superbuild(DCMTK_CMAKE_DEBUG_POSTFIX:STRING)
