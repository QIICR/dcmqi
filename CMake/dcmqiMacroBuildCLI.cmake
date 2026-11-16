#-----------------------------------------------------------------------------
# dcmqiMacroBuildCLI(NAME <name> [TARGET_LIBRARIES <library>...] [EXECUTABLE_ONLY])
#
# Builds the dcmqi command-line application <name> from <name>.cxx and
# <name>.xml in the current source directory:
#
#  - <name>CLP.h is generated from <name>.xml by dcmqiGenerateCLP
#    (apps/GenerateCLP). It defines the PARSE_ARGS macro that <name>.cxx calls.
#  - <name>Lib is a static library with the application code. Its main() is
#    renamed ModuleEntryPoint so that test drivers can link it.
#  - <name> is the executable. <name>.xml is copied next to it and installed
#    with it.
#  - With MSVC, the C/C++ runtime DLLs are installed next to the executables
#    (InstallRequiredSystemLibraries; CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP
#    turns this off).
#
# Applications are always built as executables; EXECUTABLE_ONLY is accepted
# for compatibility and has no effect.
#
# Output directories, install destinations, link flags and the IDE folder
# default to those of dcmqi. A project embedding dcmqi, such as a 3D Slicer
# extension build, overrides them with the variables it defines for its own
# command-line modules:
#
#   SlicerExecutionModel_DEFAULT_CLI_{RUNTIME,LIBRARY,ARCHIVE}_OUTPUT_DIRECTORY
#   SlicerExecutionModel_DEFAULT_CLI_INSTALL_RUNTIME_DESTINATION
#   SlicerExecutionModel_DEFAULT_CLI_EXECUTABLE_LINK_FLAGS
#   SlicerExecutionModel_DEFAULT_CLI_TARGETS_FOLDER_PREFIX
#   SlicerExecutionModel_USE_UTF8 (Windows: use UTF-8 as the process code page)
#
# 3D Slicer turns off ITK's automatic IO factory registration and instead
# provides a main() that registers the factories before calling the
# application, with the include directories and libraries it needs. They are
# used when defined:
#
#   SlicerExecutionModel_CLI_LIBRARY_WRAPPER_CXX
#   SlicerExecutionModel_EXTRA_INCLUDE_DIRECTORIES
#   SlicerExecutionModel_EXTRA_EXECUTABLE_TARGET_LIBRARIES
#
# Sets <name>_TARGETS_FOLDER in the calling scope.
#
# Requires DCMQI_TCLAP_INCLUDE_DIR (set in the top-level CMakeLists.txt) and
# the dcmqiGenerateCLP target.
#

