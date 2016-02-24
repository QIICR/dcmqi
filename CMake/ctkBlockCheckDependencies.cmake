
#
# Independently of the value of CTK_SUPERBUILD, each external project definition will
# provides either the include and library directories or a variable name
# used by the corresponding Find<package>.cmake files.
#
# Within top-level CMakeLists.txt file, the variable names will be expanded if not in
# superbuild mode. The include and library dirs are then used in
# ctkMacroBuildApp, ctkMacroBuildLib, and ctkMacroBuildPlugin
#

#-----------------------------------------------------------------------------
# Collect CTK library target dependencies
#

ctkMacroCollectAllTargetLibraries("${DCMQI_LIBS}" "Libs" ALL_TARGET_LIBRARIES)
#message(STATUS ALL_TARGET_LIBRARIES:${ALL_TARGET_LIBRARIES})

#-----------------------------------------------------------------------------
# Initialize NON_CTK_DEPENDENCIES variable
#
# Using the variable ALL_TARGET_LIBRARIES initialized above with the help
# of the macro ctkMacroCollectAllTargetLibraries, let's get the list of all Non-CTK dependencies.
# NON_CTK_DEPENDENCIES is expected by the macro ctkMacroShouldAddExternalProject
#ctkMacroGetAllNonProjectTargetLibraries("${ALL_TARGET_LIBRARIES}" NON_CTK_DEPENDENCIES)
#message(NON_CTK_DEPENDENCIES:${NON_CTK_DEPENDENCIES})

#-----------------------------------------------------------------------------
# Enable and setup External project global properties
#

if(DCMQI_SUPERBUILD)
  set(ep_install_dir ${CMAKE_BINARY_DIR}/CMakeExternals/Install)
  set(ep_suffix      "-cmake")

  set(ep_common_c_flags "${CMAKE_C_FLAGS_INIT} ${ADDITIONAL_C_FLAGS}")
  set(ep_common_cxx_flags "${CMAKE_CXX_FLAGS_INIT} ${ADDITIONAL_CXX_FLAGS}")

  set(ep_common_cache_args
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_INSTALL_PREFIX:PATH=${ep_install_dir}
      -DCMAKE_PREFIX_PATH:STRING=${CMAKE_PREFIX_PATH}
      -DBUILD_TESTING:BOOL=OFF
     )
endif()

if(NOT DEFINED DCMQI_DEPENDENCIES)
  message(FATAL_ERROR "error: DCMQI_DEPENDENCIES variable is not defined !")
endif()

set(DCMTK_enabling_variable DCMTK_LIBRARIES)
set(${DCMTK_enabling_variable}_INCLUDE_DIRS DCMTK_INCLUDE_DIR)
set(${DCMTK_enabling_variable}_FIND_PACKAGE_CMD DCMTK)

set(ITK_enabling_variable ITK_LIBRARIES)
set(${ITK_enabling_variable}_LIBRARY_DIRS ITK_LIBRARY_DIRS)
set(${ITK_enabling_variable}_INCLUDE_DIRS ITK_INCLUDE_DIRS)
set(${ITK_enabling_variable}_FIND_PACKAGE_CMD ITK)

if(FALSE)
macro(superbuild_is_external_project_includable possible_proj output_var)
  if(DEFINED ${possible_proj}_enabling_variable)
    ctkMacroShouldAddExternalProject(${${possible_proj}_enabling_variable} ${output_var})
    if(NOT ${${output_var}})
      unset(${${possible_proj}_enabling_variable}_INCLUDE_DIRS)
      unset(${${possible_proj}_enabling_variable}_LIBRARY_DIRS)
      unset(${${possible_proj}_enabling_variable}_FIND_PACKAGE_CMD)
    endif()
  else()
    set(${output_var} 0)
  endif()
endmacro()
endif()

message("Initial DCMQI_DEPENDENCIES: ${DCMQI_DEPENDENCIES}")
set(proj DCMQI)
ExternalProject_Include_Dependencies(DCMQI
  PROJECT_VAR proj
  DEPENDS_VAR DCMQI_DEPENDENCIES
  USE_SYSTEM_VAR ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj}
  )

message("Updated DCMQI_DEPENDENCIES:")
foreach(dep ${DCMQI_DEPENDENCIES})
  message("  ${dep}")
endforeach()
