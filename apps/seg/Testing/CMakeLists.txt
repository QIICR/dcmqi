
#-----------------------------------------------------------------------------
include(dcmqiTest)

#-----------------------------------------------------------------------------
set(MODULE_NAME segmentation)

#-----------------------------------------------------------------------------
set(BASELINE ${CMAKE_SOURCE_DIR}/data/segmentations)
set(DICOM_DIR ${BASELINE}/ct-3slice)
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
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/seg-example.json
    --inputImageList ${BASELINE}/liver_seg.nrrd
    --inputDICOMDirectory ${DICOM_DIR}
    --outputDICOM ${MODULE_TEMP_DIR}/liver.dcm
  )


# ------------------------------------------------------------------------------

# Creates a DICOM segmentation with 5 segments, partially overlapping:
# - Segments GREEN:
#   - overlaps with ORANGE on same frame
#   - overlaps with PURPLE on next frame
# - Segment ORANGE:
#   - overlaps with GREEN on same frame
#   - overlaps with PURPLE on next frame
# - Segment PURPLE:
#   - overlaps with GREEN on previous frame
#   - overlaps with ORANGE on previous frame
# - SEGMENT LIGHT_BLUE:
#   - no overlaps at all
# - SEGMENT DARK_BLUE:
#   - no overlaps at all
# This will produce 3 groups (i.e NRRD files):
# - Group 1: GREEN, LIGHT_BLUE, DARK_BLUE
# - Group 2: ORANGE
# - Group 3: PURPLE
dcmqi_add_test(
  NAME ${itk2dcm}_makeSEG_merged_segment_files_from_partial_overlap
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/seg-example_partial_overlaps.json
    --inputImageList ${BASELINE}/partial_overlaps-1.nrrd,${BASELINE}/partial_overlaps-2.nrrd,${BASELINE}/partial_overlaps-3.nrrd
    --inputDICOMList ${DICOM_DIR}/01.dcm,${DICOM_DIR}/02.dcm,${DICOM_DIR}/03.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/partial_overlaps.dcm
    --sortByLabelID
  )

# ------------------------------------------------------------------------------

# Creates a DICOM segmentation file that has 3 segments:
# - segment for liver (DICOM Segment Number 1)
# - segment for spine (DICOM Segment Number 2)
# - segment for heart (DICOM Segment Number 3)
dcmqi_add_test(
  NAME ${itk2dcm}_makeSEG_multiple_segment_files
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/seg-example_multiple_segments.json
    --inputImageList ${BASELINE}/liver_seg.nrrd,${BASELINE}/spine_seg.nrrd,${BASELINE}/heart_seg.nrrd
    --inputDICOMList ${DICOM_DIR}/01.dcm,${DICOM_DIR}/02.dcm,${DICOM_DIR}/03.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/liver_heart_seg.dcm
  )

  dcmqi_add_test(
    NAME ${itk2dcm}_makeSEG_multiple_segment_files_reordered
    MODULE_NAME ${MODULE_NAME}
    COMMAND $<TARGET_FILE:${itk2dcm}>
      --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/seg-example_multiple_segments_reordered.json
      --inputImageList ${BASELINE}/heart_seg.nrrd,${BASELINE}/liver_seg.nrrd,${BASELINE}/spine_seg.nrrd
      --inputDICOMList ${DICOM_DIR}/01.dcm,${DICOM_DIR}/02.dcm,${DICOM_DIR}/03.dcm
      --outputDICOM ${MODULE_TEMP_DIR}/liver_heart_seg_reordered.dcm
    )

find_program(DCIODVFY_EXECUTABLE dciodvfy)

