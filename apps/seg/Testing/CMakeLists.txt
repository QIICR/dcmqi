
#-----------------------------------------------------------------------------
include(dcmqiTest)

#-----------------------------------------------------------------------------
set(MODULE_NAME segmentation)

#-----------------------------------------------------------------------------
set(BASELINE ${CMAKE_SOURCE_DIR}/data/ct-3slice/)
set(MODULE_TEMP_DIR ${TEMP_DIR}/seg)
make_directory(${MODULE_TEMP_DIR})

#-----------------------------------------------------------------------------
set(itk2dcm itkimage2segimage)

dcmqi_add_test(
  NAME ${itk2dcm}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}> --help
  )

dcmqi_add_test(
  NAME ${itk2dcm}_makeSEG
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --metaDataFileName ${CMAKE_SOURCE_DIR}/doc/seg-example.json
    --segImageFiles ${BASELINE}/liver_seg.nrrd
    --dicomImageFiles ${BASELINE}/01.dcm,${BASELINE}/02.dcm,${BASELINE}/03.dcm
    --segDICOMFile ${MODULE_TEMP_DIR}/liver.dcm
  )

find_program(DCIODVFY_EXECUTABLE dciodvfy)

if(EXISTS ${DCIODVFY_EXECUTABLE})
  dcmqi_add_test(
    NAME ${itk2dcm}_dciodvfy
    MODULE_NAME dciodvfy
    COMMAND ${DCIODVFY_EXECUTABLE}
      ${MODULE_TEMP_DIR}/liver.dcm
    )
else()
  message(STATUS "Skipping test '${itk2dcm}_dciodvfy': dciodvfy executable not found")
endif()

#-----------------------------------------------------------------------------
set(dcm2itk segimage2itkimage)

dcmqi_add_test(
  NAME ${dcm2itk}_hello
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}> --help)

include_directories(${ITK_INCLUDE_DIRS})
add_executable(${dcm2itk}Test ${dcm2itk}Test.cxx )
target_link_libraries(${dcm2itk}Test ${dcm2itk}Lib ${SlicerExecutionModel_EXTRA_EXECUTABLE_TARGET_LIBRARIES})
set_target_properties(${dcm2itk}Test PROPERTIES LABELS ${MODULE_NAME})
set_target_properties(${dcm2itk}Test PROPERTIES FOLDER ${${dcm2itk}_TARGETS_FOLDER})
if(TARGET ITKFactoryRegistration)
  target_compile_definitions(${dcm2itk}Test PUBLIC HAS_ITK_FACTORY_REGISTRATION)
endif()

dcmqi_add_test(
  NAME ${dcm2itk}_makeNRRD
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/liver_seg.nrrd
    ${MODULE_TEMP_DIR}/1.nrrd
    ${dcm2itk}Test
    ${MODULE_TEMP_DIR}/liver.dcm
    ${MODULE_TEMP_DIR}
  TEST_DEPENDS
    ${itk2dcm}_makeSEG
  )

dcmqi_add_test(
  NAME seg_meta_roundtrip
  MODULE_NAME ${MODULE_NAME}
  COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
    ${CMAKE_SOURCE_DIR}/doc/seg-example.json
    ${MODULE_TEMP_DIR}/meta.json
  TEST_DEPENDS
    ${dcm2itk}_makeNRRD
  )
