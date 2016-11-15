cmake_minimum_required(VERSION 3.5.0)

if(DCMQI_BUILD_PARAMETRIC_MAPS)
  add_subdirectory(paramaps)
endif()
add_subdirectory(seg)
add_subdirectory(sr)
