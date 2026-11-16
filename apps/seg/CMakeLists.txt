
#-----------------------------------------------------------------------------

#
# Command-line applications
#
include(dcmqiMacroBuildCLI)

#-----------------------------------------------------------------------------
set(MODULE_NAME segimage2itkimage)

#-----------------------------------------------------------------------------
dcmqiMacroBuildCLI(
  NAME ${MODULE_NAME}
  TARGET_LIBRARIES dcmqi
  EXECUTABLE_ONLY
  )

if(WIN32)
  # Due to name clash of "max" macro, build may fail error on Windows without defining NOMINMAX.
  target_compile_definitions(${MODULE_NAME}Lib PRIVATE NOMINMAX)
endif()

#-----------------------------------------------------------------------------
set(MODULE_NAME itkimage2segimage)

#-----------------------------------------------------------------------------
dcmqiMacroBuildCLI(
  NAME ${MODULE_NAME}
  TARGET_LIBRARIES dcmqi
  EXECUTABLE_ONLY
  )

if(WIN32)
  # Due to name clash of "max" macro, build may fail error on Windows without defining NOMINMAX.
  target_compile_definitions(${MODULE_NAME}Lib PRIVATE NOMINMAX)
endif()

#-----------------------------------------------------------------------------
set(MODULE_NAME bin2labelsegimage)

#-----------------------------------------------------------------------------
dcmqiMacroBuildCLI(
  NAME ${MODULE_NAME}
  TARGET_LIBRARIES dcmqi
  EXECUTABLE_ONLY
  )

if(WIN32)
  # Due to name clash of "max" macro, build may fail error on Windows without defining NOMINMAX.
  target_compile_definitions(${MODULE_NAME}Lib PRIVATE NOMINMAX)
endif()

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
