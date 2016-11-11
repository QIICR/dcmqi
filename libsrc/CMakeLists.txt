
#-----------------------------------------------------------------------------
include(FindGit)
include(dcmqiMacroExtractRepositoryInfo)

dcmqiMacroExtractRepositoryInfo(VAR_PREFIX dcmqi)
configure_file(
  ${DCMQI_SOURCE_DIR}/CMake/dcmqiVersionConfigure.h.in
  ${DCMQI_BINARY_DIR}/include/dcmqi/internal/VersionConfigure.h
  )

source_group("Generated" FILES
  ${DCMQI_BINARY_DIR}/include/dcmqi/internal/VersionConfigure.h
  )

#-----------------------------------------------------------------------------
set(ADDITIONAL_SRCS)

if(BUILD_PARAMETRIC_MAPS)
  list(APPEND ADDITIONAL_SRCS
    ParaMapConverter.cpp
    JSONParametricMapMetaInformationHandler.cpp
    )
endif()

if(BUILTIN_JSONCPP)
  list(APPEND ADDITIONAL_SRCS
    ${DCMQI_SOURCE_DIR}/jsoncpp/jsoncpp.cpp
    )
  set(JsonCpp_INCLUDE_DIR ${DCMQI_SOURCE_DIR}/jsoncpp)
endif()

set(lib_name dcmqi)

add_library(${lib_name} STATIC
  ConverterBase.cpp
  SegmentAttributes.cpp
  Helper.cpp
  JSONMetaInformationHandlerBase.cpp
  ImageSEGConverter.cpp
  JSONSegmentationMetaInformationHandler.cpp
  ${ADDITIONAL_SRCS}
  )

target_include_directories(${lib_name} PUBLIC
  ${DCMTK_INCLUDE_DIRS}
  ${ITK_INCLUDE_DIRS}
  ${ITK_INSTALL_PREFIX}/include/vxl/core
  ${ITK_INSTALL_PREFIX}/include/vxl/vcl
  ${DCMQI_SOURCE_DIR}/include
  ${DCMQI_BINARY_DIR}/include
  ${JsonCpp_INCLUDE_DIR}
  )

target_link_libraries(${lib_name} PUBLIC
  ${DCMTK_LIBRARIES}
  ${ITK_LIBRARIES}
  $<$<NOT:$<BOOL:${BUILTIN_JSONCPP}>>:${JsonCpp_LIBRARY}>
  )
