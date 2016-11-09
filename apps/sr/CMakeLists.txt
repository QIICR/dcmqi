
#-----------------------------------------------------------------------------

#
# SlicerExecutionModel
#
find_package(SlicerExecutionModel REQUIRED)
include(${SlicerExecutionModel_USE_FILE})

#-----------------------------------------------------------------------------
set(COMMON_INCLUDE_DIRECTORIES
  )

set(COMMON_DCMQI_LIB_SOURCES
  )

set(COMMON_TARGET_LIBRARIES
  dcmqi
  )

#-----------------------------------------------------------------------------
set(MODULE_NAME tid1500writer)

#-----------------------------------------------------------------------------
set(MODULE_SRCS
  ${MODULE_NAME}.cxx
  )

set(${MODULE_NAME}_TARGET_LIBRARIES
  ${COMMON_TARGET_LIBRARIES}
  )

#-----------------------------------------------------------------------------
SEMMacroBuildCLI(
  NAME ${MODULE_NAME}
  INCLUDE_DIRECTORIES ${COMMON_INCLUDE_DIRECTORIES}
  ADDITIONAL_SRCS ${MODULE_SRCS} ${COMMON_DCMQI_LIB_SOURCES}
  TARGET_LIBRARIES ${${MODULE_NAME}_TARGET_LIBRARIES}
  EXECUTABLE_ONLY
  )

#-----------------------------------------------------------------------------
set(MODULE_NAME tid1500reader)

#-----------------------------------------------------------------------------
set(MODULE_SRCS
  ${MODULE_NAME}.cxx)

set(${MODULE_NAME}_TARGET_LIBRARIES
  ${COMMON_TARGET_LIBRARIES}
  )

SEMMacroBuildCLI(
  NAME ${MODULE_NAME}
  INCLUDE_DIRECTORIES ${COMMON_INCLUDE_DIRECTORIES}
  ADDITIONAL_SRCS ${MODULE_SRCS} ${COMMON_DCMQI_LIB_SOURCES}
  TARGET_LIBRARIES ${${MODULE_NAME}_TARGET_LIBRARIES}
  EXECUTABLE_ONLY
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()