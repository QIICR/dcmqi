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

foreach(json_file ${json_files})
  get_filename_component(json_name ${json_file} NAME_WE)
  add_test(NAME jsonlint_${json_name}
    COMMAND jsonlint ${json_file})
endforeach()

set(AJV_OPTIONS --all-errors)

foreach(json_common_schema_file ${json_common_schema_files})
  # -r needs to be used when specifying a schema that has references to external files
  list(APPEND REF_SCHEMATA -r ${json_common_schema_file})
endforeach()

foreach(json_common_schema_file ${json_common_schema_files})
  get_filename_component(json_name ${json_common_schema_file} NAME_WE)
  add_test(NAME ajv_${json_name}
    COMMAND ajv compile -s ${json_common_schema_file} ${REF_SCHEMATA} ${AJV_OPTIONS})
endforeach()

foreach(json_schema_file ${json_schema_files})
  get_filename_component(json_name ${json_schema_file} NAME_WE)
  add_test(NAME ajv_${json_name}
           COMMAND ajv compile -s ${json_schema_file} ${REF_SCHEMATA} ${AJV_OPTIONS})
endforeach()

foreach(json_example_file ${json_example_files})
  get_filename_component(json_name ${json_example_file} NAME_WE)
  STRING(REGEX REPLACE -example.json -schema.json json_schema ${json_example_file})
  add_test(NAME ajv_${json_name}
    # -d stands for data that needs to be validated
    COMMAND ajv -s ${json_schema} ${REF_SCHEMATA} -d ${json_example_file} ${AJV_OPTIONS})
endforeach()