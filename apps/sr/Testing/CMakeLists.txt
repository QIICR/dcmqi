
#-----------------------------------------------------------------------------
include(dcmqiTest)

#-----------------------------------------------------------------------------
set(MODULE_NAME tid1500)

#-----------------------------------------------------------------------------
set(EXAMPLES ${CMAKE_SOURCE_DIR}/doc/examples)
set(SEGMENTATIONS_DIR ${CMAKE_SOURCE_DIR}/data/segmentations)
set(DICOM_DIR ${SEGMENTATIONS_DIR}/ct-3slice)

#-----------------------------------------------------------------------------
set(WRITER_MODULE_NAME tid1500writer)

dcmqi_add_test(
  NAME ${WRITER_MODULE_NAME}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${WRITER_MODULE_NAME}> --help
  )

dcmqi_add_test(
  NAME ${WRITER_MODULE_NAME}_example
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${WRITER_MODULE_NAME}>
    --inputMetadata ${EXAMPLES}/sr-tid1500-example.json
    --inputImageLibraryDirectory ${DICOM_DIR}
    --inputCompositeContextDirectory ${CMAKE_SOURCE_DIR}/data/sr-example
    --outputDICOM ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sr-tid1500-example.dcm
  )

dcmqi_add_test(
  NAME ${WRITER_MODULE_NAME}_ct-liver
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${WRITER_MODULE_NAME}>
    --inputMetadata ${EXAMPLES}/sr-tid1500-ct-liver-example.json
    --inputImageLibraryDirectory ${DICOM_DIR}
    --inputCompositeContextDirectory ${SEGMENTATIONS_DIR}
    --outputDICOM ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sr-tid1500-ct-liver-example.dcm
  )


#-----------------------------------------------------------------------------
set(READER_MODULE_NAME tid1500reader)

dcmqi_add_test(
  NAME ${READER_MODULE_NAME}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${READER_MODULE_NAME}> --help
  )

#dcmqi_add_test(
#  NAME ${READER_MODULE_NAME}_example
#  MODULE_NAME ${MODULE_NAME}
#  COMMAND $<TARGET_FILE:${READER_MODULE_NAME}>
#    --inputSRFileName ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sr-tid1500-example.dcm
#    --metaDataFileName ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sr-tid1500-example.json
#  TEST_DEPENDS
#    ${WRITER_MODULE_NAME}_example
#  )

dcmqi_add_test(
  NAME ${READER_MODULE_NAME}_ct-liver
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${READER_MODULE_NAME}>
    --inputDICOM ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sr-tid1500-ct-liver-example.dcm
    --outputMetadata ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sr-tid1500-ct-liver-example.json
  TEST_DEPENDS
    ${WRITER_MODULE_NAME}_ct-liver
  )


#-----------------------------------------------------------------------------
set(MODULE_NAME tid1500)

dcmqi_add_test(
  NAME ${MODULE_NAME}_meta_roundtrip
  MODULE_NAME ${READER_MODULE_NAME}
  COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
    ${EXAMPLES}/sr-tid1500-ct-liver-example.json
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sr-tid1500-ct-liver-example.json
      "['activitySession', 'timePoint', 'imageLibrary', 'compositeContext']"
  TEST_DEPENDS
    ${READER_MODULE_NAME}_ct-liver
  )
