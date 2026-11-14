# Runs a command and checks its exit code and output.
#
#   cmake -DCOMMAND=<program|arg|...> -DEXPECTED_RESULT=<code>
#         [-DEXPECTED_REGEX=<regex>] [-DEXPECTED_STDOUT_FILE=<file>]
#         -P dcmqiCheckCommand.cmake
#
# COMMAND separates its items with '|'.
# EXPECTED_REGEX must match the standard output or the standard error.
# EXPECTED_STDOUT_FILE: the standard output must end with the content of this
# file followed by a newline, and may only have one line before it (the dcmqi
# applications print their version first). Carriage returns are ignored on
# both sides.

string(REPLACE "|" ";" command "${COMMAND}")
execute_process(
  COMMAND ${command}
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
  RESULT_VARIABLE result
  )
set(report "Command: ${command}\nExit code: ${result}\n--- stdout:\n${output}\n--- stderr:\n${error}")

if(NOT "${result}" STREQUAL "${EXPECTED_RESULT}")
  message(FATAL_ERROR "Expected exit code ${EXPECTED_RESULT}.\n${report}")
endif()

if(DEFINED EXPECTED_REGEX)
  if(NOT "${output}" MATCHES "${EXPECTED_REGEX}" AND NOT "${error}" MATCHES "${EXPECTED_REGEX}")
    message(FATAL_ERROR "Expected output matching: ${EXPECTED_REGEX}\n${report}")
  endif()
endif()

if(DEFINED EXPECTED_STDOUT_FILE)
  file(READ "${EXPECTED_STDOUT_FILE}" expected)
  string(REPLACE "\r" "" expected "${expected}")
  string(REPLACE "\r" "" output "${output}")
  string(APPEND expected "\n")
  string(LENGTH "${output}" output_length)
  string(LENGTH "${expected}" expected_length)
  set(matched FALSE)
  if(output_length GREATER_EQUAL expected_length)
    math(EXPR prefix_length "${output_length} - ${expected_length}")
    string(SUBSTRING "${output}" 0 ${prefix_length} prefix)
    string(SUBSTRING "${output}" ${prefix_length} -1 suffix)
    if("${suffix}" STREQUAL "${expected}" AND ("${prefix}" STREQUAL "" OR "${prefix}" MATCHES "^[^\n]*\n$"))
      set(matched TRUE)
    endif()
  endif()
  if(NOT matched)
    message(FATAL_ERROR "Expected the content of ${EXPECTED_STDOUT_FILE} on stdout.\n${report}")
  endif()
endif()
