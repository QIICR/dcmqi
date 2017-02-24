
#-----------------------------------------------------------------------------
include(dcmqiTest)

#-----------------------------------------------------------------------------
set(MODULE_NAME paramap)

#-----------------------------------------------------------------------------
set(BASELINE ${CMAKE_SOURCE_DIR}/data/paramaps/)
include_directories(${ITK_INCLUDE_DIRS})
set(MODULE_TEMP_DIR ${TEMP_DIR}/pmap)
make_directory(${MODULE_TEMP_DIR})

#-----------------------------------------------------------------------------
set(itk2dcm itkimage2paramap)

dcmqi_add_test_executable(
  MODULE_NAME ${itk2dcm}
  EXECUTABLE_NAME ${itk2dcm}Test
  )

dcmqi_add_test(
  NAME ${itk2dcm}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}> --help
  )

dcmqi_add_test(
  NAME ${itk2dcm}_makeParametricMap
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/pm-example.json
    --inputImage ${BASELINE}/pm-example.nrrd
    --inputDICOMList ${BASELINE}/pm-example-slice.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/paramap.dcm
  )

dcmqi_add_test(
  NAME ${itk2dcm}_makeParametricMapFP
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/pm-example-float.json
    --inputImage ${BASELINE}/pm-example-float.nrrd
    --inputDICOMList ${BASELINE}/pm-example-slice.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/paramap-float.dcm
  )

#-----------------------------------------------------------------------------
set(dcm2itk paramap2itkimage)

dcmqi_add_test(
  NAME ${dcm2itk}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}> --help
  )

dcmqi_add_test_executable(
  MODULE_NAME ${dcm2itk}
  EXECUTABLE_NAME ${dcm2itk}Test
  )

dcmqi_add_test(
  NAME ${dcm2itk}_makeNRRDParametricMap
  MODULE_NAME ${MODULE_NAME}
  RESOURCE_LOCK ${MODULE_TEMP_DIR}/pmap.nrrd
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/pm-example.nrrd ${MODULE_TEMP_DIR}/pmap.nrrd
    ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/paramap.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
  TEST_DEPENDS
    ${itk2dcm}_makeParametricMap
  )

dcmqi_add_test(
  NAME ${dcm2itk}_makeNRRDParametricMapFP
  MODULE_NAME ${MODULE_NAME}
  RESOURCE_LOCK ${MODULE_TEMP_DIR}/pmap.nrrd
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/pm-example-float.nrrd ${MODULE_TEMP_DIR}/pmap.nrrd
    ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/paramap-float.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
  TEST_DEPENDS
    ${itk2dcm}_makeParametricMapFP
  )
