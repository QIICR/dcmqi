
#-----------------------------------------------------------------------------
set(ADDITIONAL_HDRS)
set(ADDITIONAL_SRCS)

set(INCLUDE_DIR ../include/dcmqi)

set(HDRS
  ${INCLUDE_DIR}/preproc.h
  ${INCLUDE_DIR}/QIICRConstants.h
  ${INCLUDE_DIR}/QIICRUIDs.h
  ${INCLUDE_DIR}/ConverterBase.h
  ${INCLUDE_DIR}/Exceptions.h
  ${INCLUDE_DIR}/framesorter.h
  ${INCLUDE_DIR}/ImageSEGConverter.h
  ${INCLUDE_DIR}/ParaMapConverter.h
  ${INCLUDE_DIR}/Helper.h
  ${INCLUDE_DIR}/ColorUtilities.h
  ${INCLUDE_DIR}/JSONMetaInformationHandlerBase.h
  ${INCLUDE_DIR}/JSONParametricMapMetaInformationHandler.h
  ${INCLUDE_DIR}/JSONSegmentationMetaInformationHandler.h
  ${INCLUDE_DIR}/SegmentAttributes.h
  ${INCLUDE_DIR}/TID1500Reader.h
  )

set(SRCS
  ConverterBase.cpp
  ImageSEGConverter.cpp
  ParaMapConverter.cpp
  Helper.cpp
  ColorUtilities.cpp
  JSONMetaInformationHandlerBase.cpp
  JSONParametricMapMetaInformationHandler.cpp
  JSONSegmentationMetaInformationHandler.cpp
  SegmentAttributes.cpp
  TID1500Reader.cpp
  )


if(DCMQI_BUILTIN_JSONCPP)
  list(APPEND ADDITIONAL_HDRS
    ${DCMQI_SOURCE_DIR}/jsoncpp/json/json.h
    ${DCMQI_SOURCE_DIR}/jsoncpp/json/json-forwards.h
    )
  list(APPEND ADDITIONAL_SRCS
    ${DCMQI_SOURCE_DIR}/jsoncpp/jsoncpp.cpp
    )
  set(JsonCpp_INCLUDE_DIR $<BUILD_INTERFACE:${DCMQI_SOURCE_DIR}/jsoncpp>)
  install(DIRECTORY ${INCLUDE_DIR} ../jsoncpp/json DESTINATION include)
endif()

set(lib_name dcmqi)

add_library(${lib_name} STATIC
  ${HDRS}
  ${SRCS}
  ${ADDITIONAL_HDRS}
  ${ADDITIONAL_SRCS}
  )

if(DCMQI_LIBRARY_PROPERTIES)
  set_target_properties(${lib_name} PROPERTIES ${DCMQI_LIBRARY_PROPERTIES})
endif()

set_property(GLOBAL APPEND PROPERTY ${CMAKE_PROJECT_NAME}_TARGETS ${lib_name})

set(${lib_name}_INCLUDE_DIRS
  ${DCMTK_INCLUDE_DIRS}
  ${ITK_INCLUDE_DIRS}
  ${JsonCpp_INCLUDE_DIR}
  $<BUILD_INTERFACE:${DCMQI_SOURCE_DIR}/include>  
  $<BUILD_INTERFACE:${DCMQI_BINARY_DIR}/include>  
  )
if(ITK_INSTALL_PREFIX)
  if(EXISTS ${ITK_INSTALL_PREFIX}/include/vxl/core AND EXISTS ${ITK_INSTALL_PREFIX}/include/vxl/vcl)
    list(APPEND ${lib_name}_INCLUDE_DIRS
      ${ITK_INSTALL_PREFIX}/include/vxl/core
      ${ITK_INSTALL_PREFIX}/include/vxl/vcl
      )
  endif()
endif()
target_include_directories(${lib_name} PUBLIC ${${lib_name}_INCLUDE_DIRS})

target_link_libraries(${lib_name} PUBLIC
  ${DCMTK_LIBRARIES}
  ${ITK_LIBRARIES}
  $<$<NOT:$<BOOL:${DCMQI_BUILTIN_JSONCPP}>>:${JsonCpp_LIBRARY}>
  )

if(export_targets)
  install(TARGETS ${lib_name}
          EXPORT ${PROJECT_NAME}Targets
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
          RUNTIME DESTINATION bin
          INCLUDES DESTINATION include
    )
  install(DIRECTORY ${INCLUDE_DIR} DESTINATION include)
endif()
