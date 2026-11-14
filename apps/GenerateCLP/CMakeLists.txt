#-----------------------------------------------------------------------------
# dcmqiGenerateCLP: build-time tool writing the command-line parsing header
# "<module>CLP.h" of each application from its XML descriptor. See
# CMake/dcmqiMacroBuildCLI.cmake. It only uses the C++ standard library.
#
# When cross-compiling, set DCMQI_GENERATECLP_EXECUTABLE to a copy of
# dcmqiGenerateCLP built for the host.
#
set(DCMQI_GENERATECLP_EXECUTABLE "" CACHE FILEPATH
  "dcmqiGenerateCLP executable to run during the build instead of building it (for cross-compiling)")
mark_as_advanced(DCMQI_GENERATECLP_EXECUTABLE)

if(DCMQI_GENERATECLP_EXECUTABLE)
  add_executable(dcmqiGenerateCLP IMPORTED GLOBAL)
  set_property(TARGET dcmqiGenerateCLP PROPERTY IMPORTED_LOCATION "${DCMQI_GENERATECLP_EXECUTABLE}")
else()
  add_executable(dcmqiGenerateCLP dcmqiGenerateCLP.cxx)
  set_target_properties(dcmqiGenerateCLP PROPERTIES FOLDER "Module-dcmqiGenerateCLP")
endif()
