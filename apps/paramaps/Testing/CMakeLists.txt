
#-----------------------------------------------------------------------------
include(dcmqiTest)

#-----------------------------------------------------------------------------
set(BASELINE ${CMAKE_SOURCE_DIR}/data/paramaps/)
include_directories(${ITK_INCLUDE_DIRS})

#-----------------------------------------------------------------------------
set(MODULE_NAME itkimage2paramap)

dcmqi_add_test_executable(
  MODULE_NAME ${MODULE_NAME}
  EXECUTABLE_NAME ${MODULE_NAME}Test
  )

dcmqi_add_test(
  NAME ${MODULE_NAME}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${MODULE_NAME}> --help
  )

dcmqi_add_test(
  NAME ${MODULE_NAME}_makeParametricMap
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${MODULE_NAME}>
    --metaDataFileName ${CMAKE_SOURCE_DIR}/doc/pm-example.json
    --inputFileName ${BASELINE}/pm-example.nrrd
    --outputParaMapFileName ${TEMP_DIR}/paramap.dcm
    --dicomImageFileName ${BASELINE}/pm-example-slice.dcm
  )

dcmqi_add_test(
  NAME ${MODULE_NAME}_makeParametricMapFP
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${MODULE_NAME}>
    --metaDataFileName ${CMAKE_SOURCE_DIR}/doc/pm-example-float.json
    --inputFileName ${BASELINE}/pm-example-float.nrrd
    --outputParaMapFileName ${TEMP_DIR}/paramap-float.dcm
    --dicomImageFileName ${BASELINE}/pm-example-slice.dcm
  )

#-----------------------------------------------------------------------------
set(MODULE_NAME paramap2itkimage)

dcmqi_add_test(
  NAME ${MODULE_NAME}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${MODULE_NAME}> --help
  )

dcmqi_add_test_executable(
  MODULE_NAME ${MODULE_NAME}
  EXECUTABLE_NAME ${MODULE_NAME}Test
  )

dcmqi_add_test(
  NAME ${MODULE_NAME}_makeNRRDParametricMap
  MODULE_NAME ${MODULE_NAME}
  RESOURCE_LOCK ${TEMP_DIR}/pmap.nrrd
  COMMAND $<TARGET_FILE:${MODULE_NAME}Test>
    --compare ${BASELINE}/pm-example.nrrd ${TEMP_DIR}/pmap.nrrd
    ${MODULE_NAME}Test
      --inputFileName ${TEMP_DIR}/paramap.dcm
      --outputDirName ${TEMP_DIR}
  TEST_DEPENDS
    itkimage2paramap_makeParametricMap
  )

dcmqi_add_test(
  NAME ${MODULE_NAME}_makeNRRDParametricMapFP
  MODULE_NAME ${MODULE_NAME}
  RESOURCE_LOCK ${TEMP_DIR}/pmap.nrrd
  COMMAND $<TARGET_FILE:${MODULE_NAME}Test>
    --compare ${BASELINE}/pm-example-float.nrrd ${TEMP_DIR}/pmap.nrrd
    ${MODULE_NAME}Test
      --inputFileName ${TEMP_DIR}/paramap-float.dcm
      --outputDirName ${TEMP_DIR}
  TEST_DEPENDS
    itkimage2paramap_makeParametricMapFP
  )
