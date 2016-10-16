set(MODULE_NAME segimage2itkimage)

add_test(NAME ${MODULE_NAME}_hello
         COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME} --help)

set(BASELINE ${CMAKE_SOURCE_DIR}/data/ct-3slice/)

set(MODULE_TEMP_DIR ${TEMP_DIR}/seg)
make_directory(${MODULE_TEMP_DIR})

include_directories(${ITK_INCLUDE_DIRS})
add_executable(${MODULE_NAME}Test ${MODULE_NAME}Test.cxx )
target_link_libraries(${MODULE_NAME}Test ${${MODULE_NAME}_TARGET_LIBRARIES})
set_target_properties(${MODULE_NAME}Test PROPERTIES LABELS ${MODULE_NAME})
set_target_properties(${MODULE_NAME}Test PROPERTIES FOLDER ${${MODULE_NAME}_TARGETS_FOLDER})

add_test(NAME ${MODULE_NAME}_makeNRRD COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${MODULE_NAME}Test>
  --compare ${BASELINE}/liver_seg.nrrd
  ${MODULE_TEMP_DIR}/1.nrrd
  ${MODULE_NAME}Test
  ${BASELINE}/liver.dcm
  ${MODULE_TEMP_DIR})

set_property(TEST ${testname} PROPERTY LABELS ${MODULE_NAME})

set(MODULE_NAME itkimage2segimage)

add_test(NAME ${MODULE_NAME}_hello
         COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME} --help)
add_test(NAME ${MODULE_NAME}_makeSEG
         COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME}
        --metaDataFileName ${CMAKE_SOURCE_DIR}/doc/seg-example.json
        --segImageFiles ${BASELINE}/liver_seg.nrrd
        --dicomImageFiles ${BASELINE}/01.dcm,${BASELINE}/02.dcm,${BASELINE}/03.dcm
        --segDICOMFile ${MODULE_TEMP_DIR}/liver.dcm
       )

add_test(NAME seg_roundtrip
         COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
         ${MODULE_TEMP_DIR}/meta.json
         ${CMAKE_SOURCE_DIR}/doc/seg-example.json
        )
