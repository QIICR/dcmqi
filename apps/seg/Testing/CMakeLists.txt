
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

#-----------------------------------------------------------------------------
# Equivalence guard for the in-memory handler overload of
# itkimage2dcmSegmentation. The string overload is implemented as a thin
# wrapper that parses its JSON argument into a handler and delegates to the
# handler overload; this test asserts the two overloads produce semantically
# equivalent DICOM datasets for the same input.
add_executable(Itk2DicomHandlerOverloadEquivalenceTest
  Itk2DicomHandlerOverloadEquivalenceTest.cxx)
target_link_libraries(Itk2DicomHandlerOverloadEquivalenceTest
  dcmqi
  ${DCMTK_LIBRARIES})
set_target_properties(Itk2DicomHandlerOverloadEquivalenceTest PROPERTIES
  LABELS ${MODULE_NAME})

dcmqi_add_test(
  NAME ${itk2dcm}_handlerOverloadEquivalence
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:Itk2DicomHandlerOverloadEquivalenceTest>
    ${CMAKE_SOURCE_DIR}/doc/examples/seg-example.json
    ${BASELINE}/liver_seg.nrrd
    ${DICOM_DIR}/01.dcm
    ${DICOM_DIR}/02.dcm
    ${DICOM_DIR}/03.dcm
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

dcmqi_add_test(
  NAME ${itk2dcm}_makeSEG_labelmap
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/seg-example.json
    --inputImageList ${BASELINE}/liver_seg.nrrd
    --inputDICOMDirectory ${DICOM_DIR}
    --outputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-direct.dcm
    --segmentationType labelmap
  )

# Multi-segment labelmap from two non-overlapping single-segment ITK inputs.
# Verifies that segment numbering for foreground starts at 1 (regression guard
# for the bug where the first foreground segment got number 0 and collided
# with the implicit background pixel value).
dcmqi_add_test(
  NAME ${itk2dcm}_makeSEG_labelmap_multi
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/seg-example_liver_spine.json
    --inputImageList ${BASELINE}/liver_seg.nrrd,${BASELINE}/spine_seg.nrrd
    --inputDICOMList ${DICOM_DIR}/01.dcm,${DICOM_DIR}/02.dcm,${DICOM_DIR}/03.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/liver_spine-labelmap.dcm
    --segmentationType labelmap
  )

# Same as above but exercises --useLabelIDAsSegmentNumber together with
# --segmentationType labelmap.
dcmqi_add_test(
  NAME ${itk2dcm}_makeSEG_labelmap_multi_labelID
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/seg-example_liver_spine.json
    --inputImageList ${BASELINE}/liver_seg.nrrd,${BASELINE}/spine_seg.nrrd
    --inputDICOMList ${DICOM_DIR}/01.dcm,${DICOM_DIR}/02.dcm,${DICOM_DIR}/03.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/liver_spine-labelmap-labelID.dcm
    --segmentationType labelmap
    --useLabelIDAsSegmentNumber
  )

# Overlap rejection: liver+spine+heart input. Of the three segments only
# liver and heart actually overlap (522 pixels, verified by pixel-level
# intersection of the input NRRDs); spine does not overlap with either.
# In labelmap mode the converter must abort because labelmaps cannot
# represent overlapping segments — each pixel value can encode only one
# segment number. The expected error is
#   "Cannot write labelmap SEG due to overlapping segments at slice ..."
# Reuses the existing 3-segment metadata so no new JSON file is needed.
# This is the ITK->labelmap counterpart of partial_overlaps_to_labelmap,
# which exercises the same rejection in the DICOM->labelmap path.
dcmqi_add_test(
  NAME ${itk2dcm}_makeSEG_labelmap_overlap
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${itk2dcm}>
    --inputMetadata ${CMAKE_SOURCE_DIR}/doc/examples/seg-example_multiple_segments.json
    --inputImageList ${BASELINE}/liver_seg.nrrd,${BASELINE}/spine_seg.nrrd,${BASELINE}/heart_seg.nrrd
    --inputDICOMList ${DICOM_DIR}/01.dcm,${DICOM_DIR}/02.dcm,${DICOM_DIR}/03.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/liver_spine_heart-labelmap.dcm
    --segmentationType labelmap
  )
set_tests_properties(${itk2dcm}_makeSEG_labelmap_overlap PROPERTIES WILL_FAIL TRUE)


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
    --useLabelIDAsSegmentNumber
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
    # Note: dciodvfy is intentionally not run on the labelmap outputs produced
    # by ${itk2dcm}_makeSEG_labelmap{,_multi,_multi_labelID} or by the
    # bin2labelsegimage labelmap producers. All labelmap segmentations emitted
    # by dcmqi today fail dciodvfy with
    #   Error - Missing attribute Type 2C Conditional Element=<PatientOrientation>
    #           Module=<GeneralImage>
    # which is a pre-existing compliance gap shared by every labelmap-emitting
    # path (Itk2DicomConverter and Bin2Label). Once Patient Orientation is
    # populated for labelmap output, add dciodvfy invocations for
    # liver-labelmap-direct.dcm and liver_spine-labelmap.dcm here.
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
  NAME ${dcm2itk}_makeNRRD_from_labelmap_direct
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/liver_seg.nrrd
    ${MODULE_TEMP_DIR}/makeNRRD_labelmap_direct-1.nrrd
    ${dcm2itk}Test
    --inputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-direct.dcm
    --outputDirectory ${MODULE_TEMP_DIR}
    --outputType nrrd
    --prefix makeNRRD_labelmap_direct
  TEST_DEPENDS
    ${itk2dcm}_makeSEG_labelmap
  )

