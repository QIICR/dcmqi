set(MODULE_NAME segmentation)
set(BASELINE ${CMAKE_SOURCE_DIR}/data/ct-3slice/)

set(MODULE_TEMP_DIR ${TEMP_DIR}/seg)
make_directory(${MODULE_TEMP_DIR})

#-----------------------------------------------------------------------------
set(itk2dcm itkimage2segimage)

add_test(NAME ${itk2dcm}_hello
  COMMAND $<TARGET_FILE:${itk2dcm}> --help)
set_property(TEST ${itk2dcm}_hello PROPERTY LABELS ${MODULE_NAME})

add_test(NAME ${itk2dcm}_makeSEG
  COMMAND $<TARGET_FILE:${itk2dcm}>
  --metaDataFileName ${CMAKE_SOURCE_DIR}/doc/seg-example.json
  --segImageFiles ${BASELINE}/liver_seg.nrrd
  --dicomImageFiles ${BASELINE}/01.dcm,${BASELINE}/02.dcm,${BASELINE}/03.dcm
  --segDICOMFile ${MODULE_TEMP_DIR}/liver.dcm)
set_property(TEST ${itk2dcm}_makeSEG PROPERTY LABELS ${MODULE_NAME})

find_program(DCIODVFY_EXECUTABLE dciodvfy)

if(EXISTS ${DCIODVFY_EXECUTABLE})
  add_test(NAME ${itk2dcm}_dciodvfy
    COMMAND ${DCIODVFY_EXECUTABLE} ${MODULE_TEMP_DIR}/liver.dcm)
  set_property(TEST ${itk2dcm}_dciodvfy PROPERTY LABELS dciodvfy)
else()
  message(STATUS "Skipping test '${itk2dcm}_dciodvfy': dciodvfy executable not found")
endif()

#-----------------------------------------------------------------------------
set(dcm2itk segimage2itkimage)

add_test(NAME ${dcm2itk}_hello
         COMMAND $<TARGET_FILE:${dcm2itk}> --help)
set_property(TEST ${dcm2itk}_hello PROPERTY LABELS ${MODULE_NAME})

include_directories(${ITK_INCLUDE_DIRS})
add_executable(${dcm2itk}Test ${dcm2itk}Test.cxx )
target_link_libraries(${dcm2itk}Test ${dcm2itk}Lib)
set_target_properties(${dcm2itk}Test PROPERTIES LABELS ${MODULE_NAME})
set_target_properties(${dcm2itk}Test PROPERTIES FOLDER ${${dcm2itk}_TARGETS_FOLDER})

add_test(NAME ${dcm2itk}_makeNRRD COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${dcm2itk}Test>
         --compare ${BASELINE}/liver_seg.nrrd
         ${MODULE_TEMP_DIR}/1.nrrd
         ${dcm2itk}Test
         ${MODULE_TEMP_DIR}/liver.dcm
         ${MODULE_TEMP_DIR})
set_tests_properties(${dcm2itk}_makeNRRD PROPERTIES DEPENDS ${itk2dcm}_makeSEG)
set_property(TEST ${dcm2itk}_makeNRRD PROPERTY LABELS ${MODULE_NAME})


add_test(NAME seg_meta_roundtrip
  COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
  ${CMAKE_SOURCE_DIR}/doc/seg-example.json
  ${MODULE_TEMP_DIR}/meta.json)
set_tests_properties(seg_meta_roundtrip PROPERTIES DEPENDS ${dcm2itk}_makeNRRD)
set_property(TEST seg_meta_roundtrip PROPERTY LABELS ${MODULE_NAME})
