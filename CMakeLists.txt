cmake_minimum_required (VERSION 3.0)
project(dcmqi)

find_package(DCMTK NO_MODULE REQUIRED)
find_package(SlicerExecutionModel NO_MODULE REQUIRED GenerateCLP)
find_package(ITK NO_MODULE REQUIRED)

include(${SlicerExecutionModel_USE_FILE})

include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindGit.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/dcmqiMacroExtractRepositoryInfo.cmake)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

dcmqiMacroExtractRepositoryInfo(VAR_PREFIX dcmqi)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/CMake/dcmqiVersionConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/dcmqiVersionConfigure.h
)

add_subdirectory(apps)
#add_subdirectory(lib)
