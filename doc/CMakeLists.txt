file(GLOB json_files
  *.json
  )

foreach(json_file ${json_files})
  get_filename_component(json_name ${json_file} NAME_WE)
  add_test(NAME jsonlint_${json_name}
    COMMAND jsonlint ${json_file})
endforeach()

set(BASELINE ${CMAKE_SOURCE_DIR}/doc/)
set(COMMON_SCHEMA ${BASELINE}/common-schema.json)
set(SR_COMMON_SCHEMA ${BASELINE}/sr-common-schema.json)

set(AJV_OPTIONS --all-errors)

get_filename_component(json_name common-schema.json NAME_WE)
add_test(NAME ajv_${json_name} COMMAND ajv compile -s ${BASELINE}/${json_name}.json)

get_filename_component(json_name sr-common-schema.json NAME_WE)
add_test(NAME ajv_${json_name} COMMAND ajv compile -s ${BASELINE}/${json_name}.json
  -r ${BASELINE}/common-schema.json)

get_filename_component(json_name seg-schema.json NAME_WE)
add_test(NAME ajv_${json_name} COMMAND ajv compile -s ${BASELINE}/${json_name}.json
                                                   -r ${COMMON_SCHEMA})

get_filename_component(json_name pm-schema.json NAME_WE)
add_test(NAME ajv_${json_name} COMMAND ajv compile -s ${BASELINE}/${json_name}.json
                                                   -r ${COMMON_SCHEMA})

get_filename_component(json_name sr-tid1500-schema.json NAME_WE)
add_test(NAME ajv_${json_name} COMMAND ajv compile -s ${BASELINE}/${json_name}.json
                                                   -r ${COMMON_SCHEMA}
                                                   -r ${SR_COMMON_SCHEMA})

get_filename_component(json_name seg-example.json NAME_WE)
add_test(NAME ajv_${json_name} COMMAND ajv -s ${BASELINE}/seg-schema.json
  -r ${COMMON_SCHEMA}
  -d ${BASELINE}/${json_name}.json)

get_filename_component(json_name pm-example.json NAME_WE)
add_test(NAME ajv_${json_name} COMMAND ajv -s ${BASELINE}/pm-schema.json
  -r ${COMMON_SCHEMA}
  -d ${BASELINE}/${json_name}.json)

get_filename_component(json_name sr-tid1500-example.json NAME_WE)
add_test(NAME ajv_${json_name} COMMAND ajv -s ${BASELINE}/sr-tid1500-schema.json
  -r ${COMMON_SCHEMA}
  -r ${SR_COMMON_SCHEMA}
  -d ${BASELINE}/${json_name}.json ${AJV_OPTIONS})
