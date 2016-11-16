cmake_minimum_required(VERSION 3.5.0)

#-----------------------------------------------------------------------------

#
# DCMQI
#
if(NOT DCMQI_SOURCE_DIR AND NOT Slicer_SOURCE_DIR)
  find_package(DCMQI REQUIRED)
endif()

#
# SlicerExecutionModel
#
find_package(SlicerExecutionModel REQUIRED)
include(${SlicerExecutionModel_USE_FILE})

#-----------------------------------------------------------------------------
set(COMMON_INCLUDE_DIRECTORIES
  )

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
set(MODULE_SRCS
  ${MODULE_NAME}.cxx
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
set(MODULE_NAME paramap2itkimage)

#-----------------------------------------------------------------------------
set(MODULE_SRCS
  ${MODULE_NAME}.cxx
  )
  
#-----------------------------------------------------------------------------
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
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