if(EXISTS ${DCIODVFY_EXECUTABLE})
  dcmqi_add_test(
    NAME ${itk2dcm}_makeSEG_dciodvfy
    MODULE_NAME ${MODULE_NAME}
    COMMAND ${DCIODVFY_EXECUTABLE}
      ${MODULE_TEMP_DIR}/liver.dcm
    TEST_DEPENDS
      ${itk2dcm}_makeSEG
    )
  dcmqi_add_test(
    NAME ${itk2dcm}_makeSEG_multiple_segment_files_dciodvfy
    MODULE_NAME ${MODULE_NAME}
    COMMAND ${DCIODVFY_EXECUTABLE}
      ${MODULE_TEMP_DIR}/liver_heart_seg.dcm
    TEST_DEPENDS
      ${itk2dcm}_makeSEG_multiple_segment_files
    )
    dcmqi_add_test(
      NAME ${itk2dcm}_makeSEG_multiple_segment_files_reordered_dciodvfy
      MODULE_NAME ${MODULE_NAME}
      COMMAND ${DCIODVFY_EXECUTABLE}
        ${MODULE_TEMP_DIR}/liver_heart_seg_reordered.dcm
      TEST_DEPENDS
        ${itk2dcm}_makeSEG_multiple_segment_files_reordered
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

dcmqi_add_test_executable(
  MODULE_NAME ${dcm2itk}
  EXECUTABLE_NAME ${dcm2itk}Test
  )

dcmqi_add_test(
  NAME ${dcm2itk}_makeNRRD
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/liver_seg.nrrd
    ${MODULE_TEMP_DIR}/makeNRRD-1.nrrd
    ${dcm2itk}Test
    --inputDICOM ${MODULE_TEMP_DIR}/liver.dcm
    --outputDirectory ${MODULE_TEMP_DIR}
    --outputType nrrd
    --prefix makeNRRD
  TEST_DEPENDS
    ${itk2dcm}_makeSEG
  )

dcmqi_add_test(
  NAME ${dcm2itk}_makeNRRD_multiple_segment_files
  MODULE_NAME ${MODULE_NAME}
  COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/liver_seg.nrrd ${MODULE_TEMP_DIR}/makeNRRD_multiple_segments-1.nrrd
    --compare ${BASELINE}/spine_seg.nrrd ${MODULE_TEMP_DIR}/makeNRRD_multiple_segments-2.nrrd
    --compare ${BASELINE}/heart_seg.nrrd ${MODULE_TEMP_DIR}/makeNRRD_multiple_segments-3.nrrd
    ${dcm2itk}Test
    --inputDICOM ${MODULE_TEMP_DIR}/liver_heart_seg.dcm
    --outputDirectory ${MODULE_TEMP_DIR}
    --prefix makeNRRD_multiple_segments
  TEST_DEPENDS
    ${itk2dcm}_makeSEG_multiple_segment_files
  )

  dcmqi_add_test(
    NAME ${dcm2itk}_makeNRRD_multiple_segment_files_reordered
    MODULE_NAME ${MODULE_NAME}
    COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${dcm2itk}Test>
      --compare ${BASELINE}/liver_seg.nrrd ${MODULE_TEMP_DIR}/makeNRRD_multiple_segments_reordered-1.nrrd
      --compare ${BASELINE}/spine_seg.nrrd ${MODULE_TEMP_DIR}/makeNRRD_multiple_segments_reordered-2.nrrd
      --compare ${BASELINE}/heart_seg.nrrd ${MODULE_TEMP_DIR}/makeNRRD_multiple_segments_reordered-3.nrrd
      ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/liver_heart_seg_reordered.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
      --prefix makeNRRD_multiple_segments_reordered
    TEST_DEPENDS
      ${itk2dcm}_makeSEG_multiple_segment_files_reordered
    )

# ------------------------------------------------------------------------------

  # Reads a DICOM segmentation file that has 3 segments (liver, spine, heart - in this order).
  # Heart and liver segments overlap.
  # The goal is to export these segments to NRRD+JSON. Since the liver and heart segments overlap,
  # one output file will contain 2 segments (liver and spine) and the other will contain only
  # the heart. This will be reflected in the JSON metadata files with two entries in the outer
  # array (one for each NRRD file), one with 2 segments (liver,spine) and one with 1 segment (heart).
  dcmqi_add_test(
    NAME ${dcm2itk}_makeNRRD_merged_segment_file
    MODULE_NAME ${MODULE_NAME}
    COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${dcm2itk}Test>
      --compare ${BASELINE}/liver_spine_seg.nrrd ${MODULE_TEMP_DIR}/makeNRRD_merged_segment_file-1.nrrd
      --compare ${BASELINE}/heart_seg.nrrd ${MODULE_TEMP_DIR}/makeNRRD_merged_segment_file-2.nrrd
      ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/liver_heart_seg.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
      --prefix makeNRRD_merged_segment_file
      --mergeSegments
    TEST_DEPENDS
      ${itk2dcm}_makeSEG_multiple_segment_files
  )

  # Compare expected JSON output coming from makeNRRD_merged_segment_file test.
  dcmqi_add_test(
    NAME ${dcm2itk}_makeNRRD_merged_segment_file_JSON
    MODULE_NAME ${MODULE_NAME}
    COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
      ${CMAKE_SOURCE_DIR}/doc/examples/seg-example_multiple_segments_merged.json
      ${MODULE_TEMP_DIR}/makeNRRD_merged_segment_file-meta.json
    TEST_DEPENDS
      ${dcm2itk}_makeNRRD_merged_segment_file
  )

# ------------------------------------------------------------------------------


  # Reads a DICOM segmentation file (partial_overlaps.dcm) that has 5 segments,
  # - Segments GREEN:
  #   - overlaps with ORANGE on same frame
  #   - overlaps with PURPLE on next frame
  # - Segment ORANGE:
  #   - overlaps with GREEN on same frame
  #   - overlaps with PURPLE on next frame
  # - Segment PURPLE:
  #   - overlaps with GREEN on previous frame
  #   - overlaps with ORANGE on previous frame
  # - SEGMENT LIGHT_BLUE:
  #   - no overlaps at all
  # - SEGMENT DARK_BLUE:
  #   - no overlaps at all
  # This will produce 3 groups (i.e NRRD files):
  # - Group 1: GREEN, LIGHT_BLUE, DARK_BLUE
  # - Group 2: ORANGE
  # - Group 3: PURPLE

  dcmqi_add_test(
    NAME ${dcm2itk}_makeNRRD_merged_segment_files_from_partial_overlaps
    MODULE_NAME ${MODULE_NAME}
    COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${dcm2itk}Test>
      --compare ${BASELINE}/partial_overlaps-1.nrrd ${MODULE_TEMP_DIR}/makeNRRD_merged_segment_files_from_partial_overlaps-1.nrrd
      --compare ${BASELINE}/partial_overlaps-2.nrrd ${MODULE_TEMP_DIR}/makeNRRD_merged_segment_files_from_partial_overlaps-2.nrrd
      ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/partial_overlaps.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
      --prefix makeNRRD_merged_segment_files_from_partial_overlaps
      --mergeSegments
    TEST_DEPENDS
      ${itk2dcm}_makeSEG_multiple_segment_files
  )

  dcmqi_add_test(
    NAME ${dcm2itk}_makeNRRD_merged_segment_files_from_partial_overlaps_JSON
    MODULE_NAME ${MODULE_NAME}
    COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
      ${CMAKE_SOURCE_DIR}/doc/examples/seg-example_partial_overlaps.json
      ${MODULE_TEMP_DIR}/makeNRRD_merged_segment_files_from_partial_overlaps-meta.json
    TEST_DEPENDS
      ${dcm2itk}_makeNRRD_merged_segment_files_from_partial_overlaps
  )

# ------------------------------------------------------------------------------

dcmqi_add_test(
  NAME seg_meta_roundtrip
  MODULE_NAME ${MODULE_NAME}
  COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
    ${CMAKE_SOURCE_DIR}/doc/examples/seg-example.json
    ${MODULE_TEMP_DIR}/makeNRRD-meta.json
  TEST_DEPENDS
    ${dcm2itk}_makeNRRD
  )

dcmqi_add_test(
  NAME multi_seg_meta_roundtrip
  MODULE_NAME ${MODULE_NAME}
  COMMAND python ${CMAKE_SOURCE_DIR}/util/comparejson.py
    ${CMAKE_SOURCE_DIR}/doc/examples/seg-example_multiple_segments.json
    ${MODULE_TEMP_DIR}/makeNRRD_multiple_segments-meta.json
        "['labelID']"
  TEST_DEPENDS
    ${dcm2itk}_makeNRRD_multiple_segment_files
  )

set(TEST_SEG_SIZES 24x38x3 23x38x3)

foreach(seg_size ${TEST_SEG_SIZES})

  dcmqi_add_test(
    NAME ${itk2dcm}_makeSEG_${seg_size}
    MODULE_NAME ${MODULE_NAME}
    COMMAND $<TARGET_FILE:${itk2dcm}>
      --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/seg-example.json
      --inputImageList ${BASELINE}/${seg_size}/nrrd/label.nrrd
      --inputDICOMDirectory ${BASELINE}/${seg_size}/image
      --outputDICOM ${MODULE_TEMP_DIR}/${seg_size}_seg.dcm
    )

  dcmqi_add_test(
    NAME ${dcm2itk}_makeNRRD_${seg_size}
    MODULE_NAME ${MODULE_NAME}
    COMMAND $<TARGET_FILE:${dcm2itk}Test>
      --compare ${BASELINE}/${seg_size}/nrrd/label.nrrd
      ${MODULE_TEMP_DIR}/${seg_size}-1.nrrd
      ${dcm2itk}Test
      --inputDICOM ${MODULE_TEMP_DIR}/${seg_size}_seg.dcm
      --outputDirectory ${MODULE_TEMP_DIR}
      --outputType nrrd
      --prefix ${seg_size}
    TEST_DEPENDS
      ${itk2dcm}_makeSEG_${seg_size}
    )

endforeach()

