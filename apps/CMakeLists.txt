# Build-time tool generating the command-line parsing code of the applications
add_subdirectory(GenerateCLP)

add_subdirectory(paramaps)
add_subdirectory(seg)
add_subdirectory(sr)

if(BUILD_TESTING)
  add_subdirectory(GenerateCLP/Testing)
endif()