# Roundtrip: multi-segment labelmap -> NRRD, compared against the merged
# liver+spine baseline (two non-overlapping segments collapse into a single
# output ITK image with labels 0/1/2).
dcmqi_add_test(
  NAME ${dcm2itk}_makeNRRD_from_labelmap_multi
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/liver_spine_seg.nrrd
    ${MODULE_TEMP_DIR}/makeNRRD_labelmap_multi-1.nrrd
    ${dcm2itk}Test
    --inputDICOM ${MODULE_TEMP_DIR}/liver_spine-labelmap.dcm
    --outputDirectory ${MODULE_TEMP_DIR}
    --outputType nrrd
    --prefix makeNRRD_labelmap_multi
  TEST_DEPENDS
    ${itk2dcm}_makeSEG_labelmap_multi
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
      ${itk2dcm}_makeSEG_merged_segment_files_from_partial_overlap
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
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/util/comparejson.py
    ${CMAKE_SOURCE_DIR}/doc/examples/seg-example.json
    ${MODULE_TEMP_DIR}/makeNRRD-meta.json
  TEST_DEPENDS
    ${dcm2itk}_makeNRRD
  )

dcmqi_add_test(
  NAME multi_seg_meta_roundtrip
  MODULE_NAME ${MODULE_NAME}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/util/comparejson.py
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
      --skip 0
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

set(seg2labelmap bin2labelsegimage)
foreach(phantom_test_labelmap ${TEST_SEG_SIZES})

  dcmqi_add_test(
    NAME seg_phantom_${phantom_test_labelmap}_to_labelmap
    MODULE_NAME ${MODULE_NAME}
    COMMAND $<TARGET_FILE:${seg2labelmap}>
      --inputDICOM ${CMAKE_SOURCE_DIR}/data/segmentations/${phantom_test_labelmap}/seg/label.seg
      --outputDICOM ${MODULE_TEMP_DIR}/${phantom_test_labelmap}-labelmap.seg
    )
endforeach()

set(SEG_SAMPLES liver)

foreach(more_test_labelmap ${SEG_SAMPLES})

  dcmqi_add_test(
    NAME ${more_test_labelmap}_to_labelmap
    MODULE_NAME ${MODULE_NAME}
    COMMAND $<TARGET_FILE:${seg2labelmap}>
      --inputDICOM ${CMAKE_SOURCE_DIR}/data/segmentations/${more_test_labelmap}.dcm
      --outputDICOM ${MODULE_TEMP_DIR}/${more_test_labelmap}-labelmap.dcm
    )

endforeach()

# ------------------------------------------------------------------------------
# Labelmap round-trip tests: binary SEG -> bin2labelsegimage -> segimage2itkimage -> compare
# Tests all 4 combinations of 8/16-bit and MONOCHROME2/PALETTE COLOR.
#
# Uses CTest fixtures so that tests are automatically skipped (not failed) when
# the prerequisite itkimage2segimage_makeSEG has not been run.

set_tests_properties(${itk2dcm}_makeSEG PROPERTIES FIXTURES_SETUP liver_seg_dcm)

# 8-bit MONOCHROME2
dcmqi_add_test(
  NAME liver_to_labelmap_mono8
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${seg2labelmap}>
    --inputDICOM ${MODULE_TEMP_DIR}/liver.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-mono8.dcm
    --compress none
  TEST_DEPENDS
    ${itk2dcm}_makeSEG
)
set_tests_properties(liver_to_labelmap_mono8 PROPERTIES
  FIXTURES_REQUIRED liver_seg_dcm
  FIXTURES_SETUP liver_labelmap_mono8
)

dcmqi_add_test(
  NAME ${dcm2itk}_labelmap_mono8
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/liver_seg.nrrd
    ${MODULE_TEMP_DIR}/labelmap_mono8-1.nrrd
    ${dcm2itk}Test
    --inputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-mono8.dcm
    --outputDirectory ${MODULE_TEMP_DIR}
    --outputType nrrd
    --prefix labelmap_mono8
  TEST_DEPENDS
    liver_to_labelmap_mono8
)
set_tests_properties(${dcm2itk}_labelmap_mono8 PROPERTIES
  FIXTURES_REQUIRED liver_labelmap_mono8
)

# 8-bit PALETTE COLOR
dcmqi_add_test(
  NAME liver_to_labelmap_palette8
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${seg2labelmap}>
    --inputDICOM ${MODULE_TEMP_DIR}/liver.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-palette8.dcm
    --compress none
    --usePalette
  TEST_DEPENDS
    ${itk2dcm}_makeSEG
)
set_tests_properties(liver_to_labelmap_palette8 PROPERTIES
  FIXTURES_REQUIRED liver_seg_dcm
  FIXTURES_SETUP liver_labelmap_palette8
)

dcmqi_add_test(
  NAME ${dcm2itk}_labelmap_palette8
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/liver_seg.nrrd
    ${MODULE_TEMP_DIR}/labelmap_palette8-1.nrrd
    ${dcm2itk}Test
    --inputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-palette8.dcm
    --outputDirectory ${MODULE_TEMP_DIR}
    --outputType nrrd
    --prefix labelmap_palette8
  TEST_DEPENDS
    liver_to_labelmap_palette8
)
set_tests_properties(${dcm2itk}_labelmap_palette8 PROPERTIES
  FIXTURES_REQUIRED liver_labelmap_palette8
)

# 16-bit MONOCHROME2
dcmqi_add_test(
  NAME liver_to_labelmap_mono16
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${seg2labelmap}>
    --inputDICOM ${MODULE_TEMP_DIR}/liver.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-mono16.dcm
    --compress none
    --force16Bit
  TEST_DEPENDS
    ${itk2dcm}_makeSEG
)
set_tests_properties(liver_to_labelmap_mono16 PROPERTIES
  FIXTURES_REQUIRED liver_seg_dcm
  FIXTURES_SETUP liver_labelmap_mono16
)

dcmqi_add_test(
  NAME ${dcm2itk}_labelmap_mono16
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/liver_seg.nrrd
    ${MODULE_TEMP_DIR}/labelmap_mono16-1.nrrd
    ${dcm2itk}Test
    --inputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-mono16.dcm
    --outputDirectory ${MODULE_TEMP_DIR}
    --outputType nrrd
    --prefix labelmap_mono16
  TEST_DEPENDS
    liver_to_labelmap_mono16
)
set_tests_properties(${dcm2itk}_labelmap_mono16 PROPERTIES
  FIXTURES_REQUIRED liver_labelmap_mono16
)

# 16-bit PALETTE COLOR
dcmqi_add_test(
  NAME liver_to_labelmap_palette16
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${seg2labelmap}>
    --inputDICOM ${MODULE_TEMP_DIR}/liver.dcm
    --outputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-palette16.dcm
    --compress none
    --usePalette
    --force16Bit
  TEST_DEPENDS
    ${itk2dcm}_makeSEG
)
set_tests_properties(liver_to_labelmap_palette16 PROPERTIES
  FIXTURES_REQUIRED liver_seg_dcm
  FIXTURES_SETUP liver_labelmap_palette16
)

dcmqi_add_test(
  NAME ${dcm2itk}_labelmap_palette16
  MODULE_NAME ${MODULE_NAME}
  COMMAND $<TARGET_FILE:${dcm2itk}Test>
    --compare ${BASELINE}/liver_seg.nrrd
    ${MODULE_TEMP_DIR}/labelmap_palette16-1.nrrd
    ${dcm2itk}Test
    --inputDICOM ${MODULE_TEMP_DIR}/liver-labelmap-palette16.dcm
    --outputDirectory ${MODULE_TEMP_DIR}
    --outputType nrrd
    --prefix labelmap_palette16
  TEST_DEPENDS
    liver_to_labelmap_palette16
)
set_tests_properties(${dcm2itk}_labelmap_palette16 PROPERTIES
  FIXTURES_REQUIRED liver_labelmap_palette16
)

# ------------------------------------------------------------------------------

set(SEG_SAMPLES partial_overlaps)

foreach(overlapping_segs ${SEG_SAMPLES})

  dcmqi_add_test(
    NAME ${overlapping_segs}_to_labelmap
    MODULE_NAME ${MODULE_NAME}
    COMMAND $<TARGET_FILE:${seg2labelmap}>
      --inputDICOM ${CMAKE_SOURCE_DIR}/data/segmentations/${overlapping_segs}.dcm
      --outputDICOM ${MODULE_TEMP_DIR}/${overlapping_segs}-labelmap.dcm
    )
  set_tests_properties(${overlapping_segs}_to_labelmap PROPERTIES WILL_FAIL TRUE)

endforeach()