project(bgsubtraction CXX)
cmake_minimum_required(VERSION 2.8)
 
find_package(VTK REQUIRED)
find_package(OpenCV REQUIRED)

include(${VTK_USE_FILE})

#ENABLE_LANGUAGE(C)

add_executable(${PROJECT_NAME} bgsubtraction.cxx)

#include_directories(/usr/local/include/vtk-6.0)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${VTK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

