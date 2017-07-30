
#-----------------------------------------------------------------------------

file(GLOB_RECURSE HDRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include/dcmqi/*.h
  )

file(GLOB_RECURSE SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/libsrc/*.cpp
  )

if(DCMQI_BUILTIN_JSONCPP)
  list(APPEND SRCS
    ${DCMQI_SOURCE_DIR}/jsoncpp/jsoncpp.cpp
    )
  set(JsonCpp_INCLUDE_DIR ${DCMQI_SOURCE_DIR}/jsoncpp)
endif()

set(lib_name dcmqi)

add_library(${lib_name} STATIC
  ${HDRS}
  ${SRCS}
  )

if(DCMQI_LIBRARY_PROPERTIES)
  set_target_properties(${lib_name} PROPERTIES ${DCMQI_LIBRARY_PROPERTIES})
endif()

set_property(GLOBAL APPEND PROPERTY ${CMAKE_PROJECT_NAME}_TARGETS ${lib_name})

set(${lib_name}_INCLUDE_DIRS
  ${DCMTK_INCLUDE_DIRS}
  ${ITK_INCLUDE_DIRS}
  ${DCMQI_SOURCE_DIR}/include
  ${DCMQI_BINARY_DIR}/include
  ${JsonCpp_INCLUDE_DIR}
  )
if(ITK_INSTALL_PREFIX)
  list(APPEND ${lib_name}_INCLUDE_DIRS
    ${ITK_INSTALL_PREFIX}/include/vxl/core
    ${ITK_INSTALL_PREFIX}/include/vxl/vcl
    )
endif()
target_include_directories(${lib_name} PUBLIC ${${lib_name}_INCLUDE_DIRS})

target_link_libraries(${lib_name} PUBLIC
  ${DCMTK_LIBRARIES}
  ${ITK_LIBRARIES}
  $<$<NOT:$<BOOL:${DCMQI_BUILTIN_JSONCPP}>>:${JsonCpp_LIBRARY}>
  )