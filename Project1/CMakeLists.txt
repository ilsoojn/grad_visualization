
project(geotest CXX)
cmake_minimum_required(VERSION 2.8)
 
find_package(VTK REQUIRED)

include(${VTK_USE_FILE})

#ENABLE_LANGUAGE(C)

add_executable(${PROJECT_NAME} geotest.cxx)

#include_directories(/usr/local/include/vtk-7.0)
target_link_libraries(${PROJECT_NAME} ${VTK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

