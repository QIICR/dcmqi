
#-----------------------------------------------------------------------------

set(INCLUDE_DIR ../include/dcmqi)

set(HDRS
  ${INCLUDE_DIR}/DICOMFrame.h
  ${INCLUDE_DIR}/ImageVolume.h
  ${INCLUDE_DIR}/MultiframeConverter.h
  ${INCLUDE_DIR}/Helper.h
  ${INCLUDE_DIR}/SegmentationImageConverter.h
  ${INCLUDE_DIR}/ParametricMapConverter.h
  ${INCLUDE_DIR}/JSONMetaInformationHandlerBase.h
  ${INCLUDE_DIR}/JSONParametricMapMetaInformationHandler.h
  ${INCLUDE_DIR}/JSONSegmentationMetaInformationHandler.h
  ${INCLUDE_DIR}/SegmentAttributes.h
  )

set(SRCS
  DICOMFrame.cpp
  ImageVolume.cpp
  MultiframeConverter.cpp
  Helper.cpp
  SegmentationImageConverter.cpp
  ParametricMapConverter.cpp
  JSONMetaInformationHandlerBase.cpp
  JSONParametricMapMetaInformationHandler.cpp
  JSONSegmentationMetaInformationHandler.cpp
  SegmentAttributes.cpp
  )

set(ADDITIONAL_HDRS
  ${INCLUDE_DIR}/DICOMFrame.h
  ${INCLUDE_DIR}/ImageVolume.h
  ${INCLUDE_DIR}/SegmentVolume.h
  ${INCLUDE_DIR}/MultiframeObject.h
  ${INCLUDE_DIR}/ParametricMapObject.h
  ${INCLUDE_DIR}/SegmentationImageObject.h
  ${INCLUDE_DIR}/ImageVolumeGeometry.h
  ${INCLUDE_DIR}/Exceptions.h
  ${INCLUDE_DIR}/framesorter.h
  ${INCLUDE_DIR}/preproc.h
  ${INCLUDE_DIR}/QIICRConstants.h
  ${INCLUDE_DIR}/QIICRUIDs.h
  )

set(ADDITIONAL_SRCS
  DICOMFrame.cpp
  ImageVolume.cpp
  SegmentVolume.cpp
  MultiframeObject.cpp
  ParametricMapObject.cpp
  SegmentationImageObject.cpp
  ImageVolumeGeometry.cpp
  )

if(DCMQI_BUILTIN_JSONCPP)
  list(APPEND ADDITIONAL_SRCS
    ${DCMQI_SOURCE_DIR}/jsoncpp/jsoncpp.cpp
    )
  set(JsonCpp_INCLUDE_DIR ${DCMQI_SOURCE_DIR}/jsoncpp)
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