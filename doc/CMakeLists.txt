
find_program(JSONLINT_EXECUTABLE NAMES jsonlint.cmd jsonlint)
find_program(AJV_EXECUTABLE NAMES ajv.cmd ajv)

file(GLOB_RECURSE json_files
  *.json
  )

file(GLOB json_example_files
  *example.json
  )

file(GLOB json_seg_context_files
  segContexts/Segmentation*json
  )

file(GLOB json_anatomic_context_files
  segContexts/Anatomic*json
  )

file(GLOB json_common_schema_files
  *common-schema.json
  )

file(GLOB json_schema_files
  *schema.json
  )
list(REMOVE_ITEM json_schema_files ${json_common_schema_files})

#-----------------------------------------------------------------------------
# General json syntax validation
#-----------------------------------------------------------------------------

foreach(json_file ${json_files})
  get_filename_component(json_name ${json_file} NAME_WE)
  set(testname jsonlint_${json_name})
  if(JSONLINT_EXECUTABLE)
    add_test(NAME ${testname}
      COMMAND ${JSONLINT_EXECUTABLE} ${json_file})
    set_property(TEST ${testname} PROPERTY LABELS jsonlint)
  else()
    message(STATUS "Skipping '${testname}' test: jsonlint executable not found")
  endif()
endforeach()

#-----------------------------------------------------------------------------
# Schema validation
#-----------------------------------------------------------------------------

set(AJV_OPTIONS --all-errors)

# create argument list for all referenced common schemata
foreach(json_common_schema_file ${json_common_schema_files})
  # -r needs to be used when specifying a schema that has references to external files
  list(APPEND common_schema_references -r ${json_common_schema_file})
endforeach()

function(createSchemaValidationTest schema_name)
  get_filename_component(json_name ${schema_name} NAME_WE)
  set(testname ajv_${json_name})
  if(AJV_EXECUTABLE)
    add_test(NAME ${testname}
      COMMAND ${AJV_EXECUTABLE} compile -s ${schema_name} ${common_schema_references} ${AJV_OPTIONS})
    set_property(TEST ${testname} PROPERTY LABELS ajv)
  else()
    message(STATUS "Skipping '${testname}' test: ajv executable not found")
  endif()
endfunction()

# validation of each common schema
foreach(json_common_schema_file ${json_common_schema_files})
  createSchemaValidationTest(${json_common_schema_file})
endforeach()

# validation of each specific (sr, seg, pm) schema file including referenced common schemata
foreach(json_schema_file ${json_schema_files})
  createSchemaValidationTest(${json_schema_file})
endforeach()

#-----------------------------------------------------------------------------
# Example of validation tests
#-----------------------------------------------------------------------------

# function to simplify creation of example validation tests
function(createExampleDataTest example_name schema)
  set(json_example_file ${CMAKE_SOURCE_DIR}/doc/${example_name}-example.json)
  set(testname ajv_${example_name}-example)
  if(AJV_EXECUTABLE)
    add_test(NAME ${testname}
      # -d stands for data that needs to be validated
      COMMAND ${AJV_EXECUTABLE} -s ${schema} ${common_schema_references} -d ${json_example_file} ${AJV_OPTIONS})
    set_property(TEST ${testname} PROPERTY LABELS ajv)
  else()
    message(STATUS "Skipping '${testname}' test: ajv executable not found")
  endif()
endfunction()

# validate segmentation context files
set(SEGMENT_CONTEXT_SCHEMA ${CMAKE_SOURCE_DIR}/doc/segment-context-schema.json)
set(ANATOMIC_CONTEXT_SCHEMA ${CMAKE_SOURCE_DIR}/doc/anatomic-context-schema.json)
set(CONTEXT_COMMON_SCHEMA ${CMAKE_SOURCE_DIR}/doc/segment-context-common-schema.json)

function(createSegmentationContextTest seg_context_name)
  set(json_seg_context_file ${CMAKE_SOURCE_DIR}/doc/segContexts/SegmentationCategoryTypeModifier-${seg_context_name}.json)
  set(testname ajv_${seg_context_name}-segcontext)
  if(AJV_EXECUTABLE)
    add_test(NAME ${testname}
      # -d stands for data that needs to be validated
      COMMAND ${AJV_EXECUTABLE} -s ${SEGMENT_CONTEXT_SCHEMA} ${common_schema_references} -d ${json_seg_context_file} ${AJV_OPTIONS})
    set_property(TEST ${testname} PROPERTY LABELS ajv)
  else()
    message(STATUS "Skipping '${testname}' test: ajv executable not found")
  endif()
endfunction()

function(createAnatomicContextTest anatomic_context_name)
  set(json_anatomic_context_file ${CMAKE_SOURCE_DIR}/doc/segContexts/AnatomicRegionAndModifier-${anatomic_context_name}.json)
  set(testname ajv_${anatomic_context_name}-anatomiccontext)
  if(AJV_EXECUTABLE)
    add_test(NAME ${testname}
      # -d stands for data that needs to be validated
      COMMAND ${AJV_EXECUTABLE} -s ${ANATOMIC_CONTEXT_SCHEMA} ${common_schema_references} -d ${json_anatomic_context_file} ${AJV_OPTIONS})
    set_property(TEST ${testname} PROPERTY LABELS ajv)
  else()
    message(STATUS "Skipping '${testname}' test: ajv executable not found")
  endif()
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

set(SRTID1500_EXAMPLES sr-tid1500 sr-tid1500-ct-liver)
set(SRTID1500_SCHEMA ${CMAKE_SOURCE_DIR}/doc/sr-tid1500-schema.json)

foreach(srtid1500_example_name ${SRTID1500_EXAMPLES})
  createExampleDataTest(${srtid1500_example_name} ${SRTID1500_SCHEMA})
endforeach()

foreach(seg_context_name DICOM-Master SlicerGeneralAnatomy)
  createSegmentationContextTest(${seg_context_name})
endforeach()

foreach(anatomic_context_name DICOM-Master)
  createanatomicContextTest(${anatomic_context_name})
endforeach()
