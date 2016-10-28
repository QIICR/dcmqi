
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

set(MODULE_NAME tid1500reader)

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
  RUNTIME_OUTPUT_DIRECTORY        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
  LIBRARY_OUTPUT_DIRECTORY        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
  ARCHIVE_OUTPUT_DIRECTORY        "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
  INSTALL_RUNTIME_DESTINATION ${CMAKE_BINARY_DIR}
  INSTALL_LIBRARY_DESTINATION ${CMAKE_BINARY_DIR}
  INSTALL_ARCHIVE_DESTINATION ${CMAKE_BINARY_DIR}
  EXECUTABLE_ONLY
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()