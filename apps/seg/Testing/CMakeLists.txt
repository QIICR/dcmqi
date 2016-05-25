set(MODULE_NAME segimage2itkimage)

add_test(NAME ${MODULE_NAME}_hello
   COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME} --help)
add_test(NAME ${MODULE_NAME}_makeNRRD
   COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME}
   ${CMAKE_SOURCE_DIR}/data/ct-3slice/liver.dcm
   ${TEMP_DIR})




set(MODULE_NAME itkimage2segimage)

add_test(NAME ${MODULE_NAME}_hello
         COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME} --help)
add_test(NAME ${MODULE_NAME}_makeSEG
         COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME}
        --metaDataFileName ${CMAKE_SOURCE_DIR}/doc/exampledata.json
        --segImageFiles ${CMAKE_SOURCE_DIR}/data/ct-3slice/liver_seg.nrrd
        --dicomImageFiles ${CMAKE_SOURCE_DIR}/data/ct-3slice/01.dcm,${CMAKE_SOURCE_DIR}/data/ct-3slice/02.dcm,${CMAKE_SOURCE_DIR}/data/ct-3slice/03.dcm
        --segDICOMFile ${TEMP_DIR}/liver.dcm
       )
