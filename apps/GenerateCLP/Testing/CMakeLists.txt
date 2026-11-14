#-----------------------------------------------------------------------------
# Tests of dcmqiGenerateCLP and of the command-line interface it generates.
#

set(check_script ${CMAKE_CURRENT_SOURCE_DIR}/dcmqiCheckCommand.cmake)

# Items of SEM_LAUNCH_COMMAND, set when building as a Slicer extension.
string(REPLACE ";" "|" launcher "${SEM_LAUNCH_COMMAND}")
if(launcher)
  set(launcher "${launcher}|")
endif()

# dcmqi_add_command_test(<name> <label> <expected exit code> <regex> <command>...)
function(dcmqi_add_command_test name label expected_result regex)
  string(REPLACE ";" "|" command "${ARGN}")
  add_test(NAME ${name}
    COMMAND ${CMAKE_COMMAND}
      "-DCOMMAND=${command}"
      -DEXPECTED_RESULT=${expected_result}
      "-DEXPECTED_REGEX=${regex}"
      -P ${check_script}
    )
  set_property(TEST ${name} PROPERTY LABELS ${label})
endfunction()

#-----------------------------------------------------------------------------
# Descriptors that dcmqiGenerateCLP must reject
#
set(descriptor_dir ${CMAKE_CURRENT_BINARY_DIR}/descriptors)

# dcmqi_add_rejected_descriptor_test(<case> <expected error regex> <xml>)
function(dcmqi_add_rejected_descriptor_test case regex xml)
  file(WRITE ${descriptor_dir}/${case}.xml "${xml}")
  dcmqi_add_command_test(dcmqiGenerateCLP_rejects_${case} dcmqiGenerateCLP 1 "${regex}"
    $<TARGET_FILE:dcmqiGenerateCLP> ${descriptor_dir}/${case}.xml ${descriptor_dir}/${case}CLP.h
    )
endfunction()

dcmqi_add_rejected_descriptor_test(unsupportedParameterType "line 3, <float>: unsupported element" [[
<executable>
  <parameters>
    <float><name>x</name><longflag>x</longflag></float>
  </parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(unsupportedAttribute "unsupported attribute 'multiple'" [[
<executable>
  <parameters>
    <file multiple="true"><name>x</name><longflag>x</longflag></file>
  </parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(unsupportedParameterElement "<index>: unsupported element" [[
<executable>
  <parameters>
    <file><name>x</name><longflag>x</longflag><index>0</index></file>
  </parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(missingLongflag "a <longflag> .* is required" [[
<executable>
  <parameters>
    <file><name>x</name><flag>x</flag></file>
  </parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(duplicateLongflag "<longflag> 'x' is used more than once" [[
<executable>
  <parameters>
    <file><name>x</name><longflag>x</longflag></file>
    <file><name>y</name><longflag>--x</longflag></file>
  </parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(reservedLongflag "<longflag> 'xml' is reserved" [[
<executable>
  <parameters>
    <boolean><name>x</name><longflag>xml</longflag></boolean>
  </parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(booleanDefaultTrue "<default> of a boolean must be 'false'" [[
<executable>
  <parameters>
    <boolean><name>x</name><longflag>x</longflag><default>true</default></boolean>
  </parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(enumerationDefault "<default> is not one of the <element> values" [[
<executable>
  <parameters>
    <string-enumeration>
      <name>x</name><longflag>x</longflag><default>c</default>
      <element>a</element><element>b</element>
    </string-enumeration>
  </parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(integerDefault "<default> must be an integer" [[
<executable>
  <parameters>
    <integer><name>x</name><longflag>x</longflag><default>1.5</default></integer>
  </parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(unclosedElement "line 3: </parameters> does not close <file>" [[
<executable>
  <parameters>
    <file><name>x</name><longflag>x</longflag></parameters>
</executable>
]])

dcmqi_add_rejected_descriptor_test(cdataSection "CDATA sections .* are not supported" [==[
<executable>
  <description><![CDATA[text]]></description>
</executable>
]==])

#-----------------------------------------------------------------------------
# Command-line interface of the applications
#
set(applications
  paramaps/itkimage2paramap
  paramaps/paramap2itkimage
  seg/bin2labelsegimage
  seg/itkimage2segimage
  seg/segimage2itkimage
  sr/tid1500reader
  sr/tid1500writer
  )

foreach(application IN LISTS applications)
  get_filename_component(name ${application} NAME)

  # "--xml" prints the descriptor unchanged (after the version line)
  add_test(NAME ${name}_xml
    COMMAND ${CMAKE_COMMAND}
      "-DCOMMAND=${launcher}$<TARGET_FILE:${name}>|--xml"
      -DEXPECTED_RESULT=0
      -DEXPECTED_STDOUT_FILE=${DCMQI_SOURCE_DIR}/apps/${application}.xml
      -P ${check_script}
    )
  set_property(TEST ${name}_xml PROPERTY LABELS ${name})

  dcmqi_add_command_test(${name}_unknownFlag ${name} 1
    "Argument: --unknownFlag[^A-Za-z]+Couldn't find match for argument"
    ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${name}> --unknownFlag
    )
endforeach()

dcmqi_add_command_test(itkimage2segimage_badEnumeration itkimage2segimage 1
  "Value 'foo' does not meet constraint: binary\\|labelmap"
  ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:itkimage2segimage> --segmentationType foo
  )
dcmqi_add_command_test(segimage2itkimage_badEnumeration segimage2itkimage 1
  "Value 'foo' does not meet constraint: nrrd\\|mhd"
  ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:segimage2itkimage> -t foo
  )
dcmqi_add_command_test(itkimage2segimage_badInteger itkimage2segimage 1
  "Couldn't read argument value from string 'abc'"
  ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:itkimage2segimage> --skip abc
  )

# Comma-separated lists, as passed by 3D Slicer's DICOM plugins
dcmqi_add_command_test(itkimage2segimage_echoList itkimage2segimage 1
  "segImageFiles: a, b, c, [^a-z]"
  ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:itkimage2segimage> --echo --inputImageList a,b,c
  )
# Short flags, also when combined
dcmqi_add_command_test(segimage2itkimage_echoShortFlags segimage2itkimage 1
  "prefix: x[^A-Za-z]+outputType: nii[^A-Za-z]"
  ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:segimage2itkimage> --echo -p x -t nii
  )
