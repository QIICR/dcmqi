file(GLOB json_files
  *.json
  )

file(GLOB json_example_files
  *example.json
  )

file(GLOB json_common_schema_files
  *common-schema.json
  )

file(GLOB json_schema_files
  *schema.json
  )
list(REMOVE_ITEM json_schema_files ${json_common_schema_files})

# general json syntax validation
foreach(json_file ${json_files})
  get_filename_component(json_name ${json_file} NAME_WE)
  add_test(NAME jsonlint_${json_name}
    COMMAND jsonlint ${json_file})
endforeach()

set(AJV_OPTIONS --all-errors)

# create argument list for all referenced common schemata
foreach(json_common_schema_file ${json_common_schema_files})
  # -r needs to be used when specifying a schema that has references to external files
  list(APPEND common_schema_references -r ${json_common_schema_file})
endforeach()

function(createSchemaValidationTest schema_name)
  get_filename_component(json_name ${schema_name} NAME_WE)
  add_test(NAME ajv_${json_name}-schema
    COMMAND ajv compile -s ${schema_name} ${common_schema_references} ${AJV_OPTIONS})
endfunction()

# validation of each common schema
foreach(json_common_schema_file ${json_common_schema_files})
  createSchemaValidationTest(${json_common_schema_file})
endforeach()

# validation of each specific (sr, seg, pm) schema file including referenced common schemata
foreach(json_schema_file ${json_schema_files})
  createSchemaValidationTest(${json_schema_file})
endforeach()

# function to simpify creation of example validation tests
function(createExampleDataTest example_name schema)
  set(json_example_file ${CMAKE_SOURCE_DIR}/doc/${example_name}-example.json)
  add_test(NAME ajv_${example_name}-example
    # -d stands for data that needs to be validated
    COMMAND ajv -s ${schema} ${common_schema_references} -d ${json_example_file} ${AJV_OPTIONS})
endfunction()

# validation of each specific (sr, seg, pm) example file including specific schema and referenced common schemata
set(SEG_EXAMPLES seg bmmr)
set(SEG_SCHEMA ${CMAKE_SOURCE_DIR}/doc/seg-schema.json)

foreach(seg_example_name ${SEG_EXAMPLES})
  createExampleDataTest(${seg_example_name} ${SEG_SCHEMA})
endforeach()

set(PM_EXAMPLES pm)
set(PM_SCHEMA ${CMAKE_SOURCE_DIR}/doc/pm-schema.json)

foreach(pm_example_name ${PM_EXAMPLES})
  createExampleDataTest(${pm_example_name} ${PM_SCHEMA})
endforeach()

set(SRTID1500_EXAMPLES sr-tid1500)
set(SRTID1500_SCHEMA ${CMAKE_SOURCE_DIR}/doc/sr-tid1500-schema.json)

foreach(srtid1500_example_name ${SRTID1500_EXAMPLES})
  createExampleDataTest(${srtid1500_example_name} ${SRTID1500_SCHEMA})
endforeach()

