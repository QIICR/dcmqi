
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

dcmqi_add_test(
  NAME ${itk2dcm}_makeParametricMapNoDerImg256x256
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/pm-example.json
    --inputImage ${BASELINE}/pm-example-3slices-256x256.nrrd
    --inputDICOMList ${BASELINE}/pm-example-slice.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/paramap-3slices-256x256.dcm
  )

dcmqi_add_test(
  NAME ${itk2dcm}_makeParametricMapNoModifier
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
  --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/pm-example_no_modifier.json
  --inputImage ${BASELINE}/pm-example-3slices-256x256.nrrd
  --inputDICOMList ${BASELINE}/pm-example-slice.dcm
  --outputDICOM ${MODULE_TEMP_DIR}/paramap-3slices-256x256.dcm
)

dcmqi_add_test(
  NAME ${itk2dcm}_makeParametricMapNoDerImg252x255
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/pm-example.json
    --inputImage ${BASELINE}/pm-example-3slices-252x255.nrrd
    --inputDICOMList ${BASELINE}/pm-example-slice.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/paramap-3slices-252x255.dcm
  )

find_program(DCIODVFY_EXECUTABLE dciodvfy)

if(EXISTS ${DCIODVFY_EXECUTABLE})
  dcmqi_add_test(
    NAME ${itk2dcm}_makeParametricMap_dciodvfy
    MODULE_NAME ${MODULE_NAME}
    COMMAND ${DCIODVFY_EXECUTABLE}
      ${MODULE_TEMP_DIR}/paramap.dcm
    TEST_DEPENDS
      ${itk2dcm}_makeParametricMap
  )
  dcmqi_add_test(
    NAME ${itk2dcm}_makeParametricMapFP_dciodvfy
    MODULE_NAME ${MODULE_NAME}
    COMMAND ${DCIODVFY_EXECUTABLE}
      ${MODULE_TEMP_DIR}/paramap-float.dcm
    TEST_DEPENDS
      ${itk2dcm}_makeParametricMapFP
  )
else()
  message(STATUS "Skipping test '${itk2dcm}_dciodvfy': dciodvfy executable not found")
endif()

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
    --compare ${BASELINE}/pm-example.nrrd ${MODULE_TEMP_DIR}/makeNRRDParametricMap-pmap.nrrd
    ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/paramap.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
      --prefix makeNRRDParametricMap
  TEST_DEPENDS
    ${itk2dcm}_makeParametricMap
  )

dcmqi_add_test(
  NAME ${MODULE_NAME}_meta_roundtrip
  MODULE_NAME ${MODULE_NAME}
  COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
    ${CMAKE_SOURCE_DIR}/doc/examples/pm-example.json
    ${MODULE_TEMP_DIR}/makeNRRDParametricMap-meta.json
  TEST_DEPENDS
    ${dcm2itk}_makeNRRDParametricMap
  )

dcmqi_add_test(
  NAME ${dcm2itk}_makeNRRDParametricMapFP
  MODULE_NAME ${MODULE_NAME}
  RESOURCE_LOCK ${MODULE_TEMP_DIR}/pmap.nrrd
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/pm-example-float.nrrd ${MODULE_TEMP_DIR}/makeNRRDParametricMapFP-pmap.nrrd
    ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/paramap-float.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
      --prefix makeNRRDParametricMapFP
  TEST_DEPENDS
    ${itk2dcm}_makeParametricMapFP
  )

dcmqi_add_test(
  NAME ${MODULE_NAME}_FP_meta_roundtrip
  MODULE_NAME ${MODULE_NAME}
  COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
    ${CMAKE_SOURCE_DIR}/doc/examples/pm-example-float.json
    ${MODULE_TEMP_DIR}/makeNRRDParametricMapFP-meta.json
  TEST_DEPENDS
    ${dcm2itk}_makeNRRDParametricMapFP
  )

dcmqi_add_test(
  NAME ${dcm2itk}_makeNRRDParametricMapNoDerImg256x256
  MODULE_NAME ${MODULE_NAME}
  RESOURCE_LOCK ${MODULE_TEMP_DIR}/pmap.nrrd
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/pm-example-3slices-256x256.nrrd ${MODULE_TEMP_DIR}/makeNRRDParametricMap-3slices-256x256-pmap.nrrd
    ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/paramap-3slices-256x256.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
      --prefix makeNRRDParametricMap-3slices-256x256
  TEST_DEPENDS
  ${itk2dcm}_makeParametricMapNoDerImg256x256
  )

dcmqi_add_test(
  NAME ${dcm2itk}_makeNRRDParametricMapNoDerImg252x255
  MODULE_NAME ${MODULE_NAME}
  RESOURCE_LOCK ${MODULE_TEMP_DIR}/pmap.nrrd
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/pm-example-3slices-252x255.nrrd ${MODULE_TEMP_DIR}/makeNRRDParametricMap-3slices-252x255-pmap.nrrd
    ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/paramap-3slices-252x255.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
      --prefix makeNRRDParametricMap-3slices-252x255
  TEST_DEPENDS
  ${itk2dcm}_makeParametricMapNoDerImg252x255
  )

