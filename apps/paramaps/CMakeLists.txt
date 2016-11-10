
#-----------------------------------------------------------------------------

#
# SlicerExecutionModel
#
find_package(SlicerExecutionModel REQUIRED)
include(${SlicerExecutionModel_USE_FILE})

#-----------------------------------------------------------------------------
set(COMMON_INCLUDE_DIRECTORIES
  )

message("Include")
message(${DCMQI_SOURCE_DIR}/include)

set(COMMON_TARGET_LIBRARIES
  dcmqi
  )

set(COMMON_SOURCE_FILES
  )

#-----------------------------------------------------------------------------
set(MODULE_NAME itkimage2paramap)

#-----------------------------------------------------------------------------
set(${MODULE_NAME}_TARGET_LIBRARIES
  ${COMMON_TARGET_LIBRARIES}
  )

#-----------------------------------------------------------------------------
SEMMacroBuildCLI(
  NAME ${MODULE_NAME}
  INCLUDE_DIRECTORIES ${COMMON_INCLUDE_DIRECTORIES}
  ADDITIONAL_SRCS ${COMMON_SOURCE_FILES} ${MODULE_NAME}.cxx
  TARGET_LIBRARIES ${${MODULE_NAME}_TARGET_LIBRARIES}
  )

#-----------------------------------------------------------------------------
set(MODULE_NAME paramap2itkimage)

#-----------------------------------------------------------------------------
set(${MODULE_NAME}_TARGET_LIBRARIES
  ${COMMON_TARGET_LIBRARIES}
  )

#-----------------------------------------------------------------------------
SEMMacroBuildCLI(
  NAME ${MODULE_NAME}
  INCLUDE_DIRECTORIES ${COMMON_INCLUDE_DIRECTORIES}
  ADDITIONAL_SRCS ${COMMON_SOURCE_FILES} ${MODULE_NAME}.cxx
  TARGET_LIBRARIES ${${MODULE_NAME}_TARGET_LIBRARIES}
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
