
#-----------------------------------------------------------------------------
include(FindGit)
include(dcmqiMacroExtractRepositoryInfo)

dcmqiMacroExtractRepositoryInfo(VAR_PREFIX DCMQI)
configure_file(
  ${DCMQI_SOURCE_DIR}/CMake/dcmqiVersionConfigure.h.in
  ${DCMQI_BINARY_DIR}/include/dcmqi/internal/VersionConfigure.h
  )

source_group("Generated" FILES
  ${DCMQI_BINARY_DIR}/include/dcmqi/internal/VersionConfigure.h
  )

#-----------------------------------------------------------------------------
set(ADDITIONAL_SRCS)

if(DCMQI_BUILTIN_JSONCPP)
  list(APPEND ADDITIONAL_SRCS
    ${DCMQI_SOURCE_DIR}/jsoncpp/jsoncpp.cpp
    )
  set(JsonCpp_INCLUDE_DIR ${DCMQI_SOURCE_DIR}/jsoncpp)
endif()

set(lib_name dcmqi)

add_library(${lib_name} STATIC
  ConverterBase.cpp
  Helper.cpp
  ImageSEGConverter.cpp
  ParaMapConverter.cpp
  JSONMetaInformationHandlerBase.cpp
  JSONParametricMapMetaInformationHandler.cpp
  JSONSegmentationMetaInformationHandler.cpp
  SegmentAttributes.cpp
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
