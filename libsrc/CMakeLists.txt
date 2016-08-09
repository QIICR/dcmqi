if(BUILD_PARAMETRIC_MAPS)
  add_library(dcmqi STATIC
    TID1500Converter.cpp
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
else()
  add_library(dcmqi STATIC
    TID1500Converter.cpp
    ConverterBase.cpp
    SegmentAttributes.cpp
    Helper.cpp                                  
    JSONMetaInformationHandlerBase.cpp          
    ImageSEGConverter.cpp                       
    JSONSegmentationMetaInformationHandler.cpp
    ${DCMQI_SOURCE_DIR}/jsoncpp/jsoncpp.cpp
  )
endif(BUILD_PARAMETRIC_MAPS)

target_include_directories(dcmqi PUBLIC
  ${DCMTK_INCLUDE_DIRS}
  ${ITK_INCLUDE_DIRS}
  ${ITK_INSTALL_PREFIX}/include/vxl/core
  ${ITK_INSTALL_PREFIX}/include/vxl/vcl
  ${DCMQI_SOURCE_DIR}/include
  ${DCMQI_SOURCE_DIR}/jsoncpp
  )

target_link_libraries(dcmqi PUBLIC
  ${DCMTK_LIBRARIES}
  ${ITK_LIBRARIES}
  )
