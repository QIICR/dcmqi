# This file contains CMake functions and macros used when testing DCMQI
# libraries and modules.

#-----------------------------------------------------------------------------
# Create DCMQI executable wrapper for testing the selected command-line modules.
#
# Arguments - Input
#   EXECUTABLE_NAME - name of the executable wrapper
#   MODULE_NAME     - name of the module to test
#
macro(dcmqi_add_test_executable)
  set(options
  )
  set(oneValueArgs
    EXECUTABLE_NAME
    MODULE_NAME
  )
  set(multiValueArgs
    SOURCES
  )
  cmake_parse_arguments(_SELF
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
    )
  set(_name ${_SELF_EXECUTABLE_NAME})

  set(_source ${_name}.cxx)

  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_source})
    message(FATAL_ERROR "Failed to add test executable: Source file '${_source}' is missing")
  endif()

  add_executable(${_name} ${_source})
  target_link_libraries(${_name}
    ${_SELF_MODULE_NAME}Lib
    ${SlicerExecutionModel_EXTRA_EXECUTABLE_TARGET_LIBRARIES}
    )
  set_target_properties(${_name} PROPERTIES LABELS ${_SELF_MODULE_NAME})
  set_target_properties(${_name} PROPERTIES FOLDER ${${_SELF_MODULE_NAME}_TARGETS_FOLDER})
  if(TARGET ITKFactoryRegistration)
    target_compile_definitions(${_name} PUBLIC HAS_ITK_FACTORY_REGISTRATION)
  endif()
endmacro()

#-----------------------------------------------------------------------------
# DCMQI wrapper for add_test that set test LABELS using MODULE_NAME value
# and add test dependencies if TEST_DEPENDS is specified.
#
# Usage:
#
#  dcmqi_add_test(
#    NAME itkimage2segimage_hello
#    MODULE_NAME segmentation
#    COMMAND $<TARGET_FILE:itkimage2segimage> --help
#    )
#
# where in addition of the arguments already accepted by 'add_test',
# the following named arguments are also valid:
#
# MODULE_NAME      - string used as test LABEL (required)
# COMMAND          - command to execute  (required)
#                    When build as a Slicer extension the command will be
#                    executed using ${SEM_LAUNCH_COMMAND
# TEST_DEPENDS     - list of test dependencies (optional)
# RESOURCE_LOCK    - Specify a list of resources that are locked by this test.
#                    If multiple tests specify the same resource lock, they are
#                    guaranteed not to run concurrently.
#
#
macro(dcmqi_add_test)
  set(options
  )
  set(oneValueArgs
    NAME
    MODULE_NAME
  )
  set(multiValueArgs
    COMMAND
    TEST_DEPENDS
    RESOURCE_LOCK
  )
  cmake_parse_arguments(_SELF
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
    )

  set(_command ${_SELF_COMMAND})
  if(SEM_LAUNCH_COMMAND)
    set(_command ${SEM_LAUNCH_COMMAND} ${_SELF_COMMAND})
  endif()

  add_test(
    NAME ${_SELF_NAME}
    COMMAND ${_command}
    ${_SELF_UNPARSED_ARGUMENTS}
    )
  set_property(TEST ${_SELF_NAME} PROPERTY LABELS ${_SELF_MODULE_NAME})

  if(_SELF_TEST_DEPENDS)
    set_tests_properties(${_SELF_NAME}
      PROPERTIES DEPENDS ${_SELF_TEST_DEPENDS}
      )
  endif()
  if(_SELF_RESOURCE_LOCK)
    set_tests_properties(${_SELF_NAME}
      PROPERTIES RESOURCE_LOCK ${_SELF_RESOURCE_LOCK}
      )
  endif()

endmacro()
