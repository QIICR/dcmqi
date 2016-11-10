
#-----------------------------------------------------------------------------
include(${DCMQI_SOURCE_DIR}/CMake/FindGit.cmake)
include(${DCMQI_SOURCE_DIR}/CMake/dcmqiMacroExtractRepositoryInfo.cmake)

dcmqiMacroExtractRepositoryInfo(VAR_PREFIX dcmqi)
configure_file(
  ${DCMQI_SOURCE_DIR}/CMake/dcmqiVersionConfigure.h.in
  ${DCMQI_BINARY_DIR}/include/dcmqiVersionConfigure.h
  )

#-----------------------------------------------------------------------------
set(ADDITIONAL_SRCS)

if(BUILD_PARAMETRIC_MAPS)
  list(APPEND ADDITIONAL_SRCS
    ParaMapConverter.cpp
    JSONParametricMapMetaInformationHandler.cpp
    )
endif()

add_library(dcmqi STATIC
  ConverterBase.cpp
  SegmentAttributes.cpp
  Helper.cpp
  JSONMetaInformationHandlerBase.cpp
  ImageSEGConverter.cpp
  JSONSegmentationMetaInformationHandler.cpp
  ${DCMQI_SOURCE_DIR}/jsoncpp/jsoncpp.cpp
  ${ADDITIONAL_SRCS}
  )

target_include_directories(dcmqi PUBLIC
  ${DCMTK_INCLUDE_DIRS}
  ${ITK_INCLUDE_DIRS}
  ${ITK_INSTALL_PREFIX}/include/vxl/core
  ${ITK_INSTALL_PREFIX}/include/vxl/vcl
  ${DCMQI_SOURCE_DIR}/include
  ${DCMQI_BINARY_DIR}/include
  ${DCMQI_SOURCE_DIR}/jsoncpp
  )

target_link_libraries(dcmqi PUBLIC
  ${DCMTK_LIBRARIES}
  ${ITK_LIBRARIES}
  )
