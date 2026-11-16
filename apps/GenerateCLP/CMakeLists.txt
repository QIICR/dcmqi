#-----------------------------------------------------------------------------
# dcmqiGenerateCLP: build-time tool writing the command-line parsing header
# "<module>CLP.h" of each application from its XML descriptor. See
# CMake/dcmqiMacroBuildCLI.cmake. It only uses the C++ standard library.
#
# When cross-compiling, set DCMQI_GENERATECLP_EXECUTABLE (declared in the
# top-level CMakeLists.txt) to a copy of dcmqiGenerateCLP built for the host.
#
if(DCMQI_GENERATECLP_EXECUTABLE)
  add_executable(dcmqiGenerateCLP IMPORTED GLOBAL)
  set_property(TARGET dcmqiGenerateCLP PROPERTY IMPORTED_LOCATION "${DCMQI_GENERATECLP_EXECUTABLE}")
else()
  add_executable(dcmqiGenerateCLP dcmqiGenerateCLP.cxx)
  set_target_properties(dcmqiGenerateCLP PROPERTIES FOLDER "Module-dcmqiGenerateCLP")
endif()
