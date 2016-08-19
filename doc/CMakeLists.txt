file(GLOB json_files
  *.json
  )

file(GLOB json_schema_files
  *schema.json
  )

foreach(json_file ${json_files})
  get_filename_component(json_name ${json_file} NAME_WE)
  add_test(NAME jsonlint_${json_name}
    COMMAND jsonlint ${json_file})
endforeach()

foreach(json_schema_file ${json_files})
  get_filename_component(json_name ${json_schema_file} NAME_WE)
  add_test(NAME ajv_${json_name}
    COMMAND ajv compile -s ${json_schema_file})
endforeach()

