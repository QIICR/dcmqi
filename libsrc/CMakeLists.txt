add_library(dcmqi STATIC
  ParaMapConverter.cpp
  ConverterBase.cpp
  SegmentAttributes.cpp
  Helper.cpp                                  
  JSONMetaInformationHandlerBase.cpp          
  ImageSEGConverter.cpp                       
  JSONParametricMapMetaInformationHandler.cpp
  JSONSegmentationMetaInformationHandler.cpp
  ${DCMQI_SOURCE_DIR}/jsoncpp/jsoncpp.cpp
)

target_include_directories(dcmqi PUBLIC
  ${DCMTK_INCLUDE_DIRS}
  ${ITK_INCLUDE_DIRS}
  ${ITK_INSTALL_PREFIX}/include/vxl/core
  ${ITK_INSTALL_PREFIX}/include/vxl/vcl
  ${DCMQI_SOURCE_DIR}/include
  ${DCMQI_SOURCE_DIR}/jsoncpp
  )
