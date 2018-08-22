
#-----------------------------------------------------------------------------
include(dcmqiTest)

#-----------------------------------------------------------------------------
set(MODULE_NAME tid1500)

#-----------------------------------------------------------------------------
set(EXAMPLES ${CMAKE_SOURCE_DIR}/doc/examples)
set(SEGMENTATIONS_DIR ${CMAKE_SOURCE_DIR}/data/segmentations)
set(DICOM_DIR ${SEGMENTATIONS_DIR}/ct-3slice)
set(MODULE_TEMP_DIR ${TEMP_DIR}/tid1500)
make_directory(${MODULE_TEMP_DIR})

#-----------------------------------------------------------------------------
set(WRITER_MODULE_NAME tid1500writer)

dcmqi_add_test(
  NAME ${WRITER_MODULE_NAME}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${WRITER_MODULE_NAME}> --help
  )

dcmqi_add_test(
  NAME ${WRITER_MODULE_NAME}_nan
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${WRITER_MODULE_NAME}>
    --inputMetadata ${EXAMPLES}/sr-tid1500-example-nan.json
    --inputImageLibraryDirectory ${DICOM_DIR}
    --inputCompositeContextDirectory ${CMAKE_SOURCE_DIR}/data/sr-example
    --outputDICOM ${MODULE_TEMP_DIR}/sr-tid1500-example-nan.dcm
  )

dcmqi_add_test(
  NAME ${WRITER_MODULE_NAME}_example
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${WRITER_MODULE_NAME}>
    --inputMetadata ${EXAMPLES}/sr-tid1500-example.json
    --inputImageLibraryDirectory ${DICOM_DIR}
    --inputCompositeContextDirectory ${CMAKE_SOURCE_DIR}/data/sr-example
    --outputDICOM ${MODULE_TEMP_DIR}/sr-tid1500-example.dcm
  )

dcmqi_add_test(
  NAME ${WRITER_MODULE_NAME}_ct-liver
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${WRITER_MODULE_NAME}>
    --inputMetadata ${EXAMPLES}/sr-tid1500-ct-liver-example.json
    --inputImageLibraryDirectory ${DICOM_DIR}
    --inputCompositeContextDirectory ${SEGMENTATIONS_DIR}
    --outputDICOM ${MODULE_TEMP_DIR}/sr-tid1500-ct-liver-example.dcm
  )

  dcmqi_add_test(
    NAME ${WRITER_MODULE_NAME}_qualitative
    MODULE_NAME ${MODULE_NAME}
    COMMAND $<TARGET_FILE:${WRITER_MODULE_NAME}>
      --inputMetadata ${EXAMPLES}/sr-tid1500-qualitative.json
      --inputImageLibraryDirectory ${DICOM_DIR}
      --inputCompositeContextDirectory ${CMAKE_SOURCE_DIR}/data/sr-example
      --outputDICOM ${MODULE_TEMP_DIR}/sr-tid1500-qualitative.dcm
    )

find_program(DCIODVFY_EXECUTABLE dciodvfy)

if(EXISTS ${DCIODVFY_EXECUTABLE})
  dcmqi_add_test(
    NAME ${WRITER_MODULE_NAME}_example_dciodvfy
    MODULE_NAME ${MODULE_NAME}
    COMMAND ${DCIODVFY_EXECUTABLE}
      ${MODULE_TEMP_DIR}/sr-tid1500-example.dcm
    TEST_DEPENDS
      ${WRITER_MODULE_NAME}_example
  )
  dcmqi_add_test(
    NAME ${WRITER_MODULE_NAME}_ct-liver_dciodvfy
    MODULE_NAME ${MODULE_NAME}
    COMMAND ${DCIODVFY_EXECUTABLE}
      ${MODULE_TEMP_DIR}/sr-tid1500-ct-liver-example.dcm
    TEST_DEPENDS
      ${WRITER_MODULE_NAME}_ct-liver
  )
else()
  message(STATUS "Skipping test '${WRITER_MODULE_NAME}_dciodvfy': dciodvfy executable not found")
endif()

#-----------------------------------------------------------------------------
set(READER_MODULE_NAME tid1500reader)

dcmqi_add_test(
  NAME ${READER_MODULE_NAME}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${READER_MODULE_NAME}> --help
  )

dcmqi_add_test(
  NAME ${READER_MODULE_NAME}_example
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${READER_MODULE_NAME}>
    --inputDICOM ${MODULE_TEMP_DIR}/sr-tid1500-example.dcm
    --outputMetadata ${MODULE_TEMP_DIR}/sr-tid1500-example.json
  TEST_DEPENDS
    ${WRITER_MODULE_NAME}_example
  )

dcmqi_add_test(
  NAME ${READER_MODULE_NAME}_ct-liver
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${READER_MODULE_NAME}>
    --inputDICOM ${MODULE_TEMP_DIR}/sr-tid1500-ct-liver-example.dcm
    --outputMetadata ${MODULE_TEMP_DIR}/sr-tid1500-ct-liver-example.json
  TEST_DEPENDS
    ${WRITER_MODULE_NAME}_ct-liver
  )

dcmqi_add_test(
  NAME ${READER_MODULE_NAME}_qualitative
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${READER_MODULE_NAME}>
    --inputDICOM ${MODULE_TEMP_DIR}/sr-tid1500-qualitative.dcm
    --outputMetadata ${MODULE_TEMP_DIR}/sr-tid1500-qualitative.json
  TEST_DEPENDS
    ${WRITER_MODULE_NAME}_qualitative
  )

#-----------------------------------------------------------------------------
set(MODULE_NAME tid1500)

dcmqi_add_test(
  NAME ${MODULE_NAME}_meta_roundtrip
  MODULE_NAME ${READER_MODULE_NAME}
  COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
    ${EXAMPLES}/sr-tid1500-ct-liver-example.json
    ${MODULE_TEMP_DIR}/sr-tid1500-ct-liver-example.json
      "['activitySession', 'timePoint', 'imageLibrary', 'compositeContext','procedureReported']"
  TEST_DEPENDS
    ${READER_MODULE_NAME}_ct-liver
  )
