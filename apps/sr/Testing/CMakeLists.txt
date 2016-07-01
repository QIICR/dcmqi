set(MODULE_NAME tid1500writer)

add_test(NAME ${MODULE_NAME}_hello
         COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME} --help)

add_test(NAME ${MODULE_NAME}_example
COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MODULE_NAME}
  --metaDataFileName ${CMAKE_SOURCE_DIR}/doc/sr-tid1500-example.json
  --outputFileName ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sr-tid1500-example.dcm
  )
