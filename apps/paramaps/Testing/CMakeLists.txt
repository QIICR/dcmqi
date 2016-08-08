set(BASELINE ${CMAKE_SOURCE_DIR}/data/paramaps/)
include_directories(${ITK_INCLUDE_DIRS})

set(MODULE_NAME itkimage2paramap)

add_executable(${MODULE_NAME}Test ${MODULE_NAME}Test.cxx )
target_link_libraries(${MODULE_NAME}Test ${${MODULE_NAME}_TARGET_LIBRARIES})
set_target_properties(${MODULE_NAME}Test PROPERTIES LABELS ${MODULE_NAME})
set_target_properties(${MODULE_NAME}Test PROPERTIES FOLDER ${${MODULE_NAME}_TARGETS_FOLDER})

add_test(NAME ${MODULE_NAME}_hello
         COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME} --help)
add_test(NAME ${MODULE_NAME}_makeParametricMap
         COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME}
        --metaDataFileName ${CMAKE_SOURCE_DIR}/doc/pm-example.json
        --inputFileName ${BASELINE}/pm-example.nrrd
        --outputParaMapFileName ${TEMP_DIR}/paramap.dcm
       )

set_property(TEST ${testname} PROPERTY LABELS ${MODULE_NAME})