function(dcmqiMacroBuildCLI)
  cmake_parse_arguments(_CLI "EXECUTABLE_ONLY" "NAME" "TARGET_LIBRARIES" ${ARGN})
  if(_CLI_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "dcmqiMacroBuildCLI: unknown arguments: ${_CLI_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT _CLI_NAME)
    message(FATAL_ERROR "dcmqiMacroBuildCLI: NAME is required")
  endif()
  if(NOT TARGET dcmqiGenerateCLP)
    message(FATAL_ERROR "dcmqiMacroBuildCLI: the dcmqiGenerateCLP target is not defined")
  endif()
  if(NOT DCMQI_TCLAP_INCLUDE_DIR)
    message(FATAL_ERROR "dcmqiMacroBuildCLI: DCMQI_TCLAP_INCLUDE_DIR is not set")
  endif()

  set(name ${_CLI_NAME})
  set(xml ${CMAKE_CURRENT_SOURCE_DIR}/${name}.xml)
  set(source ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cxx)
  set(header ${CMAKE_CURRENT_BINARY_DIR}/${name}CLP.h)
  foreach(file IN ITEMS ${xml} ${source})
    if(NOT EXISTS ${file})
      message(FATAL_ERROR "dcmqiMacroBuildCLI: ${file} does not exist")
    endif()
  endforeach()

  # Locations
  foreach(type IN ITEMS RUNTIME LIBRARY ARCHIVE)
    if(DEFINED SlicerExecutionModel_DEFAULT_CLI_${type}_OUTPUT_DIRECTORY)
      set(${type}_output_directory ${SlicerExecutionModel_DEFAULT_CLI_${type}_OUTPUT_DIRECTORY})
    else()
      set(${type}_output_directory ${CMAKE_${type}_OUTPUT_DIRECTORY})
    endif()
  endforeach()
  if(DEFINED SlicerExecutionModel_DEFAULT_CLI_INSTALL_RUNTIME_DESTINATION)
    set(install_destination ${SlicerExecutionModel_DEFAULT_CLI_INSTALL_RUNTIME_DESTINATION})
  else()
    set(install_destination ${DCMQI_INSTALL_BIN_DIR})
  endif()
  if(DEFINED SlicerExecutionModel_DEFAULT_CLI_EXECUTABLE_LINK_FLAGS)
    set(link_flags ${SlicerExecutionModel_DEFAULT_CLI_EXECUTABLE_LINK_FLAGS})
  elseif(APPLE)
    set(link_flags "-Wl,-rpath,@loader_path/../../../")
  else()
    set(link_flags "")
  endif()
  if(DEFINED SlicerExecutionModel_DEFAULT_CLI_TARGETS_FOLDER_PREFIX)
    set(folder ${SlicerExecutionModel_DEFAULT_CLI_TARGETS_FOLDER_PREFIX}${name})
  else()
    set(folder Module-${name})
  endif()

  # <name>CLP.h
  add_custom_command(
    OUTPUT ${header}
    COMMAND dcmqiGenerateCLP ${xml} ${header}
    DEPENDS ${xml} dcmqiGenerateCLP
    COMMENT "Generating ${name}CLP.h"
    VERBATIM
    )

  # <name>Lib
  add_library(${name}Lib STATIC ${source} ${header})
  target_compile_definitions(${name}Lib PRIVATE main=ModuleEntryPoint)
  # The TCLAP headers go first: an embedding project may have another copy of
  # TCLAP on its include path.
  target_include_directories(${name}Lib BEFORE PUBLIC
    ${DCMQI_TCLAP_INCLUDE_DIR}
    ${DCMQI_SOURCE_DIR}/apps/GenerateCLP
    ${CMAKE_CURRENT_BINARY_DIR}
    )
  target_link_libraries(${name}Lib PUBLIC ${_CLI_TARGET_LIBRARIES})
  set_target_properties(${name}Lib PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${LIBRARY_output_directory}"
    ARCHIVE_OUTPUT_DIRECTORY "${ARCHIVE_output_directory}"
    FOLDER "${folder}"
    )

  # <name>
  if(SlicerExecutionModel_CLI_LIBRARY_WRAPPER_CXX)
    add_executable(${name} ${SlicerExecutionModel_CLI_LIBRARY_WRAPPER_CXX})
    # Without MODULE_STATIC, Slicer's wrapper declares ModuleEntryPoint as
    # imported from a DLL on Windows; here it is in the static <name>Lib.
    target_compile_definitions(${name} PRIVATE MODULE_STATIC)
  else()
    add_executable(${name} ${DCMQI_SOURCE_DIR}/CMake/dcmqiCLIExecutableWrapper.cxx)
  endif()
  if(SlicerExecutionModel_EXTRA_INCLUDE_DIRECTORIES)
    target_include_directories(${name} PRIVATE ${SlicerExecutionModel_EXTRA_INCLUDE_DIRECTORIES})
  endif()
  target_link_libraries(${name} PRIVATE ${name}Lib ${SlicerExecutionModel_EXTRA_EXECUTABLE_TARGET_LIBRARIES})
  if(WIN32 AND SlicerExecutionModel_USE_UTF8)
    target_sources(${name} PRIVATE ${DCMQI_SOURCE_DIR}/CMake/dcmqiCLIUTF8.manifest)
  endif()
  set_target_properties(${name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_output_directory}"
    FOLDER "${folder}"
    )
  if(link_flags)
    set_property(TARGET ${name} APPEND_STRING PROPERTY LINK_FLAGS " ${link_flags}")
  endif()

  # The XML descriptor goes next to the executable, where 3D Slicer looks for it.
  add_custom_command(TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${xml} $<TARGET_FILE_DIR:${name}>
    VERBATIM
    )

  install(TARGETS ${name}
    RUNTIME DESTINATION ${install_destination} COMPONENT RuntimeLibraries
    )
  install(FILES ${xml}
    DESTINATION ${install_destination} COMPONENT RuntimeLibraries
    )

  # The Microsoft C/C++ runtime DLLs are installed next to the executables,
  # once, so that the applications run on systems without the Visual C++
  # Redistributable.
  get_property(runtime_installed GLOBAL PROPERTY DCMQI_CLI_SYSTEM_RUNTIME_INSTALLED)
  if(MSVC AND NOT runtime_installed)
    set_property(GLOBAL PROPERTY DCMQI_CLI_SYSTEM_RUNTIME_INSTALLED TRUE)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${install_destination})
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT RuntimeLibraries)
    include(InstallRequiredSystemLibraries)
  endif()

  set(${name}_TARGETS_FOLDER ${folder} PARENT_SCOPE)
endfunction()
