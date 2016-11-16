# This file contains CMake functions and macros used when testing DCMQI
# libraries and modules.

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
# TEST_DEPENDS     - list of test dependencies (optional)
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
    TEST_DEPENDS
  )
  cmake_parse_arguments(_SELF
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
    )

  add_test(NAME ${_SELF_NAME} ${_SELF_UNPARSED_ARGUMENTS})
  set_property(TEST ${_SELF_NAME} PROPERTY LABELS ${_SELF_MODULE_NAME})

  if(_SELF_DEPENDS)
    set_tests_properties(${_SELF_NAME}
      PROPERTIES DEPENDS ${_SELF_TEST_DEPENDS}
      )
  endif()

endmacro()
